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

#include <stdlib.h>
#include <locale.h>

#include <cairo-xlib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <lightdm.h>

#include "shares.h"
#include "configuration.h"
#include "indicator_power.h"
#include "indicator_a11y.h"
#include "indicator_clock.h"
#include "indicator_layout.h"

/* Static functions */

static gboolean connect_to_lightdm          (void);
static gboolean init_css                    (void);
static gboolean init_gui                    (void);
static void set_background                  (const char* value);
static void run_gui                         (void);

static gboolean load_users_list             (void);
static gboolean load_sessions_list          (void);
static gboolean load_languages_list         (void);

static void init_user_selection             (void);
static void load_user_options               (LightDMUser* user);
static void set_logo_image                  (void);
static void set_message_label               (const gchar* text);
static void set_login_button_state          (LoginButtonState state);
static void update_minimal_user_image_size  (void);
static gboolean update_date_label           (gpointer dummy);

static void take_screenshot                 (void);

static void start_authentication            (const gchar* username);
static void cancel_authentication           (void);
static void start_session                   (void);

static gchar* get_user                      (void);
static UserType get_user_type               (void);

static gchar* get_session                   (void);
static void set_session                     (const gchar* session);

static gchar* get_language                  (void);
static void set_language                    (const gchar* language);

static cairo_surface_t* create_root_surface (GdkScreen* screen);

/* Callbacks and events */
static void sigterm_callback                (int signum);

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
void on_center_window                       (GtkWidget* widget,
                                             gpointer data);
void on_login_clicked                       (GtkWidget* widget,
                                             gpointer data);
void on_cancel_clicked                      (GtkWidget* widget,
                                             gpointer data);
void on_promt_activate                      (GtkWidget* widget,
                                             gpointer data);
void on_user_selection_changed              (GtkWidget* widget,
                                             gpointer data);
gboolean on_arrows_press                    (GtkWidget* widget,
                                             GdkEventKey* event,
                                             gpointer data);
gboolean on_login_window_key_press          (GtkWidget* widget,
                                             GdkEventKey* event,
                                             gpointer data);
void on_show_menu                           (GtkWidget* widget,
                                             GtkWidget* menu);

/* ------------------------------------------------------------------------- *
 * Definition: main
 * ------------------------------------------------------------------------- */

int main(int argc, char** argv)
{
    signal(SIGTERM, sigterm_callback);

    g_message("Another GTK+ Greeter version %s", PACKAGE_VERSION);

    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    textdomain(GETTEXT_PACKAGE);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    gtk_init(&argc, &argv);

    load_settings();

    gboolean inited = connect_to_lightdm();
    if(inited)
        init_css();
    inited &= init_gui();

    if(inited)
    {
        init_power_indicator();
        init_a11y_indicator();
        init_clock_indicator();
        init_layout_indicator();

        init_user_selection();
        run_gui();
    }
    else
    {
        g_critical("Greeter initialization failed");
    }

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

    GError* error = NULL;
    if(!lightdm_greeter_connect_sync(greeter.greeter, &error))
    {
        #ifndef _DEBUG_
        if(error)
        {
            g_critical("Connection to LightDM failed with error: %s", error->message);
            show_error(_("Error"), _("Connection to LightDM failed with error: %s"), error->message);
            g_clear_error(&error);
        }
        else
        {
            g_critical("Connection to LightDM failed");
            show_error(_("Error"), _("Connection to LightDM failed"));
        }
        return FALSE;
        #endif
    }

    g_signal_connect(greeter.greeter, "show-prompt", G_CALLBACK(on_show_prompt), NULL);
    g_signal_connect(greeter.greeter, "show-message", G_CALLBACK(on_show_message), NULL);
    g_signal_connect(greeter.greeter, "authentication-complete", G_CALLBACK(on_authentication_complete), NULL);
    g_signal_connect(greeter.greeter, "autologin-timer-expired", G_CALLBACK(on_autologin_timer_expired), NULL);
    g_signal_connect(lightdm_user_list_get_instance(), "user-added", G_CALLBACK(on_user_added), NULL);
    g_signal_connect(lightdm_user_list_get_instance(), "user-changed", G_CALLBACK(on_user_changed), NULL);
    g_signal_connect(lightdm_user_list_get_instance(), "user-removed", G_CALLBACK(on_user_removed), NULL);

    return TRUE;
}

static gboolean init_css(void)
{
    if(!config.appearance.css_file)
    {
        g_message("No CSS file defined");
        return TRUE;
    }

    gchar* css_file = NULL;
    if(g_path_is_absolute(config.appearance.css_file))
        css_file = g_strdup(config.appearance.css_file);
    else
        css_file = g_build_filename(GREETER_DATA_DIR, config.appearance.css_file, NULL);

    g_message("Loading CSS file: %s", css_file);

    if(!g_file_test(css_file, G_FILE_TEST_EXISTS))
    {
        g_warning("Error loading CSS: file not exists");
        g_free(css_file);
        return FALSE;
    }

    GError* error = NULL;
    GtkCssProvider* provider = gtk_css_provider_new();
    GdkScreen* screen = gdk_display_get_default_screen(gdk_display_get_default());

    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gboolean loaded = gtk_css_provider_load_from_path(provider, css_file, &error);

    if(!loaded)
    {
        g_warning("Error loading CSS: %s", error->message);
        g_clear_error(&error);
    }
    g_object_unref(provider);
    g_free(css_file);
    return loaded;
}

static gboolean init_gui(void)
{
    g_message("Creating GUI");
    GError* error = NULL;
    GtkBuilder* builder = gtk_builder_new();

    gchar* ui_file = NULL;
    if(g_path_is_absolute(config.appearance.ui_file))
        ui_file = g_strdup(config.appearance.ui_file);
    else
        ui_file = g_build_filename(GREETER_DATA_DIR, config.appearance.ui_file, NULL);

    g_message("Loading UI file: %s", ui_file);
    if(!gtk_builder_add_from_file(builder, ui_file, &error))
    {
        g_critical("Error loading UI file: %s", error->message);
        show_error(_("Error"), _("Error loading UI file:\n\n%s"), error->message);
        g_free(ui_file);
        g_clear_error(&error);
        return FALSE;
    }
    g_free(ui_file);

    gdk_window_set_cursor(gdk_get_default_root_window(), gdk_cursor_new(GDK_LEFT_PTR));

    struct BuilderWidget
    {
        GtkWidget** pwidget;
        const gchar* name;
        gboolean needed;
    };

    const struct BuilderWidget WIDGETS[] =
    {
        {&greeter.ui.login_window,           "login_window",            TRUE},
        {&greeter.ui.panel_window,           "panel_window",            FALSE},
        {&greeter.ui.login_box,              "login_box",               FALSE},
        {&greeter.ui.login_widget,           "login_widget",            FALSE},
        {&greeter.ui.cancel_widget,          "cancel_widget",           FALSE},
        {&greeter.ui.prompt_box,             "prompt_box",              FALSE},
        {&greeter.ui.prompt_widget,          "prompt_widget",           FALSE},
        {&greeter.ui.prompt_entry,           "prompt_entry",            TRUE},
        {&greeter.ui.message_widget,         "message_widget",          FALSE},
        {&greeter.ui.user_image,             "user_image",              FALSE},
        {&greeter.ui.date_widget,            "date_widget",             FALSE},

        {&greeter.ui.panel.menubar,          "menubar",                 FALSE},
        {&greeter.ui.power.power_widget,     "power_widget",            FALSE},
        {&greeter.ui.power.power_menu,       "power_menu",              FALSE},
        {&greeter.ui.power.power_menu_icon,  "power_menu_icon",         FALSE},
        {&greeter.ui.power.suspend_widget,   "power_suspend_widget",    FALSE},
        {&greeter.ui.power.hibernate_widget, "power_hibernate_widget",  FALSE},
        {&greeter.ui.power.restart_widget,   "power_restart_widget",    FALSE},
        {&greeter.ui.power.shutdown_widget,  "power_shutdown_widget",   FALSE},
        {&greeter.ui.a11y.a11y_widget,       "a11y_widget",             FALSE},
        {&greeter.ui.a11y.a11y_menu,         "a11y_menu",               FALSE},
        {&greeter.ui.a11y.a11y_menu_icon,    "a11y_menu_icon",          FALSE},
        {&greeter.ui.a11y.osk_widget,        "a11y_osk_widget",         FALSE},
        {&greeter.ui.a11y.contrast_widget,   "a11y_contrast_widget",    FALSE},
        {&greeter.ui.a11y.font_widget,       "a11y_font_widget",        FALSE},
        {&greeter.ui.clock.time_widget,      "time_widget",             FALSE},
        {&greeter.ui.clock.time_menu,        "time_menu",               FALSE},
        {&greeter.ui.layout.layout_widget,   "layout_widget",           FALSE},
        {&greeter.ui.layout.layout_menu,     "layout_menu",             FALSE},

        {&greeter.ui.user_view,              "user_view",               TRUE},
        {&greeter.ui.session_view,           "session_view",            FALSE},
        {&greeter.ui.language_view,          "language_view",           FALSE},
        {&greeter.ui.user_view_box,          "user_view_box",           FALSE},

        {&greeter.ui.host_widget,            "host_widget",             FALSE},
        {&greeter.ui.logo_image,             "logo_image",              FALSE},

        {NULL, NULL, FALSE}
    };

    for(const struct BuilderWidget* w = WIDGETS; w->pwidget != NULL; ++w)
    {
        *w->pwidget = GTK_WIDGET(gtk_builder_get_object(builder, w->name));
        if(w->needed && *w->pwidget == NULL)
        {
            g_critical("Widget is not found: %s\n", w->name);
            show_error(_("Loading UI: error"), _("Widget is not found: %s\n"), w->name);
            return FALSE;
        }
        else
            g_debug("Widget is not found: %s\n", w->name);
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

    greeter.state.default_user_image = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                                                DEFAULT_USER_ICON,
                                                                config.appearance.default_user_image_size,
                                                                0, &error);
    greeter.state.default_user_image_scaled = greeter.state.default_user_image;
    if(error)
    {
        g_warning("Failed to load default avatar: %s", error->message);
        g_clear_error(&error);
    }
    else
        greeter.state.default_user_image_scaled = scale_image(greeter.state.default_user_image,
                                                              config.appearance.list_view_image_size);

    /* Login window */

    if(!load_sessions_list())
        gtk_widget_hide(greeter.ui.session_view);

    if(!load_languages_list() || !config.greeter.show_language_selector)
        gtk_widget_hide(greeter.ui.language_view);

    if(!load_users_list())
        return FALSE;

    if(greeter.ui.login_widget && config.appearance.fixed_login_button_width &&
       GTK_IS_BIN(greeter.ui.login_widget) &&
       GTK_IS_LABEL(gtk_bin_get_child(GTK_BIN(greeter.ui.login_widget))))
    {
        int a = strlen(_("Login")), b = strlen(_("Unlock"));
        greeter.ui.login_widget_label = gtk_bin_get_child(GTK_BIN(greeter.ui.login_widget));
        gtk_label_set_width_chars(GTK_LABEL(greeter.ui.login_widget_label),
                                  a > b ? a : b);
    }

    set_logo_image();
    set_widget_text(greeter.ui.host_widget, lightdm_get_hostname());

    gtk_builder_connect_signals(builder, greeter.greeter);

    return TRUE;
}

static void set_background(const char* value)
{
    g_debug("Background: %s", value);
    if(g_strcmp0(value, greeter.state.last_background) == 0)
        return;

    GdkPixbuf* background_pixbuf = NULL;
    GdkRGBA background_color;

    if(!gdk_rgba_parse(&background_color, value))
    {
        GError* error = NULL;
        gchar* path;
        if(g_path_is_absolute(value))
            path = g_strdup(value);
        else
            path = g_build_filename(GREETER_DATA_DIR, value, NULL);

        g_debug("Loading background: %s", path);

        background_pixbuf = gdk_pixbuf_new_from_file(path, &error);
        g_clear_error(&error);
        g_free(path);

        if(!background_pixbuf)
        {
            g_warning("Failed to load background: %s", error->message);
            return;
        }
    }
    else
        g_debug("Using background color: %s", value);

    GdkRectangle monitor_geometry;
    for(int i = 0; i < gdk_display_get_n_screens(gdk_display_get_default()); ++i)
    {
        GdkScreen* screen = gdk_display_get_screen(gdk_display_get_default(), i);
        cairo_surface_t* surface = create_root_surface(screen);
        cairo_t* c = cairo_create(surface);

        for(int monitor = 0; monitor < gdk_screen_get_n_monitors(screen); monitor++)
        {
            gdk_screen_get_monitor_geometry(screen, monitor, &monitor_geometry);

            if(background_pixbuf)
            {
                GdkPixbuf* pixbuf = gdk_pixbuf_scale_simple(background_pixbuf, monitor_geometry.width, monitor_geometry.height, GDK_INTERP_BILINEAR);
                if(!gdk_pixbuf_get_has_alpha(pixbuf))
                {
                    GdkPixbuf* p = gdk_pixbuf_add_alpha (pixbuf, FALSE, 255, 255, 255);
                    g_object_unref(pixbuf);
                    pixbuf = p;
                }
                gdk_cairo_set_source_pixbuf(c, pixbuf, monitor_geometry.x, monitor_geometry.y);
            }
            else
                gdk_cairo_set_source_rgba(c, &background_color);
            cairo_paint(c);
        }
        cairo_destroy(c);
        /* Refresh background */
        gdk_flush();
        XClearWindow(GDK_SCREEN_XDISPLAY(screen), RootWindow(GDK_SCREEN_XDISPLAY(screen), i));
    }

    if(background_pixbuf)
        g_object_unref(background_pixbuf);
}

static void run_gui(void)
{
    if(config.appearance.background && !config.appearance.user_background)
        set_background(config.appearance.background);

    if(config.appearance.fixed_user_image_size && greeter.ui.user_image)
        update_minimal_user_image_size();

    if(greeter.ui.date_widget)
    {
        update_date_label(NULL);
        g_timeout_add_seconds(1, (GSourceFunc)update_date_label, NULL);
    }

    if(lightdm_greeter_get_hide_users_hint(greeter.greeter) && greeter.ui.user_view_box)
        gtk_widget_hide(greeter.ui.user_view_box);

    greeter.state.panel.position = config.panel.position == PANEL_POS_TOP ? WINDOW_POSITION_TOP : WINDOW_POSITION_BOTTOM;

    gtk_widget_show(greeter.ui.login_window);
    gtk_widget_show(greeter.ui.panel_window);

    update_windows_layout();

    gdk_window_focus(gtk_widget_get_window(greeter.ui.login_window), GDK_CURRENT_TIME);
    gtk_main();
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
    gint* value = g_hash_table_lookup(greeter.state.users_display_names, display_name);
    return value ? *value : 0;
}

static void append_user(GtkTreeModel* model,
                        LightDMUser* user,
                        gboolean update_hash_table)
{
    const gchar* base_display_name = lightdm_user_get_display_name(user);
    const gchar* user_name = lightdm_user_get_name(user);
    const gchar* image_file = lightdm_user_get_image(user);

    g_debug("Adding user: %s (%s)", base_display_name, user_name);

    gint same_name_count = 0;
    if(update_hash_table)
        same_name_count = update_users_names_table(base_display_name);
    else
        same_name_count = get_same_name_count(base_display_name);

    gchar* display_name = NULL;
    if(config.appearance.user_name_format == USER_NAME_FORMAT_NAME)
        display_name = g_strdup(user_name);
    else if(config.appearance.user_name_format == USER_NAME_FORMAT_BOTH || same_name_count > 1)
        display_name = g_strdup_printf("%s (%s)", base_display_name, user_name);
    else
        display_name = g_strdup(base_display_name);

    GdkPixbuf* image = greeter.state.default_user_image;
    if(image_file)
    {
        GError* error = NULL;
        image = gdk_pixbuf_new_from_file(image_file, &error);
        if(error)
        {
            g_warning("Failed to load user image (%s): %s", user_name, error->message);
            g_clear_error(&error);
            image = greeter.state.default_user_image;
        }
    }

    GtkTreeIter iter;
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       USER_COLUMN_NAME, user_name,
                       USER_COLUMN_TYPE, USER_TYPE_REGULAR,
                       USER_COLUMN_DISPLAY_NAME, display_name,
                       USER_COLUMN_WEIGHT, lightdm_user_get_logged_in(user) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
                       USER_COLUMN_IMAGE, image,
                       USER_COLUMN_IMAGE_SCALED, scale_image(image, config.appearance.list_view_image_size),
                       -1);
}

static void append_custom_user(GtkTreeModel* model,
                               gint type,
                               const gchar* name,
                               const gchar* display_name)
{
    g_debug("Adding not real user: %s (%s)", display_name, name);

    GtkTreeIter iter;
    gtk_list_store_append(GTK_LIST_STORE(model), &iter);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       USER_COLUMN_NAME, name,
                       USER_COLUMN_TYPE, type,
                       USER_COLUMN_DISPLAY_NAME, display_name,
                       USER_COLUMN_WEIGHT, PANGO_WEIGHT_NORMAL,
                       USER_COLUMN_IMAGE, greeter.state.default_user_image,
                       -1);
}

static gboolean load_users_list(void)
{
    g_message("Reading users list");

    GtkTreeIter iter;
    GtkTreeModel* model = get_widget_model(greeter.ui.user_view);
    const GList* items = lightdm_user_list_get_users(lightdm_user_list_get_instance());
    const GList* item;

    if(!items)
        g_warning("lightdm_user_list_get_users() return NULL");

    if(!greeter.state.users_display_names)
        greeter.state.users_display_names = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);
    else
    {
       g_debug("Creating names table");
       g_hash_table_remove_all(greeter.state.users_display_names);
       gtk_list_store_clear(GTK_LIST_STORE(model));
    }

    for(item = items; item != NULL; item = item->next)
        update_users_names_table(lightdm_user_get_display_name(item->data));

    for(item = items; item != NULL; item = item->next)
        append_user(model, item->data, FALSE);

    if(lightdm_greeter_get_has_guest_account_hint(greeter.greeter))
        append_custom_user(model, USER_TYPE_GUEST, USER_GUEST, _("Guest Account"));

    if(config.greeter.allow_other_users)
        append_custom_user(model, USER_TYPE_OTHER, USER_OTHER, _("Other..."));

    if(!gtk_tree_model_get_iter_first(model, &iter))
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
    if(!images_cache)
        images_cache = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);

    GdkPixbuf* pixbuf = g_hash_table_lookup(images_cache, session);
    if(pixbuf)
        return pixbuf;

    gchar* image_name = g_strdup_printf("%s.png", session);
    gchar* image_path = g_build_filename(GREETER_DATA_DIR, image_name, NULL);

    pixbuf = gdk_pixbuf_new_from_file(image_path, NULL);

    g_free(image_name);
    g_free(image_path);

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

static gboolean load_sessions_list(void)
{
    g_message("Reading sessions list");

    const GList* items = lightdm_get_sessions();

    if(!items)
    {
        g_warning("lightdm_get_sessions() return NULL");
        return FALSE;
    }

    GtkTreeIter iter;
    GtkTreeModel* model = get_widget_model(greeter.ui.session_view);

    for(const GList* item = items; item != NULL; item = item->next)
    {
        LightDMSession* session = item->data;
        const gchar* key = lightdm_session_get_key(session);

        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           SESSION_COLUMN_NAME, key,
                           SESSION_COLUMN_DISPLAY_NAME, lightdm_session_get_name(session),
                           -1);
        if(config.greeter.show_session_icon)
        {
            GdkPixbuf* pixbuf = get_session_image(key, TRUE);
            if(pixbuf)
                gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                                   SESSION_COLUMN_IMAGE, pixbuf,
                                   -1);
        }
    }
    return TRUE;
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
    GtkTreeModel* model = get_widget_model(greeter.ui.language_view);

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

        gtk_list_store_append(GTK_LIST_STORE(model), &iter);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           LANGUAGE_COLUMN_CODE, code,
                           LANGUAGE_COLUMN_DISPLAY_NAME, label,
                           -1);
        g_free(label);
    }
    return TRUE;
}

static gboolean get_first_logged_user(GtkTreeModel* model,
                                      GtkTreeIter* iter)
{
    if(!gtk_tree_model_get_iter_first(model, iter))
        return FALSE;
    do
    {
        gchar* user_name = NULL;
        gtk_tree_model_get(model, iter, USER_COLUMN_NAME, &user_name, -1);
        LightDMUser* user = user_name ? lightdm_user_list_get_user_by_name(lightdm_user_list_get_instance(), user_name) : NULL;
        if(user && lightdm_user_get_logged_in(user))
        {
            g_free(user_name);
            return TRUE;
        }
        g_free(user_name);
    } while(gtk_tree_model_iter_next(model, iter));

    return FALSE;
}

static void init_user_selection(void)
{
    g_debug("init_user_selection()");

    gchar* last_logged_user = NULL;
    const gchar* selected_user = NULL;
    if(lightdm_greeter_get_select_user_hint(greeter.greeter))
        selected_user = lightdm_greeter_get_select_user_hint(greeter.greeter);
    else if(lightdm_greeter_get_select_guest_hint(greeter.greeter))
        selected_user = USER_GUEST;
    else
        selected_user = last_logged_user = get_last_logged_user();

    GtkTreeIter iter;
    GtkTreeModel* model = get_widget_model(greeter.ui.user_view);

    if(selected_user && get_model_iter_str(model, USER_COLUMN_NAME, selected_user, &iter))
        set_widget_active_iter(greeter.ui.user_view, &iter);
    else if(get_first_logged_user(model, &iter))
        set_widget_active_iter(greeter.ui.user_view, &iter);
    else
        set_widget_active_first(greeter.ui.user_view);
    g_free(last_logged_user);
}

static void load_user_options(LightDMUser* user)
{
    set_session(user ? lightdm_user_get_session(user) : NULL);
    set_language(user ? lightdm_user_get_language(user) : NULL);

    if(config.appearance.user_background)
    {
        const gchar* bg = user ? lightdm_user_get_background(user) : config.appearance.background;
        set_background(bg ? bg : config.appearance.background);
    }
}

static void set_prompt_label(const gchar* text)
{
    set_widget_text(greeter.ui.prompt_widget, text);
}

static void set_message_label(const gchar* text)
{
    gtk_widget_set_visible(greeter.ui.message_widget, text != NULL && g_strcmp0(text, "") != 0);
    set_widget_text(greeter.ui.message_widget, text);
}

static void set_login_button_state(LoginButtonState state)
{
    const gchar* text = state == LOGIN_BUTTON_UNLOCK ? _("Unlock") : _("Login");
    GtkWidget* widget = greeter.ui.login_widget_label ? greeter.ui.login_widget_label : greeter.ui.login_widget;
    set_widget_text(widget, text);
}

static void set_logo_image(void)
{
    if(!greeter.ui.logo_image || !config.appearance.logo)
        return;
    if(strlen(config.appearance.logo) == 0)
    {
        g_debug("Setting logo: empty => hiding");
        gtk_widget_hide(greeter.ui.logo_image);
    }
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
                gtk_image_set_pixel_size(GTK_IMAGE(greeter.ui.logo_image), pixel_size);
        }
        else
            icon_name = g_strdup(logo_string);
        if(icon_name)
            gtk_image_set_from_icon_name(GTK_IMAGE(greeter.ui.logo_image), icon_name, GTK_ICON_SIZE_INVALID);
        g_free(icon_name);
    }
    else
    {
        GError* error = NULL;
        gchar* path = NULL;
        if(g_path_is_absolute(config.appearance.logo))
            path = g_strdup(config.appearance.logo);
        else
            path = g_build_filename(GREETER_DATA_DIR, config.appearance.logo, NULL);

        g_debug("Loading logo from file: %s", path);
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(path, &error);
        if(pixbuf)
            gtk_image_set_from_pixbuf(GTK_IMAGE(greeter.ui.logo_image), pixbuf);
        else
            g_warning("Failed to load logo: %s", error->message);
        g_object_unref(pixbuf);
        g_clear_error(&error);
    }
}

static void update_minimal_user_image_size(void)
{
    GtkTreeIter iter;
    GtkTreeModel* model = get_widget_model(greeter.ui.user_view);
    if(gtk_tree_model_get_iter_first(model, &iter))
    {
        int max_w = 0;
        int max_h = 0;
        const GdkPixbuf* image;
        do
        {
            gtk_tree_model_get(model, &iter, USER_COLUMN_IMAGE, &image, -1);
            int w = gdk_pixbuf_get_width(image);
            int h = gdk_pixbuf_get_height(image);
            if(w > max_w)
                max_w = w;
            if(h > max_h)
                max_h = h;
        }
        while(gtk_tree_model_iter_next(model, &iter));

        gtk_widget_set_size_request(greeter.ui.user_image, max_w, max_h);
    }
}

static gboolean update_date_label(gpointer dummy)
{
    GDateTime* datetime = g_date_time_new_now_local();
    if(!datetime)
    {
        set_widget_text(greeter.ui.date_widget, "[date]");
        return TRUE;
    }
    gchar* str = g_date_time_format(datetime, config.appearance.date_format);
    set_widget_text(greeter.ui.date_widget, str);
    g_free(str);
    g_date_time_unref(datetime);
    return FALSE;
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
        show_error(_("Screenshot"), "%s", error->message);
        g_clear_error(&error);
    };
    g_date_time_unref(datetime);
    g_object_unref(screenshot);
    g_free(file_path);
    g_free(file_name);
    g_free(file_dir);
}

static void start_authentication(const gchar* user_name)
{
    g_message("Starting authentication for user \"%s\"", user_name);

    save_last_logged_user(user_name);

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
    set_login_button_state(user && lightdm_user_get_logged_in(user) ? LOGIN_BUTTON_UNLOCK : LOGIN_BUTTON_LOGIN);
    gtk_widget_hide(greeter.ui.cancel_widget);
}

static void cancel_authentication(void)
{
    g_message("Cancelling authentication for current user");

    /* If in authentication then stop that first */
    greeter.state.cancelling = FALSE;
    if(lightdm_greeter_get_in_authentication(greeter.greeter))
    {
        greeter.state.cancelling = TRUE;
        lightdm_greeter_cancel_authentication(greeter.greeter);
        set_message_label(NULL);
    }

    /* Start a new login */
    if(lightdm_greeter_get_hide_users_hint(greeter.greeter))
        start_authentication(USER_OTHER);
    else
    {
        gchar* user_name = get_user();
        start_authentication(user_name);
        g_free(user_name);
    }
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

    gchar* session = get_session();
    if(lightdm_greeter_start_session_sync(greeter.greeter, session, NULL))
    {
        a11y_osk_kill();
    }
    else
    {
        set_message_label(_("Failed to start session"));
        start_authentication(lightdm_greeter_get_authentication_user(greeter.greeter));
    }
    g_free(session);
}

static gchar* get_user(void)
{
    return get_widget_selection_str(greeter.ui.user_view, USER_COLUMN_NAME, NULL);
}

static UserType get_user_type(void)
{
    return get_widget_selection_int(greeter.ui.user_view, USER_COLUMN_TYPE, USER_TYPE_REGULAR);
}

static gchar* get_session(void)
{
    return get_widget_selection_str(greeter.ui.session_view,
                                    SESSION_COLUMN_NAME,
                                    lightdm_greeter_get_default_session_hint(greeter.greeter));
}

static void set_session(const gchar* session)
{
    GtkTreeIter iter;
    if(session && get_model_iter_str(get_widget_model(greeter.ui.session_view),
                                     SESSION_COLUMN_NAME, session, &iter))
    {
        set_widget_active_iter(greeter.ui.session_view, &iter);
        return;
    }

    const gchar* default_session = lightdm_greeter_get_default_session_hint(greeter.greeter);

    if(default_session && g_strcmp0(session, default_session) != 0)
        set_session(default_session);
    else
        set_widget_active_first(greeter.ui.session_view);
}

static gchar* get_language(void)
{
    return get_widget_selection_str(greeter.ui.language_view, LANGUAGE_COLUMN_CODE, NULL);
}

static void set_language(const gchar* language)
{
    GtkTreeIter iter;
    if(language && get_model_iter_str(get_widget_model(greeter.ui.language_view),
                                      LANGUAGE_COLUMN_CODE, language, &iter))
    {
        set_widget_active_iter(greeter.ui.language_view, &iter);
        return;
    }

    /* If failed to find this language, then try the default */
    const gchar* default_language = NULL;
    if(lightdm_get_language())
        default_language = lightdm_language_get_code(lightdm_get_language());
    if(default_language && g_strcmp0(default_language, language) != 0)
        set_language(default_language);
    else
        set_widget_active_first(greeter.ui.language_view);
}

static cairo_surface_t* create_root_surface(GdkScreen* screen)
{
    gint number, width, height;
    Display* display;
    Pixmap pixmap;
    cairo_surface_t* surface;

    number = gdk_screen_get_number(screen);
    width = gdk_screen_get_width(screen);
    height = gdk_screen_get_height(screen);

    /* Open a new connection so with Retain Permanent so the pixmap remains when the greeter quits*/
    gdk_flush();
    display = XOpenDisplay(gdk_display_get_name(gdk_screen_get_display(screen)));
    if(!display)
    {
        g_warning("Failed to create root pixmap");
        return NULL;
    }
    XSetCloseDownMode(display, RetainPermanent);
    pixmap = XCreatePixmap(display, RootWindow(display, number), width, height, DefaultDepth(display, number));
    XCloseDisplay(display);

    /* Convert into a Cairo surface*/
    surface = cairo_xlib_surface_create(GDK_SCREEN_XDISPLAY(screen),
                                        pixmap,
                                        GDK_VISUAL_XVISUAL(gdk_screen_get_system_visual(screen)),
                                        width, height);
    /* Use this pixmap for the background*/
    XSetWindowBackgroundPixmap(GDK_SCREEN_XDISPLAY(screen),
                               RootWindow(GDK_SCREEN_XDISPLAY(screen), number),
                               cairo_xlib_surface_get_drawable(surface));


    return surface;
}

/* ------------------------------------------------------------------------- *
 * Definitions: callbacks
 * ------------------------------------------------------------------------- */

static void sigterm_callback(int signum)
{
    exit(EXIT_SUCCESS);
}

/* ------------------------------------------------------------------------- *
 * Definitions: LightDM callbacks
 * ------------------------------------------------------------------------- */

static void on_show_prompt(LightDMGreeter* greeter_ptr,
                           const gchar* text,
                           LightDMPromptType type)
{
    g_debug("LightDM signal: show-prompt (%s)", text);

    greeter.state.prompted = TRUE;

    set_prompt_label(dgettext("Linux-PAM", text));
    gtk_entry_set_text(GTK_ENTRY(greeter.ui.prompt_entry), "");
    gtk_entry_set_visibility(GTK_ENTRY(greeter.ui.prompt_entry), type != LIGHTDM_PROMPT_TYPE_SECRET);
    gtk_widget_set_sensitive(greeter.ui.prompt_entry, TRUE);
    gtk_widget_set_sensitive(greeter.ui.login_widget, TRUE);
    gtk_widget_show(greeter.ui.prompt_box);
    gtk_widget_show(greeter.ui.login_box);
    gtk_widget_grab_focus(greeter.ui.prompt_entry);
}

static void on_show_message(LightDMGreeter* greeter_ptr,
                            const gchar* text,
                            LightDMMessageType type)
{
    g_debug("LightDM signal: show-message(%d: %s)", type, text);
    set_message_label(text);
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

    gtk_widget_hide(greeter.ui.prompt_box);
    gtk_widget_show(greeter.ui.login_box);

    if(lightdm_greeter_get_is_authenticated(greeter.greeter))
    {
        if(greeter.state.prompted)
            start_session();
    }
    else
    {
        if(greeter.state.prompted)
        {
            set_message_label(_("Incorrect password, please try again"));
            lightdm_greeter_authenticate(greeter.greeter, lightdm_greeter_get_authentication_user(greeter.greeter));
        }
        else
        {
            set_message_label(_("Failed to authenticate"));
            gchar* user_name = get_user();
            start_authentication(user_name);
            g_free(user_name);
        }
    }
}

static void on_autologin_timer_expired(LightDMGreeter* greeter_ptr)
{
    g_debug("LightDM signal: autologin-timer-expired");
    lightdm_greeter_authenticate_autologin(greeter_ptr);
    return;
    if(lightdm_greeter_get_autologin_guest_hint(greeter.greeter))
        start_authentication(USER_GUEST);
    else if(lightdm_greeter_get_autologin_user_hint(greeter.greeter))
        start_authentication(lightdm_greeter_get_autologin_user_hint(greeter.greeter));
}

static void on_user_added(LightDMUserList* user_list,
                          LightDMUser* user)
{
    g_debug("LightDM signal: user-added");
    append_user(get_widget_model(greeter.ui.user_view), user, TRUE);
}

static void on_user_changed(LightDMUserList* user_list,
                            LightDMUser* user)
{
    g_debug("LightDM signal: user-changed");
    GtkTreeIter iter;
    GtkTreeModel* model = get_widget_model(greeter.ui.user_view);
    const gchar* name = lightdm_user_get_name(user);
    if(!get_model_iter_str(model, USER_COLUMN_NAME, name, &iter))
        return;

    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
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
    GtkTreeModel* model = get_widget_model(greeter.ui.user_view);
    const gchar* name = lightdm_user_get_name(user);
    if(!get_model_iter_str(model, USER_COLUMN_NAME, name, &iter))
        return;
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
}

/* ------------------------------------------------------------------------- *
 * Definitions: GUI callbacks
 * ------------------------------------------------------------------------- */

G_MODULE_EXPORT void on_center_window(GtkWidget* widget,
                                      gpointer data)
{
    set_window_position(widget, &config.greeter.position);
}

G_MODULE_EXPORT void on_login_clicked(GtkWidget* widget,
                                      gpointer data)
{
    if(lightdm_greeter_get_is_authenticated(greeter.greeter))
        start_session();
    else if(lightdm_greeter_get_in_authentication(greeter.greeter))
    {
        const gchar* text = gtk_entry_get_text(GTK_ENTRY(greeter.ui.prompt_entry));
        lightdm_greeter_respond(greeter.greeter, text);
        gtk_widget_show(greeter.ui.cancel_widget);
        if(get_user_type() == USER_TYPE_OTHER && gtk_entry_get_visibility(GTK_ENTRY(greeter.ui.prompt_entry)))
        {
            LightDMUser* user = lightdm_user_list_get_user_by_name(lightdm_user_list_get_instance(), text);
            set_login_button_state(user && lightdm_user_get_logged_in(user) ? LOGIN_BUTTON_UNLOCK : LOGIN_BUTTON_LOGIN);
            load_user_options(user);
        }
    }
    else
    {
        g_warning("Authentication state is undeterminated");
        return;
    }
    gtk_widget_set_sensitive(greeter.ui.prompt_entry, FALSE);
    gtk_widget_set_sensitive(greeter.ui.login_widget, FALSE);
    set_message_label(NULL);
}

G_MODULE_EXPORT void on_cancel_clicked(GtkWidget* widget,
                                       gpointer data)
{
    cancel_authentication();
}

G_MODULE_EXPORT void on_promt_activate(GtkWidget* widget,
                                       gpointer data)
{
    on_login_clicked(widget, NULL);
}

G_MODULE_EXPORT void on_user_selection_changed(GtkWidget* widget,
                                               gpointer data)
{
    gchar* user_name = get_user();
    g_debug("User selection changed: %s", user_name);

    LightDMUser* user = lightdm_user_list_get_user_by_name(lightdm_user_list_get_instance(), user_name);
    gboolean logged_in = user && lightdm_user_get_logged_in(user);

    gtk_widget_set_sensitive(greeter.ui.session_view, !logged_in);
    gtk_widget_set_sensitive(greeter.ui.language_view, !logged_in);

    if(greeter.ui.user_image)
    {
        GdkPixbuf* pixbuf = get_widget_selection_image(greeter.ui.user_view, USER_COLUMN_IMAGE, NULL);
        gtk_image_set_from_pixbuf(GTK_IMAGE(greeter.ui.user_image), pixbuf);
        gtk_widget_set_visible(greeter.ui.user_image, pixbuf != NULL);
    }

    set_message_label(NULL);

    start_authentication(user_name);
    grab_widget_focus(gtk_widget_get_visible(greeter.ui.prompt_entry) ? greeter.ui.prompt_entry : greeter.ui.login_widget);

    g_free(user_name);
}

G_MODULE_EXPORT gboolean on_arrows_press(GtkWidget* widget,
                                         GdkEventKey* event,
                                         gpointer data)
{
    if(event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_Down)
    {
        GtkTreeIter iter;
        if(!get_widget_active_iter(greeter.ui.user_view, &iter))
            return FALSE;
        gboolean has_next;
        if(event->keyval == GDK_KEY_Up)
            has_next = gtk_tree_model_iter_previous(get_widget_model(greeter.ui.user_view), &iter);
        else
            has_next = gtk_tree_model_iter_next(get_widget_model(greeter.ui.user_view), &iter);
        if(has_next)
            set_widget_active_iter(greeter.ui.user_view, &iter);
        return TRUE;
    }
    return FALSE;
}

G_MODULE_EXPORT gboolean on_login_window_key_press(GtkWidget* widget,
                                                   GdkEventKey* event,
                                                   gpointer data)
{
    switch(event->keyval)
    {
        case GDK_KEY_F10:
            if(greeter.ui.panel.menubar)
                gtk_menu_shell_select_first(GTK_MENU_SHELL(greeter.ui.panel.menubar), FALSE);
            else
                gtk_widget_grab_focus(greeter.ui.panel_window);
            break;
        case GDK_KEY_Escape:
            cancel_authentication();
            break;
        case GDK_KEY_F1:
            a11y_toggle_osk();
            break;
        case GDK_KEY_F2:
            a11y_toggle_font();
            break;
        case GDK_KEY_F3:
            a11y_toggle_contrast();
            break;
        case GDK_KEY_Print:
            take_screenshot();
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

G_MODULE_EXPORT void on_show_menu(GtkWidget* widget,
                                  GtkWidget* menu)
{
    if(gtk_widget_get_visible(menu))
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);
}
