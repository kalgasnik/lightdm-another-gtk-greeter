/* main.c
 *
 * Copyright (C) 2012 Paddubsky A.V. <pan.pav.7c5@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <math.h>
#include <locale.h>
#include <stdlib.h>

#include <cairo-xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <X11/Xatom.h>
#include <lightdm.h>

#include "shares.h"
#include "composite_widgets.h"
#include "configuration.h"
#include "indicator_power.h"
#include "indicator_a11y.h"
#include "indicator_clock.h"
#include "indicator_layout.h"
#include "model_menu.h"
#include "model_listbox.h"

/* Static functions */

static gboolean connect_to_lightdm          (void);
static gboolean init_gui                    (void);
static void run_gui                         (void);
static void close_gui                       (void);

static gboolean load_users_list             (void);
static void load_sessions_list              (void);
static gboolean load_languages_list         (void);

static void init_user_selection             (void);
static void load_user_options               (LightDMUser* user);
static void set_screen_background           (GdkScreen* screen,
                                             GdkPixbuf* image,
                                             GdkRGBA color,
                                             gboolean set_props);
static void set_background                  (const gchar* value);
static void set_logo_image                  (void);
static void set_message_text                (const gchar* text);
static void set_prompt_text                 (const gchar* text);
static void set_login_button_state          (gboolean logged);
static gboolean update_date_label           (gpointer dummy);
static void set_login_button_width          (void);

static void take_screenshot                 (void);

static void start_authentication            (const gchar* username);
static void cancel_authentication           (void);
static void start_session                   (void);

static gchar* get_user_name                 (void);
static UserType get_user_type               (void);

static gchar* get_session                   (void);
static void set_session                     (const gchar* session);

static gchar* get_language                  (void);
static void set_language                    (const gchar* language);

void update_default_user_image              (void);
static void update_user_image               (void);

static GdkPixbuf* fit_image                 (GdkPixbuf* source,
                                             gint size,
                                             UserImageFit fit);

/* Callbacks and events */
static void on_sigterm_signal               (int signum);

/* LightDM callbacks */
static void on_show_prompt                  (LightDMGreeter* greeter_ptr,
                                             const gchar* text,
                                             LightDMPromptType type);
static void on_show_message                 (LightDMGreeter* greeter_ptr,
                                             const gchar* text,
                                             LightDMMessageType type);
static void on_authentication_complete      (LightDMGreeter* greeter_ptr);
static void on_autologin_timer_expired      (LightDMGreeter* greeter_ptr);
static void on_user_added                   (LightDMUserList* user_list,
                                             LightDMUser* user);
static void on_user_changed                 (LightDMUserList* user_list,
                                             LightDMUser* user);
static void on_user_removed                 (LightDMUserList* user_list,
                                             LightDMUser* user);

/* GUI callbacks */
void on_login_clicked                       (GtkWidget* widget,
                                             gpointer data);
void on_cancel_clicked                      (GtkWidget* widget,
                                             gpointer data);
void on_prompt_activate                     (GtkWidget* widget,
                                             gpointer data);
void on_user_selection_changed              (GtkWidget* widget,
                                             gpointer data);
gboolean on_user_selection_key_press        (GtkWidget* widget,
                                             GdkEventKey* event,
                                             gpointer data);
gboolean on_prompt_key_press                (GtkWidget* widget,
                                             GdkEventKey* event,
                                             gpointer data);
gboolean on_login_window_key_press          (GtkWidget* widget,
                                             GdkEventKey* event,
                                             gpointer data);
gboolean on_panel_window_key_press          (GtkWidget* widget,
                                             GdkEventKey* event,
                                             gpointer data);
gboolean on_special_key_press               (GtkWidget* widget,
                                             GdkEventKey* event,
                                             gpointer data);
void on_password_toggled                    (GtkWidget* widget,
                                             gpointer data);
gboolean on_password_mouse_clicked          (GtkWidget* widget,
                                             GdkEventButton* event,
                                             gpointer data);
void on_screen_changed                      (GtkWidget* widget,
                                             GdkScreen* previous_screen,
                                             gboolean update_layout);
void on_main_content_size_allocate          (GtkWidget *widget,
                                             GdkRectangle *allocation,
                                             gpointer data);

/* ------------------------------------------------------------------------- *
 * Definition: main
 * ------------------------------------------------------------------------- */

int main(int argc, char** argv)
{
    #ifdef _DEBUG_
    GREETER_DATA_DIR = g_build_filename(g_get_current_dir(), GREETER_DATA_DIR, NULL);
    #endif

    g_message("Another GTK+ Greeter version %s", PACKAGE_VERSION);
    signal(SIGTERM, on_sigterm_signal);

    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    gtk_init(&argc, &argv);

    load_settings();
    read_state();

    gboolean inited = connect_to_lightdm();
    inited &= init_gui();

    if(inited)
    {
        init_power_indicator();
        init_a11y_indicator();
        init_clock_indicator();
        init_layout_indicator();
        if(!greeter.state.theming.gtk_theme_applied)
            apply_gtk_theme(gtk_settings_get_default(), config.appearance.gtk_theme);
        run_gui();
        close_gui();
    }
    else
        g_critical("Greeter initialization failed");

    return inited ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* ------------------------------------------------------------------------- *
 * Definitions: static
 * ------------------------------------------------------------------------- */

static gboolean connect_to_lightdm(void)
{
    g_debug("Connecting to LightDM");

    greeter.greeter = lightdm_greeter_new();
    g_assert(greeter.greeter != NULL);

    #ifndef _DEBUG_
    GError* error = NULL;
    if(!lightdm_greeter_connect_sync(greeter.greeter, &error))
    {
        g_critical("Connection to LightDM failed: %s", error ? error->message : "unknown error");
        show_message_dialog(GTK_MESSAGE_ERROR, _("Error"), _("Connection to LightDM failed: %s"),
                            error ? error->message : _("unknown error"));
        g_clear_error(&error);
        return FALSE;
    }
    #endif

    g_signal_connect(greeter.greeter, "show-prompt", G_CALLBACK(on_show_prompt), NULL);
    g_signal_connect(greeter.greeter, "show-message", G_CALLBACK(on_show_message), NULL);
    g_signal_connect(greeter.greeter, "authentication-complete", G_CALLBACK(on_authentication_complete), NULL);
    g_signal_connect(greeter.greeter, "autologin-timer-expired", G_CALLBACK(on_autologin_timer_expired), NULL);
    g_signal_connect(lightdm_user_list_get_instance(), "user-added", G_CALLBACK(on_user_added), NULL);
    g_signal_connect(lightdm_user_list_get_instance(), "user-changed", G_CALLBACK(on_user_changed), NULL);
    g_signal_connect(lightdm_user_list_get_instance(), "user-removed", G_CALLBACK(on_user_removed), NULL);

    return TRUE;
}

static gboolean init_gui(void)
{
    if(!config.appearance.ui_file)
    {
        const gchar* message = "No UI file defined";
        show_message_dialog(GTK_MESSAGE_ERROR, _("Error"),
                            _("Error loading UI file:\n\n%s"), _(message));
        g_critical("Error loading UI file: %s", message);
        return FALSE;
    }

    g_message("Loading UI file: %s", config.appearance.ui_file);

    GError* error = NULL;
    GtkBuilder* builder = gtk_builder_new();
    if(!gtk_builder_add_from_file(builder, config.appearance.ui_file, &error))
    {
        show_message_dialog(GTK_MESSAGE_ERROR, _("Error"),
                            _("Error loading UI file:\n\n%s"), error->message);
        g_critical("Error loading UI file: %s", error->message);
        g_clear_error(&error);
        return FALSE;
    }

    struct BuilderWidget
    {
        /* Place to store widget reference */
        GtkWidget**  pwidget;
        /* Widget ID in .ui file */
        const gchar* name;
        GtkWidget**  default_widget;
    };

    const struct BuilderWidget WIDGETS[] =
    {
        {&greeter.ui.screen_window,             "screen_window",                NULL},
        {&greeter.ui.screen_layout,             "screen_layout",                NULL},

        {&greeter.ui.main_content,              "main_content",                 NULL},
        {&greeter.ui.main_layout,               "main_layout",                  &greeter.ui.main_content},
        {&greeter.ui.panel_content,             "panel_content",                NULL},
        {&greeter.ui.panel_layout,              "panel_layout",                 &greeter.ui.panel_content},
        {&greeter.ui.panel_menubar,             "panel_menubar",                NULL},
        {&greeter.ui.onboard_content,           "onboard_content",              NULL},
        {&greeter.ui.onboard_layout,            "onboard_layout",               &greeter.ui.onboard_content},
        {&greeter.ui.messagebox_content,        "messagebox_content",           NULL},
        {&greeter.ui.messagebox_layout,         "messagebox_layout",            &greeter.ui.messagebox_content},
        {&greeter.ui.messagebox_title,          "messagebox_title",             NULL},
        {&greeter.ui.messagebox_text,           "messagebox_text",              NULL},
        {&greeter.ui.messagebox_buttons,        "messagebox_buttons",           NULL},
        {&greeter.ui.messagebox_icon,           "messagebox_icon",              NULL},

        {&greeter.ui.login_widget,              "login_widget",                 NULL},
        {&greeter.ui.login_label,               "login_label",                  &greeter.ui.login_widget},
        {&greeter.ui.login_box,                 "login_box",                    &greeter.ui.login_widget},

        {&greeter.ui.no_prompt_login_widget,    "no_prompt_login_widget",       NULL},
        {&greeter.ui.no_prompt_login_label,     "no_prompt_login_label",        &greeter.ui.no_prompt_login_widget},
        {&greeter.ui.no_prompt_login_box,       "no_prompt_login_box",          &greeter.ui.no_prompt_login_widget},

        {&greeter.ui.cancel_widget,             "cancel_widget",                NULL},
        {&greeter.ui.cancel_box,                "cancel_box",                   &greeter.ui.cancel_widget},

        {&greeter.ui.message_widget,            "message_widget",               NULL},
        {&greeter.ui.message_box,               "message_box",                  &greeter.ui.message_widget},

        {&greeter.ui.prompt_entry,              "prompt_entry",                 NULL},
        {&greeter.ui.prompt_text,               "prompt_text",                  NULL},
        {&greeter.ui.prompt_box,                "prompt_box",                   NULL},

        {&greeter.ui.authentication_widget,     "authentication_widget",        NULL},
        {&greeter.ui.authentication_box,        "authentication_box",           &greeter.ui.authentication_widget},

        {&greeter.ui.users_widget,              "users_widget",                 NULL},
        {&greeter.ui.users_text,                "users_text",                   &greeter.ui.users_widget},
        {&greeter.ui.users_box,                 "users_box",                    &greeter.ui.users_widget},
        {(GtkWidget**)(&greeter.ui.users_model),"users_model",                  NULL},

        {&greeter.ui.sessions_widget,           "sessions_widget",              NULL},
        {&greeter.ui.sessions_text,             "sessions_text",                &greeter.ui.sessions_widget},
        {&greeter.ui.sessions_box,              "sessions_box",                 &greeter.ui.sessions_widget},
        {(GtkWidget**)(&greeter.ui.sessions_model), "sessions_model",           NULL},

        {&greeter.ui.languages_widget,          "languages_widget",             NULL},
        {&greeter.ui.languages_text,            "languages_text",               &greeter.ui.languages_widget},
        {&greeter.ui.languages_box,             "languages_box",                &greeter.ui.languages_widget},
        {(GtkWidget**)(&greeter.ui.languages_model), "languages_model",         NULL},

        {&greeter.ui.user_image_widget,         "user_image_widget",            NULL},
        {&greeter.ui.user_image_box,            "user_image_box",               &greeter.ui.user_image_widget},

        {&greeter.ui.date_widget,               "date_widget",                  NULL},
        {&greeter.ui.date_box,                  "date_box",                     &greeter.ui.date_widget},

        {&greeter.ui.host_widget,               "host_widget",                  NULL},
        {&greeter.ui.host_box,                  "host_box",                     &greeter.ui.host_widget},

        {&greeter.ui.logo_image_widget,         "logo_image_widget",            NULL},
        {&greeter.ui.logo_image_box,            "logo_image_box",               &greeter.ui.logo_image_widget},

        {&greeter.ui.power.widget,              "power_widget",                 NULL},
        {&greeter.ui.power.box,                 "power_box",                    &greeter.ui.power.widget},
        {&greeter.ui.power.menu,                "power_menu",                   NULL},
        {&greeter.ui.power.actions[POWER_ACTION_SUSPEND],  "power_suspend_widget",     NULL},
        {&greeter.ui.power.actions[POWER_ACTION_HIBERNATE],"power_hibernate_widget",   NULL},
        {&greeter.ui.power.actions[POWER_ACTION_RESTART],  "power_restart_widget",     NULL},
        {&greeter.ui.power.actions[POWER_ACTION_SHUTDOWN], "power_shutdown_widget",    NULL},
        {&greeter.ui.power.actions_box[POWER_ACTION_SUSPEND],  "power_suspend_box",    &greeter.ui.power.actions[POWER_ACTION_SUSPEND]},
        {&greeter.ui.power.actions_box[POWER_ACTION_HIBERNATE],"power_hibernate_box",  &greeter.ui.power.actions[POWER_ACTION_HIBERNATE]},
        {&greeter.ui.power.actions_box[POWER_ACTION_RESTART],  "power_restart_box",    &greeter.ui.power.actions[POWER_ACTION_RESTART]},
        {&greeter.ui.power.actions_box[POWER_ACTION_SHUTDOWN], "power_shutdown_box",   &greeter.ui.power.actions[POWER_ACTION_SHUTDOWN]},

        {&greeter.ui.a11y.widget,               "a11y_widget",                  NULL},
        {&greeter.ui.a11y.box,                  "a11y_box",                     &greeter.ui.a11y.widget},
        {&greeter.ui.a11y.menu,                 "a11y_menu",                    NULL},
        {&greeter.ui.a11y.osk_widget,           "a11y_osk_widget",              NULL},
        {&greeter.ui.a11y.osk_box,              "a11y_osk_box",                 &greeter.ui.a11y.osk_widget},
        {&greeter.ui.a11y.contrast_widget,      "a11y_contrast_widget",         NULL},
        {&greeter.ui.a11y.contrast_box,         "a11y_contrast_box",            &greeter.ui.a11y.contrast_widget},
        {&greeter.ui.a11y.font_widget,          "a11y_font_widget",             NULL},
        {&greeter.ui.a11y.font_box,             "a11y_font_box",                &greeter.ui.a11y.font_widget},
        {&greeter.ui.a11y.dpi_widget,           "a11y_dpi_widget",              NULL},
        {&greeter.ui.a11y.dpi_box,              "a11y_dpi_box",                 &greeter.ui.a11y.dpi_widget},

        {&greeter.ui.clock.time_widget,         "clock_time_widget",            NULL},
        {&greeter.ui.clock.time_box,            "clock_time_box",               &greeter.ui.clock.time_widget},
        {&greeter.ui.clock.time_menu,           "clock_time_menu",              NULL},
        {&greeter.ui.clock.date_widget,         "clock_date_widget",            NULL},
        {&greeter.ui.clock.date_box,            "clock_date_box",               &greeter.ui.clock.date_widget},

        {&greeter.ui.layout.widget,             "layout_widget",                NULL},
        {&greeter.ui.layout.box,                "layout_box",                   &greeter.ui.layout.widget},
        {&greeter.ui.layout.menu,               "layout_menu",                  NULL},

        {&greeter.ui.password_toggle_widget,    "password_toggle_widget",       NULL},
        {&greeter.ui.password_toggle_box,       "password_toggle_box",          &greeter.ui.password_toggle_widget},
        {NULL, NULL, FALSE}
    };

    for(const struct BuilderWidget* w = WIDGETS; w->pwidget != NULL; ++w)
    {
        *w->pwidget = GTK_WIDGET(gtk_builder_get_object(builder, w->name));
        if(!*w->pwidget && w->default_widget)
            *w->pwidget = *w->default_widget;
        if(*w->pwidget)
        {
            if(GTK_IS_IMAGE_MENU_ITEM(*w->pwidget) &&
               gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(*w->pwidget)))
                fix_image_menu_item_if_empty(GTK_IMAGE_MENU_ITEM(*w->pwidget));
        }
        else
        {
            g_warning("Widget is not found: %s", w->name);
        }
    }

    void update_widget_name(GObject* object,
                            gpointer nothing)
    {
        if(GTK_IS_WIDGET(object))
            gtk_widget_set_name(GTK_WIDGET(object), gtk_buildable_get_name(GTK_BUILDABLE(object)));
    }

    GSList* builder_widgets = gtk_builder_get_objects(builder);
    g_slist_foreach(builder_widgets, (GFunc)update_widget_name, NULL);
    g_slist_free(builder_widgets);

    /* Disabling system F10 hotkey */
    g_object_set(gtk_settings_get_default(), "gtk-menu-bar-accel", NULL, NULL);

    if(!greeter.ui.users_model)
        greeter.ui.users_model = GTK_LIST_STORE(get_widget_model(greeter.ui.users_widget));
    if(!greeter.ui.languages_model)
        greeter.ui.languages_model = GTK_LIST_STORE(get_widget_model(greeter.ui.languages_widget));
    if(!greeter.ui.sessions_model)
        greeter.ui.sessions_model = GTK_LIST_STORE(get_widget_model(greeter.ui.sessions_widget));

    struct ModelBindingData
    {
        GtkWidget*    widget;
        NewWidgetFunc new_widget;
        GtkListStore* model;
        GSList*       model_bindings;
        GtkWidget*    label;
        gint          label_column;
        GCallback     on_changed, on_activated;
    }
    models[] =
    {
        {
            greeter.ui.users_widget,
            user_widget_new,
            greeter.ui.users_model,
            config.appearance.templates.user.bindings,
            greeter.ui.users_text,
            USER_COLUMN_DISPLAY_NAME,
            G_CALLBACK(on_user_selection_changed),
            G_CALLBACK(on_login_clicked)
        },
        {
            greeter.ui.languages_widget,
            language_widget_new,
            greeter.ui.languages_model,
            config.appearance.templates.language.bindings,
            greeter.ui.languages_text,
            LANGUAGE_COLUMN_DISPLAY_NAME,
            NULL,
            NULL
        },
        {
            greeter.ui.sessions_widget,
            session_widget_new,
            greeter.ui.sessions_model,
            config.appearance.templates.session.bindings,
            greeter.ui.sessions_text,
            SESSION_COLUMN_DISPLAY_NAME,
            NULL,
            NULL
        },
        {NULL, NULL, NULL, NULL, NULL, -1, NULL, NULL}
    };

    for(struct ModelBindingData* model = models; model->new_widget != NULL; model++)
    {
        if(IS_MENU_WIDGET(model->widget))
            bind_menu_widget_model(model->widget,
                                   model->new_widget,
                                   model->model, model->model_bindings,
                                   model->label, model->label_column,
                                   model->on_changed);
        else if(GTK_IS_LIST_BOX(model->widget))
            bind_listbox_model(GTK_LIST_BOX(model->widget),
                               model->new_widget,
                               model->model, model->model_bindings,
                               model->on_changed,
                               model->on_activated);
    }

    if(config.appearance.user_image.enabled && greeter.ui.user_image_widget)
    {
        gint width, height;
        gtk_widget_get_preferred_width(greeter.ui.user_image_widget, &width, NULL);
        gtk_widget_get_preferred_height(greeter.ui.user_image_widget, &height, NULL);
        greeter.state.user_image.size = MIN(width, height);
    }

    if(config.appearance.list_image.enabled)
        greeter.state.list_image.size = config.appearance.list_image.size;

    update_default_user_image();

    if(!load_languages_list() || !config.greeter.show_language_selector)
        gtk_widget_hide(greeter.ui.languages_box);

    load_sessions_list();

    greeter.state.no_users_list = lightdm_greeter_get_hide_users_hint(greeter.greeter) ||
                                  !greeter.ui.users_widget;
    if(greeter.state.no_users_list)
        gtk_widget_hide(greeter.ui.users_box);
    else
        load_users_list();

    gdk_window_set_cursor(gdk_get_default_root_window(), gdk_cursor_new(GDK_LEFT_PTR));
    set_logo_image();
    set_widget_text(greeter.ui.host_widget, lightdm_get_hostname());

    /* Screen layout */
    rearrange_grid_child(GTK_GRID(greeter.ui.screen_layout), greeter.ui.main_layout, UI_LAYOUT_ROW_MAIN);

    if(config.panel.enabled)
    {
        rearrange_grid_child(GTK_GRID(greeter.ui.screen_layout), greeter.ui.panel_layout,
                             config.panel.position == PANEL_POS_TOP ? UI_LAYOUT_ROW_PANEL_TOP : UI_LAYOUT_ROW_PANEL_BOTTOM);
    }
    else
        gtk_widget_hide(greeter.ui.panel_layout);

    gtk_widget_hide(greeter.ui.onboard_layout);
    gtk_widget_hide(greeter.ui.messagebox_layout);

    gtk_builder_connect_signals(builder, greeter.greeter);

    return TRUE;
}

static void run_gui(void)
{
    if(config.appearance.fixed_login_button_width)
        set_login_button_width();

    if(greeter.ui.date_widget)
    {
        update_date_label(NULL);
        g_timeout_add_seconds(1, (GSourceFunc)update_date_label, NULL);
    }

    on_screen_changed(greeter.ui.screen_window, NULL, FALSE);
    init_user_selection();
    gtk_widget_show(greeter.ui.screen_window);
    update_main_window_layout();
    focus_main_window();
    if(config.appearance.background && !config.appearance.user_background)
        set_background(config.appearance.background);
    gtk_main();
}

static void close_gui(void)
{
    if(greeter.state.autostart_pid)
    {
        kill(greeter.state.autostart_pid, SIGTERM);
        g_spawn_close_pid(greeter.state.autostart_pid);
        greeter.state.autostart_pid = 0;
    }
}

static gint update_users_names_table(const gchar* display_name)
{
    gint* value = g_hash_table_lookup(greeter.state.users_display_names, display_name);
    if(!value)
    {
        value = g_malloc(sizeof(gint));
        *value = 0;
        g_hash_table_insert(greeter.state.users_display_names, g_strdup(display_name), value);
    }
    return ++(*value);
}

static gint get_same_name_count(const gchar* display_name)
{
    const gint* value = g_hash_table_lookup(greeter.state.users_display_names, display_name);
    return value ? *value : 0;
}

static void append_user(LightDMUser* user,
                        gboolean update_hash_table)
{
    const gchar* base_display_name = lightdm_user_get_display_name(user);
    const gchar* user_name = lightdm_user_get_name(user);
    const gchar* image_file = lightdm_user_get_image(user);
    gchar* display_name = NULL;
    GdkPixbuf* user_image = greeter.state.user_image.default_image;
    GdkPixbuf* list_image = greeter.state.list_image.default_image;

    g_debug("Adding user: %s (%s)", base_display_name, user_name);

    gint same_name_count = 0;
    if(update_hash_table)
        same_name_count = update_users_names_table(base_display_name);
    else
        same_name_count = get_same_name_count(base_display_name);

    if(config.appearance.user_name_format == USER_NAME_FORMAT_NAME)
        display_name = g_strdup(user_name);
    else if(config.appearance.user_name_format == USER_NAME_FORMAT_BOTH || same_name_count > 1)
        display_name = g_strdup_printf("%s (%s)", base_display_name, user_name);
    else
        display_name = g_strdup(base_display_name);

    if(config.appearance.user_image.enabled ||
       config.appearance.list_image.enabled)
    {
        GError* error = NULL;
        GdkPixbuf* image = gdk_pixbuf_new_from_file(image_file, &error);
        if(!image)
        {
            g_warning("Failed to load user image (%s): %s", user_name, error->message);
            g_clear_error(&error);
        }
        else
        {
            if(config.appearance.user_image.enabled)
                user_image = fit_image(image, greeter.state.user_image.size, config.appearance.user_image.fit);
            if(config.appearance.list_image.enabled)
                list_image = fit_image(image, greeter.state.list_image.size, config.appearance.list_image.fit);
            g_object_unref(image);
        }
    }

    GtkTreeIter iter;
    gtk_list_store_append(greeter.ui.users_model, &iter);
    gtk_list_store_set(greeter.ui.users_model, &iter,
                       USER_COLUMN_NAME, user_name,
                       USER_COLUMN_TYPE, USER_TYPE_REGULAR,
                       USER_COLUMN_DISPLAY_NAME, display_name,
                       USER_COLUMN_WEIGHT, lightdm_user_get_logged_in(user) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
                       USER_COLUMN_USER_IMAGE, user_image,
                       USER_COLUMN_LIST_IMAGE, list_image,
                       USER_COLUMN_LOGGED_IN, lightdm_user_get_logged_in(user),
                       -1);
    g_free(display_name);
}

static void append_custom_user(gint type,
                               const gchar* name,
                               const gchar* display_name)
{
    g_debug("Adding not real user: %s (%s)", display_name, name);

    GtkTreeIter iter;
    gtk_list_store_append(greeter.ui.users_model, &iter);
    gtk_list_store_set(greeter.ui.users_model, &iter,
                       USER_COLUMN_NAME, name,
                       USER_COLUMN_TYPE, type,
                       USER_COLUMN_DISPLAY_NAME, display_name,
                       USER_COLUMN_WEIGHT, PANGO_WEIGHT_NORMAL,
                       USER_COLUMN_USER_IMAGE, greeter.state.user_image.default_image,
                       USER_COLUMN_LIST_IMAGE, greeter.state.list_image.default_image,
                       USER_COLUMN_LOGGED_IN, FALSE,
                       -1);
}

static gboolean load_users_list(void)
{
    g_message("Reading users list");

    greeter.state.users_display_names = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);

    const GList* items = lightdm_user_list_get_users(lightdm_user_list_get_instance());
    const GList* item;
    GtkTreeIter iter;

    g_return_val_if_fail(items != NULL, FALSE);

    for(item = items; item != NULL; item = item->next)
        update_users_names_table(lightdm_user_get_display_name(item->data));

    for(item = items; item != NULL; item = item->next)
        append_user(item->data, FALSE);

    if(lightdm_greeter_get_has_guest_account_hint(greeter.greeter))
        append_custom_user(USER_TYPE_GUEST, USER_GUEST, _("Guest Account"));

    if(config.greeter.allow_other_users || greeter.state.no_users_list)
        append_custom_user(USER_TYPE_OTHER, USER_OTHER, _("Other..."));

    if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(greeter.ui.users_model), &iter))
    {
        g_warning("No users to display");
        return FALSE;
    }

    return TRUE;
}

static GdkPixbuf* get_session_image(const gchar* session,
                                    gboolean check_alter_names)
{
    static GHashTable* images_cache = NULL;
    if(!config.greeter.show_session_icon)
        return NULL;
    if(!images_cache)
        images_cache = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);

    GdkPixbuf* pixbuf = g_hash_table_lookup(images_cache, session);
    if(pixbuf)
        return pixbuf;

    gchar* image_name = g_strdup_printf("%s.png", session);
    gchar* image_path = g_build_filename(GREETER_DATA_DIR, image_name, NULL);
    pixbuf = gdk_pixbuf_new_from_file(image_path, NULL);
    g_free(image_path);
    g_free(image_name);

    if(pixbuf)
        g_hash_table_insert(images_cache, g_strdup(session), pixbuf);
    else if(check_alter_names)
    {
        if(g_strcmp0(session, "ubuntu-2d") == 0)
            pixbuf = get_session_image("ubuntu", FALSE);
        else if(g_strcmp0(session, "gnome-classic") == 0 ||
                g_strcmp0(session, "gnome-fallback") == 0 ||
                g_strcmp0(session, "gnome-shell") == 0)
            pixbuf = get_session_image("gnome", FALSE);
        else if(g_strcmp0(session, "kde-plasma") == 0)
            pixbuf = get_session_image("kde", FALSE);
    }
    return pixbuf;
}

static void load_sessions_list(void)
{
    g_message("Reading sessions list");
    GtkTreeIter iter;
    for(const GList* item = lightdm_get_sessions(); item != NULL; item = item->next)
    {
        LightDMSession* session = item->data;
        const gchar* name = lightdm_session_get_key(session);
        gtk_list_store_append(greeter.ui.sessions_model, &iter);
        gtk_list_store_set(greeter.ui.sessions_model, &iter,
                           SESSION_COLUMN_NAME, name,
                           SESSION_COLUMN_DISPLAY_NAME, lightdm_session_get_name(session),
                           SESSION_COLUMN_IMAGE, get_session_image(name, TRUE),
                           SESSION_COLUMN_COMMENT, lightdm_session_get_comment(session),
                           -1);
    }
}

static gboolean load_languages_list(void)
{
    g_message("Reading languages list");

    const GList* items = lightdm_get_languages();
    if(!items)
    {
        g_warning("lightdm_get_languages() return NULL");
        return FALSE;
    }
    GtkTreeIter iter;
    for(const GList* item = items; item != NULL; item = item->next)
    {
        LightDMLanguage* language = item->data;

        const gchar* code = lightdm_language_get_code(language);
        const gchar* country = lightdm_language_get_territory(language);
        gchar* label = country ? g_strdup_printf("%s - %s", lightdm_language_get_name(language), country)
                               : g_strdup(lightdm_language_get_name(language));

        const gchar* modifier = strchr(code, '@');
        if(modifier != NULL)
        {
            gchar* label_new = g_strdup_printf("%s [%s]", label, modifier + 1);
            g_free(label);
            label = label_new;
        }

        gtk_list_store_append(greeter.ui.languages_model, &iter);
        gtk_list_store_set(greeter.ui.languages_model, &iter,
                           LANGUAGE_COLUMN_CODE, code,
                           LANGUAGE_COLUMN_DISPLAY_NAME, label,
                           -1);
        g_free(label);
    }
    return TRUE;
}

static gboolean get_first_logged_user(GtkTreeIter* iter)
{
    if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(greeter.ui.users_model), iter))
        return FALSE;
    do
    {
        gchar* user_name = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(greeter.ui.users_model), iter, USER_COLUMN_NAME, &user_name, -1);
        LightDMUser* user = user_name ? lightdm_user_list_get_user_by_name(lightdm_user_list_get_instance(), user_name) : NULL;
        if(user && lightdm_user_get_logged_in(user))
        {
            g_free(user_name);
            return TRUE;
        }
        g_free(user_name);
    } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(greeter.ui.users_model), iter));

    return FALSE;
}

static void init_user_selection(void)
{
    if(greeter.state.no_users_list)
    {
        on_user_selection_changed(NULL, NULL);
        return;
    }

    gchar* last_logged_user = NULL;
    const gchar* selected_user = NULL;
    if(greeter.state.no_users_list)
        selected_user = USER_OTHER;
    else if(lightdm_greeter_get_select_user_hint(greeter.greeter))
        selected_user = lightdm_greeter_get_select_user_hint(greeter.greeter);
    else if(lightdm_greeter_get_select_guest_hint(greeter.greeter))
        selected_user = USER_GUEST;
    else
        selected_user = last_logged_user = get_state_value_str("greeter", "last-user");

    GtkTreeIter iter;
    if(selected_user && get_model_iter_str(greeter.ui.users_model,
                                           USER_COLUMN_NAME, selected_user, &iter))
        set_widget_active_iter(greeter.ui.users_widget, &iter);
    else if(get_first_logged_user(&iter))
        set_widget_active_iter(greeter.ui.users_widget, &iter);
    else
        set_widget_active_first(greeter.ui.users_widget);
    g_free(last_logged_user);
}

static void load_user_options(LightDMUser* user)
{
    const gboolean logged_in = user && lightdm_user_get_logged_in(user);

    set_widget_sensitive(greeter.ui.sessions_box, !logged_in);
    set_widget_sensitive(greeter.ui.languages_box, !logged_in);

    set_session(user ? lightdm_user_get_session(user) : NULL);
    set_language(user ? lightdm_user_get_language(user) : NULL);

    if(config.appearance.user_background)
    {
        const gchar* bg = user ? lightdm_user_get_background(user) : config.appearance.background;
        set_background(bg ? bg : config.appearance.background);
    }
}

static void set_screen_background(GdkScreen* screen,
                                  GdkPixbuf* image,
                                  GdkRGBA color,
                                  gboolean set_props)
{
    Atom prop_root = None;
    Atom prop_esetroot = None;
    Pixmap pixmap = None;
    Window root_window = RootWindow(GDK_SCREEN_XDISPLAY(screen), GDK_SCREEN_XNUMBER(screen));
    cairo_surface_t *surface;
    cairo_t *cairo;

    if(set_props)
    {
        prop_root = XInternAtom(GDK_SCREEN_XDISPLAY(screen), "_XROOTPMAP_ID", False);
        prop_esetroot = XInternAtom(GDK_SCREEN_XDISPLAY(screen), "ESETROOT_PMAP_ID", False);
        if(!prop_root && !prop_esetroot)
            set_props = FALSE;
    }

    Display* pixmap_display = XOpenDisplay(gdk_display_get_name(gdk_screen_get_display(screen)));
    XSetCloseDownMode(pixmap_display, RetainPermanent);
    pixmap = XCreatePixmap(pixmap_display, root_window,
                           DisplayWidth(GDK_SCREEN_XDISPLAY(screen), GDK_SCREEN_XNUMBER(screen)),
                           DisplayHeight(GDK_SCREEN_XDISPLAY(screen), GDK_SCREEN_XNUMBER(screen)),
                           DefaultDepth(GDK_SCREEN_XDISPLAY(screen), GDK_SCREEN_XNUMBER(screen)));
    XCloseDisplay(pixmap_display);

    surface = cairo_xlib_surface_create(GDK_SCREEN_XDISPLAY(screen), pixmap,
                                        DefaultVisual(GDK_SCREEN_XDISPLAY(screen), 0),
                                        DisplayWidth(GDK_SCREEN_XDISPLAY(screen), GDK_SCREEN_XNUMBER(screen)),
                                        DisplayHeight(GDK_SCREEN_XDISPLAY(screen), GDK_SCREEN_XNUMBER(screen)));

    cairo = cairo_create(surface);
    XSetWindowBackgroundPixmap(GDK_SCREEN_XDISPLAY(screen),
                               root_window, cairo_xlib_surface_get_drawable(surface));

    if(!image)
    {
        gdk_cairo_set_source_rgba(cairo, &color);
        cairo_paint(cairo);
    }
    else
    {
        GdkRectangle geometry;
        for(int monitor = 0; monitor < gdk_screen_get_n_monitors(screen); monitor++)
        {
            gdk_screen_get_monitor_geometry(screen, monitor, &geometry);
            GdkPixbuf* pixbuf = gdk_pixbuf_scale_simple(image, geometry.width, geometry.height, GDK_INTERP_BILINEAR);
            if(!gdk_pixbuf_get_has_alpha(pixbuf))
            {
                GdkPixbuf* p = gdk_pixbuf_add_alpha(pixbuf, FALSE, 255, 255, 255);
                g_object_unref(pixbuf);
                pixbuf = p;
            }
            gdk_cairo_set_source_pixbuf(cairo, pixbuf, geometry.x, geometry.y);
            cairo_paint(cairo);
            g_object_unref(pixbuf);
        }
    }
    cairo_destroy(cairo);
    cairo_surface_destroy(surface);

    if(set_props)
    {
        long pixmap_as_long = (long)pixmap;
        XChangeProperty(GDK_SCREEN_XDISPLAY(screen), root_window, prop_root, XA_PIXMAP,
                        32, PropModeReplace, (guchar*)&pixmap_as_long, 1);
        XChangeProperty(GDK_SCREEN_XDISPLAY(screen), root_window, prop_esetroot, XA_PIXMAP,
                        32, PropModeReplace, (guchar*)&pixmap_as_long, 1);
    }
    gdk_flush();
    XClearWindow(GDK_SCREEN_XDISPLAY(screen), root_window);
}

static gboolean on_draw_screen_background(GtkWidget* widget,
                                         cairo_t* cr,
                                         gpointer data)
{
    gdk_cairo_set_source_pixbuf(cr, greeter.state.window_background, 0, 0);
    cairo_paint(cr);
    return FALSE;
}

static void set_window_background(GdkWindow* window,
                                  GdkScreen* screen,
                                  GdkPixbuf* image,
                                  GdkRGBA* color)
{
    static gulong draw_handler_id = 0;
    if(image)
    {
        GdkPixbuf* pixbuf = gdk_pixbuf_scale_simple(image,
                                                    gdk_screen_get_width(screen),
                                                    gdk_screen_get_height(screen),
                                                    GDK_INTERP_BILINEAR);
        if(!gdk_pixbuf_get_has_alpha(pixbuf))
        {
            GdkPixbuf* p = gdk_pixbuf_add_alpha(pixbuf, FALSE, 255, 255, 255);
            g_object_unref(pixbuf);
            pixbuf = p;
        }
        greeter.state.window_background = pixbuf;
        gtk_widget_set_app_paintable(greeter.ui.screen_window, TRUE);
        draw_handler_id = g_signal_connect(greeter.ui.screen_window, "draw",
                                           G_CALLBACK(on_draw_screen_background), NULL);
    }
    else
    {
        if(draw_handler_id)
        {
            gtk_widget_set_app_paintable(greeter.ui.screen_window, FALSE);
            g_signal_handler_disconnect(greeter.ui.screen_window, draw_handler_id);
        }
        gtk_widget_override_background_color(greeter.ui.screen_window, GTK_STATE_FLAG_NORMAL, color);
    }
}

static void set_background(const gchar* value)
{
    if(g_strcmp0(value, greeter.state.last_background) == 0)
        return;

    GdkPixbuf* background_pixbuf = NULL;
    GdkRGBA background_color;

    if(gdk_rgba_parse(&background_color, value))
    {
        g_debug("Using background color: %s", value);
    }
    else
    {
        g_debug("Loading background from file: %s", value);

        GError* error = NULL;
        background_pixbuf = gdk_pixbuf_new_from_file(value, &error);
        if(error)
        {
            g_warning("Failed to load background: %s", error->message);
            g_clear_error(&error);
            return;
        }
    }

    for(int i = 0; i < gdk_display_get_n_screens(gdk_display_get_default()); ++i)
    {
        /* One screen, Gtk3 ? */
        GdkScreen* screen = gdk_display_get_screen(gdk_display_get_default(), i);
        if(screen == gtk_window_get_screen(GTK_WINDOW(greeter.ui.screen_window)))
            set_window_background(gtk_widget_get_window(greeter.ui.screen_window), screen,
                                  background_pixbuf, &background_color);
        set_screen_background(screen,
                              background_pixbuf, background_color,
                              config.appearance.x_background);
    }

    greeter.state.last_background = value;
    if(background_pixbuf)
        g_object_unref(background_pixbuf);
}

static void set_login_button_state(gboolean logged)
{
    const gchar* text = logged ? _(ACTION_TEXT_UNLOCK) : _(ACTION_TEXT_LOGIN);
    set_widget_text(greeter.ui.login_label, text);
    set_widget_text(greeter.ui.no_prompt_login_label, text);
}

static void set_logo_image(void)
{
    if(!greeter.ui.logo_image_widget || !config.appearance.logo)
        return;
    if(strlen(config.appearance.logo) == 0)
        gtk_widget_hide(greeter.ui.logo_image_box);
    else if(config.appearance.logo[0] == '#')
    {
        g_debug("Setting logo from icon: %s", config.appearance.logo);
        gchar* logo_string = config.appearance.logo + 1;
        gchar* comma = g_strrstr(logo_string, ",");
        gchar* icon_name = NULL;
        if(comma)
        {
            gint pixel_size = atol(comma + 1);
            if(comma > logo_string)
                icon_name = g_strndup(logo_string, comma - logo_string);
            if(pixel_size)
                gtk_image_set_pixel_size(GTK_IMAGE(greeter.ui.logo_image_widget), pixel_size);
        }
        else
            icon_name = g_strdup(logo_string);
        if(icon_name)
            gtk_image_set_from_icon_name(GTK_IMAGE(greeter.ui.logo_image_widget), icon_name, GTK_ICON_SIZE_INVALID);
        g_free(icon_name);
    }
    else
    {
        g_debug("Loading logo from file: %s", config.appearance.logo);
        GError* error = NULL;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(config.appearance.logo, &error);
        if(pixbuf)
            gtk_image_set_from_pixbuf(GTK_IMAGE(greeter.ui.logo_image_widget), pixbuf);
        else
            g_warning("Failed to load logo: %s", error->message);
        g_object_unref(pixbuf);
        g_clear_error(&error);
    }
}

static void set_message_text(const gchar* text)
{
    gtk_widget_set_visible(greeter.ui.message_box, text != NULL && strlen(text) > 0);
    set_widget_text(greeter.ui.message_widget, text);
}

static void set_prompt_text(const gchar* text)
{
    gtk_widget_set_visible(greeter.ui.prompt_box, text != NULL);
    gtk_widget_set_visible(greeter.ui.prompt_text, (text != NULL && !greeter.state.password_required) ||
                           (!config.appearance.hide_prompt_text && greeter.state.password_required));
    if(text)
        set_widget_text(greeter.ui.prompt_text, text);

    if(greeter.ui.no_prompt_login_box)
    {
        gtk_widget_set_visible(greeter.ui.login_box, text != NULL);
        gtk_widget_set_visible(greeter.ui.no_prompt_login_box, text == NULL);
    }
}

static gboolean update_date_label(gpointer dummy)
{
    GDateTime* datetime = g_date_time_new_now_local();
    g_return_val_if_fail(datetime != NULL, FALSE);
    gchar* str = g_date_time_format(datetime, config.appearance.date_format);
    set_widget_text(greeter.ui.date_widget, str);
    g_free(str);
    g_date_time_unref(datetime);
    return TRUE;
}

static void set_login_button_width(void)
{
    if(greeter.ui.login_label || GTK_IS_BIN(greeter.ui.login_widget))
    {
        if(!greeter.ui.login_label)
            greeter.ui.login_label = gtk_bin_get_child(GTK_BIN(greeter.ui.login_widget));
        if(GTK_IS_LABEL(greeter.ui.login_label))
        {
            int a = strlen(_(ACTION_TEXT_LOGIN));
            int b = strlen(_(ACTION_TEXT_UNLOCK));
            gtk_label_set_width_chars(GTK_LABEL(greeter.ui.login_label), a > b ? a : b);
        }
    }
}

static void take_screenshot(void)
{
    g_debug("Taking screenshot");
    gchar* file_dir = g_build_filename(g_get_tmp_dir(), APP_NAME, NULL);
    if(g_mkdir_with_parents(file_dir, 02755) != 0)
    {
        g_warning("Creating temporary directory: failed (%s)", file_dir);
        g_free(file_dir);
        return;
    }

    GdkWindow* root_window = gdk_get_default_root_window();
    GdkPixbuf* screenshot = gdk_pixbuf_get_from_window(root_window, 0, 0,
                                                       gdk_window_get_width(root_window),
                                                       gdk_window_get_height(root_window));

    GDateTime* datetime = g_date_time_new_now_local();

    gchar* file_name = datetime ? g_date_time_format(datetime, "screenshot_%F_%T.png") : g_strdup("screenshot.png");
    gchar* file_path = g_build_filename(file_dir, file_name, NULL);

    GError* error = NULL;
    gdk_pixbuf_save(screenshot, file_path, "png", &error, NULL);
    if(error)
    {
        g_warning("Failed: %s", error->message);
        show_message_dialog(GTK_MESSAGE_ERROR, _("Screenshot"),
                            "%s", error->message);
        g_clear_error(&error);
    }
    else
        g_message("Screenshot saved as \"%s\"", file_path);
    g_date_time_unref(datetime);
    g_object_unref(screenshot);
    g_free(file_path);
    g_free(file_name);
    g_free(file_dir);
}

static void start_authentication(const gchar* user_name)
{
    g_message("Starting authentication for user \"%s\"", user_name);

    set_state_value_str("greeter", "last-user", user_name);

    greeter.state.cancelling = FALSE;
    greeter.state.prompted = FALSE;

    LightDMUser* user = NULL;
    if(g_strcmp0(user_name, USER_OTHER) == 0)
        lightdm_greeter_authenticate(greeter.greeter, NULL);
    else if(g_strcmp0(user_name, USER_GUEST) == 0)
        lightdm_greeter_authenticate_as_guest(greeter.greeter);
    else
    {
        user = lightdm_user_list_get_user_by_name(lightdm_user_list_get_instance(), user_name);
        lightdm_greeter_authenticate(greeter.greeter, user_name);
    }
    load_user_options(user);
    set_login_button_state(user && lightdm_user_get_logged_in(user));
    gtk_widget_hide(greeter.ui.cancel_box);
    gtk_widget_hide(greeter.ui.authentication_box);
}

static void cancel_authentication(void)
{
    g_message("Cancelling authentication for current user");

    greeter.state.cancelling = FALSE;
    if(lightdm_greeter_get_in_authentication(greeter.greeter))
    {
        greeter.state.cancelling = TRUE;
        lightdm_greeter_cancel_authentication(greeter.greeter);
        set_message_text(NULL);
    }

    if(greeter.state.no_users_list)
        start_authentication(USER_OTHER);
    else
    {
        gchar* user_name = get_user_name();
        start_authentication(user_name);
        g_free(user_name);
    }
    focus_main_window();
}

static void start_session(void)
{
    g_message("Starting session for authenticated user");

    gchar* language = get_language();
    if(language)
    {
        lightdm_greeter_set_language(greeter.greeter, language);
        g_free(language);
    }

    gchar* user_name = get_user_name();
    set_state_value_str("greeter", "last-user", user_name);
    g_free(user_name);

    gchar* session = get_session();
    if(!session)
    {
        GtkTreeIter iter;
        if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(greeter.ui.sessions_model), &iter))
        {
            gtk_tree_model_get(GTK_TREE_MODEL(greeter.ui.sessions_model), &iter,
                               SESSION_COLUMN_NAME, &session, -1);
            show_message_dialog(0, "", "No session selected and no default session: using first in the list (%s)", session);
        }
        else
            show_message_dialog(0, "", "No session selected and no default session");
    }
    if(lightdm_greeter_start_session_sync(greeter.greeter, session, NULL))
    {
        gtk_widget_hide(greeter.ui.main_layout);
        gtk_widget_hide(greeter.ui.panel_layout);
        a11y_close();
    }
    else
    {
        set_message_text(_("Failed to start session"));
        start_authentication(lightdm_greeter_get_authentication_user(greeter.greeter));
    }
    g_free(session);
}

static gchar* get_user_name(void)
{
    return greeter.state.no_users_list ? g_strdup(USER_OTHER)
                                       : get_widget_selection_str(greeter.ui.users_widget,
                                                                  USER_COLUMN_NAME, NULL);
}

static UserType get_user_type(void)
{
    return greeter.state.no_users_list ? USER_TYPE_OTHER
                                       : get_widget_selection_int(greeter.ui.users_widget,
                                                                  USER_COLUMN_TYPE, USER_TYPE_REGULAR);
}

static gchar* get_session(void)
{
    return get_widget_selection_str(greeter.ui.sessions_widget,
                                    SESSION_COLUMN_NAME,
                                    lightdm_greeter_get_default_session_hint(greeter.greeter));
}

static void set_session(const gchar* session)
{
    GtkTreeIter iter;
    if(session && get_model_iter_str(greeter.ui.sessions_model, SESSION_COLUMN_NAME, session, &iter))
    {
        set_widget_active_iter(greeter.ui.sessions_widget, &iter);
        return;
    }

    const gchar* default_session = lightdm_greeter_get_default_session_hint(greeter.greeter);

    if(default_session && g_strcmp0(session, default_session) != 0)
        set_session(default_session);
    else
        set_widget_active_first(greeter.ui.sessions_widget);
}

static gchar* get_language(void)
{
    return get_widget_selection_str(greeter.ui.languages_widget, LANGUAGE_COLUMN_CODE, NULL);
}

static void set_language(const gchar* language)
{
    GtkTreeIter iter;
    if(language && get_model_iter_str(greeter.ui.languages_model, LANGUAGE_COLUMN_CODE, language, &iter))
    {
        set_widget_active_iter(greeter.ui.languages_widget, &iter);
        return;
    }

    const gchar* default_language = NULL;
    if(lightdm_get_language())
        default_language = lightdm_language_get_code(lightdm_get_language());
    if(default_language && g_strcmp0(default_language, language) != 0)
        set_language(default_language);
    else
        set_widget_active_first(greeter.ui.languages_widget);
}

void update_default_user_image(void)
{
    static gboolean is_file_and_readed = FALSE;

    if(is_file_and_readed || !(config.appearance.user_image.enabled || config.appearance.list_image.enabled))
        return;

    GdkPixbuf* old_user_image = greeter.state.user_image.default_image;
    GdkPixbuf* old_list_image = greeter.state.list_image.default_image;

    if(config.appearance.default_user_image[0] == '#')
    {
        const gchar* name = config.appearance.default_user_image + 1;
        greeter.state.user_image.default_image = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), name,
                                                                          greeter.state.user_image.size, 0, NULL);
        greeter.state.list_image.default_image = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), name,
                                                                          greeter.state.list_image.size, 0, NULL);
    }
    else
    {
        GdkPixbuf* image = gdk_pixbuf_new_from_file(config.appearance.default_user_image, NULL);
        if(image)
        {
            if(config.appearance.user_image.enabled)
                greeter.state.user_image.default_image = fit_image(image, greeter.state.user_image.size, config.appearance.user_image.fit);
            if(config.appearance.list_image.enabled)
                greeter.state.list_image.default_image = fit_image(image, greeter.state.list_image.size, config.appearance.list_image.fit);
            g_object_unref(image);
        }
        is_file_and_readed = TRUE;
    }
    GtkTreeIter iter;
    if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(greeter.ui.users_model), &iter))
        return;
    do
    {
        GdkPixbuf* user_image;
        GdkPixbuf* list_image;
        gtk_tree_model_get(GTK_TREE_MODEL(greeter.ui.users_model), &iter,
                           USER_COLUMN_USER_IMAGE, &user_image,
                           USER_COLUMN_LIST_IMAGE, &list_image, -1);
        if(config.appearance.user_image.enabled &&
           user_image == old_user_image)
        {
            gtk_list_store_set(greeter.ui.users_model, &iter, USER_COLUMN_USER_IMAGE, greeter.state.user_image.default_image, -1);
            g_object_unref(user_image);
        }
        if(config.appearance.list_image.enabled &&
           list_image == old_list_image)
        {
            gtk_list_store_set(greeter.ui.users_model, &iter, USER_COLUMN_LIST_IMAGE, greeter.state.list_image.default_image, -1);
            g_object_unref(list_image);
        }
    } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(greeter.ui.users_model), &iter));
    update_user_image();
}

static void update_user_image(void)
{
    if(config.appearance.user_image.enabled && greeter.ui.user_image_widget)
    {
        GdkPixbuf* image = get_widget_selection_image(greeter.ui.users_widget, USER_COLUMN_USER_IMAGE, NULL);
        if(config.appearance.user_image.fit == USER_IMAGE_FIT_NONE)
            gtk_widget_set_size_request(greeter.ui.user_image_widget,
                                        gdk_pixbuf_get_width(image), gdk_pixbuf_get_height(image));
        gtk_image_set_from_pixbuf(GTK_IMAGE(greeter.ui.user_image_widget), image);
    }
}

static GdkPixbuf* fit_image(GdkPixbuf* source,
                            gint new_size,
                            UserImageFit fit)
{
    if(new_size == 0)
        return NULL;
    gint width = gdk_pixbuf_get_width(source);
    gint height = gdk_pixbuf_get_height(source);
    gint src_size = MAX(width, height);
    if((fit == USER_IMAGE_FIT_ALL && src_size != new_size) ||
       (fit == USER_IMAGE_FIT_BIGGER && src_size > new_size) ||
       (fit == USER_IMAGE_FIT_SMALLER && src_size < new_size))
    {
        if(src_size == width)
            return gdk_pixbuf_scale_simple(source, new_size, (gint)height*new_size/src_size, GDK_INTERP_BILINEAR);
        else
            return gdk_pixbuf_scale_simple(source, (gint)width*new_size/src_size, new_size, GDK_INTERP_BILINEAR);
    }
    return (GdkPixbuf*)g_object_ref(source);
}

/* ------------------------------------------------------------------------- *
 * Definitions: callbacks
 * ------------------------------------------------------------------------- */

static void on_sigterm_signal(int signum)
{
    gtk_main_quit();
}

/* ------------------------------------------------------------------------- *
 * Definitions: LightDM callbacks
 * ------------------------------------------------------------------------- */

static void on_show_prompt(LightDMGreeter* greeter_ptr,
                           const gchar* text,
                           LightDMPromptType type)
{
    g_debug("LightDM signal: show-prompt (%s)", text);

    greeter.state.password_required = (type == LIGHTDM_PROMPT_TYPE_SECRET);
    greeter.state.prompted = TRUE;
    set_prompt_text(dgettext("Linux-PAM", text));
    gtk_entry_set_text(GTK_ENTRY(greeter.ui.prompt_entry), "");
    gtk_entry_set_visibility(GTK_ENTRY(greeter.ui.prompt_entry),
                             !greeter.state.password_required || greeter.state.show_password);
    gtk_widget_set_visible(greeter.ui.password_toggle_box,
                           config.greeter.allow_password_toggle && greeter.state.password_required);
    set_widget_toggled(greeter.ui.password_toggle_widget,
                       config.appearance.invert_password_state ? !greeter.state.show_password : greeter.state.show_password,
                       G_CALLBACK(on_password_toggled));
    gtk_widget_set_sensitive(greeter.ui.prompt_entry, TRUE);
    gtk_widget_set_sensitive(greeter.ui.login_widget, TRUE);
    focus_main_window();
}

static void on_show_message(LightDMGreeter* greeter_ptr,
                            const gchar* text,
                            LightDMMessageType type)
{
    g_debug("LightDM signal: show-message(%d: %s)", type, text);
    set_message_text(text);
}

static void on_authentication_complete(LightDMGreeter* greeter_ptr)
{
    g_debug("LightDM signal: authentication-complete");
    gtk_entry_set_text(GTK_ENTRY(greeter.ui.prompt_entry), "");

    if(greeter.state.cancelling)
    {
        cancel_authentication();
        return;
    }

    set_prompt_text(NULL);
    focus_main_window();

    #ifdef _DEBUG_
    return;
    #endif

    if(lightdm_greeter_get_is_authenticated(greeter.greeter))
    {
        if(greeter.state.prompted)
            start_session();
    }
    else
    {
        if(greeter.state.prompted)
        {
            set_message_text(_("Incorrect password, please try again"));
            lightdm_greeter_authenticate(greeter.greeter, lightdm_greeter_get_authentication_user(greeter.greeter));
        }
        else
        {
            set_message_text(_("Failed to authenticate"));
            gchar* user_name = get_user_name();
            start_authentication(user_name);
            g_free(user_name);
        }
    }
    gtk_widget_hide(greeter.ui.authentication_box);
}

static void on_autologin_timer_expired(LightDMGreeter* greeter_ptr)
{
    g_debug("LightDM signal: autologin-timer-expired");
    lightdm_greeter_authenticate_autologin(greeter_ptr);
}

static void on_user_added(LightDMUserList* user_list,
                          LightDMUser* user)
{
    g_debug("LightDM signal: user-added");
    append_user(user, TRUE);
}

static void on_user_changed(LightDMUserList* user_list,
                            LightDMUser* user)
{
    g_debug("LightDM signal: user-changed");
    GtkTreeIter iter;
    const gchar* name = lightdm_user_get_name(user);
    if(!get_model_iter_str(greeter.ui.users_model, USER_COLUMN_NAME, name, &iter))
        return;

    gtk_list_store_set(greeter.ui.users_model, &iter,
                       USER_COLUMN_NAME, name,
                       USER_COLUMN_DISPLAY_NAME, lightdm_user_get_display_name(user),
                       USER_COLUMN_WEIGHT, lightdm_user_get_logged_in(user) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
                       -1);
}

void on_user_removed(LightDMUserList* user_list,
                     LightDMUser* user)
{
    g_debug("LightDM signal: user-removed");
    GtkTreeIter iter;
    const gchar* name = lightdm_user_get_name(user);
    if(!get_model_iter_str(greeter.ui.users_model, USER_COLUMN_NAME, name, &iter))
        return;
    gtk_list_store_remove(greeter.ui.users_model, &iter);
}

/* ------------------------------------------------------------------------- *
 * Definitions: GUI callbacks
 * ------------------------------------------------------------------------- */

void on_login_clicked(GtkWidget* widget,
                      gpointer data)
{
    if(lightdm_greeter_get_is_authenticated(greeter.greeter))
        start_session();
    else if(lightdm_greeter_get_in_authentication(greeter.greeter))
    {
        const gchar* text = gtk_entry_get_text(GTK_ENTRY(greeter.ui.prompt_entry));
        lightdm_greeter_respond(greeter.greeter, text);
        gtk_widget_show(greeter.ui.cancel_box);
        if(get_user_type() == USER_TYPE_OTHER  && gtk_entry_get_visibility(GTK_ENTRY(greeter.ui.prompt_entry)))
        {
            LightDMUser* user = lightdm_user_list_get_user_by_name(lightdm_user_list_get_instance(), text);
            set_login_button_state(user && lightdm_user_get_logged_in(user));
            load_user_options(user);
        }
        else
        {
            gtk_widget_show(greeter.ui.authentication_box);
        }
    }
    else
    {
        g_warning("Authentication state is undeterminated");
        return;
    }
    gtk_widget_set_sensitive(greeter.ui.prompt_entry, FALSE);
    gtk_widget_set_sensitive(greeter.ui.login_widget, FALSE);
    set_message_text(NULL);
}

void on_cancel_clicked(GtkWidget* widget,
                       gpointer data)
{
    cancel_authentication();
}

void on_prompt_activate(GtkWidget* widget,
                        gpointer data)
{
    on_login_clicked(widget, NULL);
}

void on_user_selection_changed(GtkWidget* widget,
                               gpointer data)
{
    gchar* user_name = get_user_name();
    g_debug("User selection changed: %s", user_name);

    update_user_image();
    set_message_text(NULL);
    start_authentication(user_name);
    #ifdef _DEBUG_
    if(get_user_type() == USER_TYPE_OTHER ||
       !lightdm_user_get_logged_in(lightdm_user_list_get_user_by_name(lightdm_user_list_get_instance(), user_name)))
    {
        gboolean prompt(gpointer data)
        {
            on_show_prompt(greeter.greeter, "[debug] password:", LIGHTDM_PROMPT_TYPE_SECRET);
            return FALSE;
        }
        g_timeout_add(75, prompt, NULL);
    }
    else
        on_authentication_complete(greeter.greeter);
    #endif
    g_free(user_name);
}

gboolean on_user_selection_key_press(GtkWidget* widget,
                                     GdkEventKey* event,
                                     gpointer data)
{
    switch(event->keyval)
    {
        case GDK_KEY_Return:
            on_login_clicked(NULL, NULL);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

gboolean on_prompt_key_press(GtkWidget* widget,
                             GdkEventKey* event,
                             gpointer data)
{
    if(!greeter.state.no_users_list &&
       (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_Down))
    {
        GtkTreeIter iter;
        if(!get_widget_active_iter(greeter.ui.users_widget, &iter))
            return FALSE;
        gboolean has_next;
        if(event->keyval == GDK_KEY_Up)
            has_next = gtk_tree_model_iter_previous(GTK_TREE_MODEL(greeter.ui.users_model), &iter);
        else
            has_next = gtk_tree_model_iter_next(GTK_TREE_MODEL(greeter.ui.users_model), &iter);
        if(has_next)
            set_widget_active_iter(greeter.ui.users_widget, &iter);
        return TRUE;
    }
    return FALSE;
}

gboolean on_login_window_key_press(GtkWidget* widget,
                                   GdkEventKey* event,
                                   gpointer data)
{
    static guint32 escape_time = 0;
    switch(event->keyval)
    {
        case GDK_KEY_F10:
            if(GTK_IS_MENU_SHELL(greeter.ui.panel_menubar))
                gtk_menu_shell_select_first(GTK_MENU_SHELL(greeter.ui.panel_menubar), TRUE);
            else if(greeter.ui.panel_menubar)
                gtk_widget_grab_focus(greeter.ui.panel_content);
            break;
        case GDK_KEY_Escape:
            if(escape_time && event->time - escape_time <= config.greeter.double_escape_time)
            {   /* Double <Esc> */
                escape_time = 0;
                init_user_selection();
            }
            else
            {   /* Escape */
                escape_time = event->time;
                cancel_authentication();
            }
            break;
        default:
            escape_time = 0;
            return FALSE;
    }
    if(event->keyval != GDK_KEY_Escape)
        escape_time = 0;
    return TRUE;
}

gboolean on_panel_window_key_press(GtkWidget* widget,
                                   GdkEventKey* event,
                                   gpointer data)
{
    switch(event->keyval)
    {
        case GDK_KEY_F10: case GDK_KEY_Escape:
            if(greeter.ui.panel_menubar)
                gtk_menu_shell_cancel(GTK_MENU_SHELL(greeter.ui.panel_menubar));
            focus_main_window();
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

gboolean on_special_key_press(GtkWidget* widget,
                              GdkEventKey* event,
                              gpointer data)
{
    switch(event->keyval)
    {
        case GDK_KEY_F1:
            a11y_toggle_osk();
            break;
        case GDK_KEY_F2:
            a11y_toggle_font();
            break;
        case GDK_KEY_F3:
            a11y_toggle_contrast();
            break;
        case GDK_KEY_F4:
            a11y_toggle_dpi();
            break;
        case GDK_KEY_Print:
            take_screenshot();
            break;
        case GDK_KEY_PowerOff:
            do_power_action(config.power.button_press_action);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

void on_password_toggled(GtkWidget* widget,
                         gpointer data)
{
    if(config.greeter.allow_password_toggle && greeter.state.password_required)
    {
        greeter.state.show_password = !greeter.state.show_password;
        set_widget_toggled(greeter.ui.password_toggle_widget,
                           config.appearance.invert_password_state ? !greeter.state.show_password : greeter.state.show_password,
                           G_CALLBACK(on_password_toggled));
        gtk_entry_set_visibility(GTK_ENTRY(greeter.ui.prompt_entry), greeter.state.show_password);
    }
}

gboolean on_password_mouse_clicked(GtkWidget* widget,
                                   GdkEventButton* event,
                                   gpointer data)
{
    if(event->type == GDK_BUTTON_PRESS && event->button == 1)
        on_password_toggled(widget, data);
    return FALSE;
}

void on_screen_changed(GtkWidget* widget,
                       GdkScreen* previous_screen,
                       gboolean update_layout)
{
    GdkRectangle geometry;
    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(greeter.ui.screen_window));
    gdk_screen_get_monitor_geometry(screen, gdk_screen_get_primary_monitor(screen), &geometry);
    gtk_window_set_resizable(GTK_WINDOW(greeter.ui.screen_window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(greeter.ui.screen_window), geometry.width, geometry.height);
    gtk_widget_set_size_request(greeter.ui.screen_window, geometry.width, geometry.height);
    gtk_window_move(GTK_WINDOW(greeter.ui.screen_window), geometry.x, geometry.y);
    gtk_window_set_resizable(GTK_WINDOW(greeter.ui.screen_window), FALSE);
    if(update_layout)
        update_main_window_layout();
}

void on_main_content_size_allocate(GtkWidget *widget,
                                   GdkRectangle *allocation,
                                   gpointer data)
{
    static GdkRectangle old_allocation;
    if(allocation->width != old_allocation.width || allocation->height != old_allocation.height ||
       allocation->x != old_allocation.x || allocation->y != old_allocation.y)
    {
        old_allocation = *allocation;
        update_main_window_layout();
    }
}
