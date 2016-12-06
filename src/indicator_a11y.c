/* indicator_a11y.c
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gtk/gtkx.h>
#include <signal.h>

#include "shares.h"
#include "configuration.h"
#include "indicator_a11y.h"

/* Types */

typedef struct _OnscreenKeyboardInfo
{
    gboolean (*check) (void);
    void (*open) (void);
    void (*close) (void);
    void (*kill) (void);
} OnscreenKeyboardInfo;

/* Exported functions */

void on_a11y_font_toggled                       (GtkWidget* widget,
                                                 gpointer data);
void on_a11y_dpi_toggled                        (GtkWidget* widget,
                                                 gpointer data);
void on_a11y_contrast_toggled                   (GtkWidget* widget,
                                                 gpointer data);
void on_a11y_osk_toggled                        (GtkWidget* widget,
                                                 gpointer data);

/* Static functions */

static gint get_increment                       (gint value,
                                                 gint increment,
                                                 gboolean is_percent);
static gboolean program_is_available            (const gchar* name);

static gboolean osk_check_custom                (void);
static void osk_open_custom                     (void);
static void osk_close_custom                    (void);
static void osk_kill_custom                     (void);

static gboolean osk_check_onboard               (void);
static void osk_open_onboard                    (void);
static void osk_close_onboard                   (void);
static void osk_kill_onboard                    (void);

/* Static variables */

static struct
{
    OnscreenKeyboardInfo* onscreen_keyboard;
    struct
    {
        gboolean font, dpi, contrast, osk;
    } state;
} a11y;

static struct
{
    OnscreenKeyboardInfo info;
    GPid pid;
}
keyboard_command =
{
    .info =
    {
        .check = osk_check_custom,
        .open = osk_open_custom,
        .close = osk_close_custom,
        .kill = osk_kill_custom
    }
};

static struct
{
    OnscreenKeyboardInfo info;
    GPid pid;
    GtkSocket* socket;
}
keyboard_onboard =
{
    .info =
    {
        .check = osk_check_onboard,
        .open = osk_open_onboard,
        .close = osk_close_onboard,
        .kill = osk_kill_onboard
    }
};

/* ------------------------------------------------------------------------- *
 * Definitions: public
 * ------------------------------------------------------------------------- */

void init_a11y_indicator(void)
{
    if(config.a11y.enabled)
    {
        if(config.a11y.osk.enabled)
        {
            if(config.a11y.osk.use_onboard && keyboard_onboard.info.check())
                a11y.onscreen_keyboard = &keyboard_onboard.info;
            else if(keyboard_command.info.check())
                a11y.onscreen_keyboard = &keyboard_command.info;
            else
            {
                g_warning("a11y: no virtual keyboard found");
                config.a11y.osk.enabled = FALSE;
            }
        }

        if(!config.a11y.osk.enabled &&
           !config.a11y.contrast.enabled &&
           !config.a11y.font.enabled &&
           !config.a11y.dpi.enabled)
        {
            g_message("a11y: no options enabled, hiding menu item");
            config.a11y.enabled = FALSE;
        }
    }

    gtk_widget_set_visible(greeter.ui.a11y.widget, config.a11y.enabled);
    gtk_widget_set_visible(greeter.ui.a11y.osk_box, config.a11y.enabled && config.a11y.osk.enabled);
    gtk_widget_set_visible(greeter.ui.a11y.font_box, config.a11y.enabled && config.a11y.font.enabled);
    gtk_widget_set_visible(greeter.ui.a11y.dpi_box, config.a11y.enabled && config.a11y.dpi.enabled);
    gtk_widget_set_visible(greeter.ui.a11y.contrast_box, config.a11y.enabled && config.a11y.contrast.enabled);

    if(config.a11y.enabled)
    {
        if(config.a11y.contrast.enabled && get_state_value_int("a11y", "contrast"))
            a11y_toggle_contrast();
        if(config.a11y.font.enabled && get_state_value_int("a11y", "font"))
            a11y_toggle_font();
        if(config.a11y.dpi.enabled && get_state_value_int("a11y", "dpi"))
            a11y_toggle_dpi();
    }
}

void a11y_close(void)
{
    g_return_if_fail(a11y.onscreen_keyboard != NULL);
    g_return_if_fail(a11y.onscreen_keyboard->kill != NULL);
    a11y.onscreen_keyboard->kill();
}

void a11y_toggle_osk()
{
    g_return_if_fail(config.a11y.osk.enabled);
    g_return_if_fail(a11y.onscreen_keyboard != NULL);

    a11y.state.osk = !a11y.state.osk;
    gtk_widget_hide(greeter.ui.main_content);
    if(greeter.ui.a11y.osk_widget)
        set_widget_toggled(greeter.ui.a11y.osk_widget, a11y.state.osk, G_CALLBACK(on_a11y_osk_toggled));
    if(a11y.state.osk)
        a11y.onscreen_keyboard->open();
    else
        a11y.onscreen_keyboard->close();
    update_main_window_layout();
    gtk_widget_show(greeter.ui.main_content);
}

void a11y_toggle_font(void)
{
    g_return_if_fail(config.a11y.font.enabled);

    a11y.state.font = !a11y.state.font;
    if(greeter.ui.a11y.font_widget)
        set_widget_toggled(greeter.ui.a11y.font_widget, a11y.state.font, G_CALLBACK(on_a11y_font_toggled));

    GtkSettings* settings = gtk_settings_get_default();
    if(a11y.state.font)
    {
        gchar* font_name;
        gchar** tokens;
        guint length;

        g_object_get(settings, "gtk-font-name", &font_name, NULL);
        tokens = g_strsplit(font_name, " ", -1);
        length = g_strv_length(tokens);
        if(length > 1)
        {
            gint size = atoi(tokens[length - 1]);
            if(size > 0)
            {
                if(config.a11y.font.is_percent)
                    size += (int)size*config.a11y.font.increment/100;
                else
                    size += config.a11y.font.increment;
                g_free(tokens[length - 1]);
                tokens[length - 1] = g_strdup_printf("%d", size);
                g_free(font_name);
                font_name = g_strjoinv(" ", tokens);
                g_object_set(settings, "gtk-font-name", font_name, NULL);
            }
        }
        g_strfreev(tokens);
        g_free(font_name);
    }
    else
    {
        g_object_set(settings, "gtk-font-name", config.appearance.font, NULL);
    }
    set_state_value_int("a11y", "font", a11y.state.font);
    update_main_window_layout();
}

void a11y_toggle_dpi(void)
{
    g_return_if_fail(config.a11y.dpi.enabled);

    a11y.state.dpi = !a11y.state.dpi;
    if(greeter.ui.a11y.dpi_widget)
        set_widget_toggled(greeter.ui.a11y.dpi_widget, a11y.state.dpi, G_CALLBACK(on_a11y_dpi_toggled));

    gint value = config.appearance.dpi;
    GtkSettings* settings = gtk_settings_get_default();
    if(a11y.state.dpi)
    {
        gint current;
        g_object_get(settings, "gtk-xft-dpi", &current, NULL);
        value = get_increment(value, config.a11y.dpi.increment, config.a11y.dpi.is_percent);
    }
    g_object_set(settings, "gtk-xft-dpi", value*1024, NULL);
    set_state_value_int("a11y", "dpi", a11y.state.dpi);
}

void a11y_toggle_contrast()
{
    g_return_if_fail(config.a11y.contrast.enabled);

    a11y.state.contrast = !a11y.state.contrast;
    if(greeter.ui.a11y.contrast_widget)
        set_widget_toggled(greeter.ui.a11y.contrast_widget, a11y.state.contrast, G_CALLBACK(on_a11y_contrast_toggled));

    GtkSettings* settings = gtk_settings_get_default();
    apply_gtk_theme(settings, a11y.state.contrast ? config.a11y.contrast.gtk_theme : config.appearance.gtk_theme);
    apply_icon_theme(settings, a11y.state.contrast ? config.a11y.contrast.icon_theme : config.appearance.icon_theme);
    set_state_value_int("a11y", "contrast", a11y.state.contrast);
}

/* ------------------------------------------------------------------------- *
 * Definitions: exported
 * ------------------------------------------------------------------------- */

void on_a11y_font_toggled(GtkWidget* widget,
                          gpointer data)
{
    a11y_toggle_font();
}

void on_a11y_dpi_toggled(GtkWidget* widget,
                         gpointer data)
{
    a11y_toggle_dpi();
}

void on_a11y_contrast_toggled(GtkWidget* widget,
                              gpointer data)
{
    a11y_toggle_contrast();
}

void on_a11y_osk_toggled(GtkWidget* widget,
                         gpointer data)
{
    a11y_toggle_osk();
}

/* ------------------------------------------------------------------------- *
 * Definitions: static
 * ------------------------------------------------------------------------- */

static gint get_increment(gint value,
                          gint increment,
                          gboolean is_percent)
{
    return value + (is_percent ? value*increment/100 : increment);
}

static gboolean program_is_available(const gchar* name)
{
    gchar* cmd = g_strdup_printf("which %s", name);
    int result = system(cmd);
    g_free(cmd);
    return result == 0;
}

static gboolean osk_check_custom(void)
{
    return config.a11y.osk.command && program_is_available(config.a11y.osk.command[0]);
}

static void osk_open_custom(void)
{
    g_message("Opening on-screen keyboard");
    g_return_if_fail(config.a11y.osk.command != NULL);
    osk_close_custom();

    GError* error = NULL;
    if(!g_spawn_async(NULL, config.a11y.osk.command, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &keyboard_command.pid, &error))
    {
        a11y.onscreen_keyboard = NULL;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(greeter.ui.a11y.osk_widget), FALSE);
        g_warning("On-screen keyboard command error: %s", error->message);
        show_message_dialog(GTK_MESSAGE_ERROR, _("On-screen keyboard"),
                            _("Failed to start keyboard command:\n%s"), error->message);
        g_clear_error(&error);
    }
}

static void osk_close_custom(void)
{
    g_message("Killing on-screen keyboard");
    g_return_if_fail(keyboard_command.pid != 0);
    kill(keyboard_command.pid, SIGTERM);
    g_spawn_close_pid(keyboard_command.pid);
    keyboard_command.pid = 0;
}

static void osk_kill_custom(void)
{
    osk_close_custom();
}

static gboolean osk_check_onboard(void)
{
    /* we need widget to place "onboard" in it */
    return greeter.ui.onboard_content && program_is_available("onboard");
}

static gboolean spawn_onboard(void)
{
    gchar* COMMAND_LINE[] = {"onboard", "--xid", NULL};
    GPid pid = 0;
    GtkSocket* socket = NULL;
    GError* error = NULL;
    gint out_fd = 0;

    if(!g_spawn_async_with_pipes(NULL,
                                 COMMAND_LINE,
                                 NULL,
                                 G_SPAWN_SEARCH_PATH,
                                 NULL,
                                 NULL,
                                 &pid,
                                 NULL, &out_fd, NULL,
                                 &error))
    {
        g_warning("\"Onboard\" command failed: %s", error->message);
        g_clear_error(&error);
        return FALSE;
    }

    gchar* text = NULL;
    GIOChannel* out_channel = g_io_channel_unix_new(out_fd);

    g_return_val_if_fail(out_channel != NULL, FALSE);

    if(g_io_channel_read_line(out_channel, &text, NULL, NULL, &error) == G_IO_STATUS_NORMAL)
    {
        gchar* end_ptr = NULL;

        text = g_strstrip(text);
        gint id = g_ascii_strtoll(text, &end_ptr, 0);

        if(id == 0 || (end_ptr && *end_ptr != '\0'))
        {
            g_warning("Unrecognized output from 'onboard': '%s'", text);
        }
        else if(!(socket = GTK_SOCKET(gtk_socket_new())))
        {
            g_warning("Can not connect to 'onboard': %s", error->message);
            g_clear_error(&error);
        }
        else
        {
            g_message("\"Onboard\" socket: %d", id);
            gboolean at_top;
            switch(config.a11y.osk.onboard_position)
            {
            case ONBOARD_POS_TOP: at_top = TRUE; break;
            case ONBOARD_POS_BOTTOM: at_top = FALSE; break;
            case ONBOARD_POS_PANEL: at_top = config.panel.position == PANEL_POS_TOP; break;
            case ONBOARD_POS_PANEL_OPPOSITE: at_top = config.panel.position != PANEL_POS_TOP;
            };

            rearrange_grid_child(GTK_GRID(greeter.ui.screen_layout), greeter.ui.onboard_layout,
                                 at_top ? UI_LAYOUT_ROW_ONBOARD_TOP : UI_LAYOUT_ROW_ONBOARD_BOTTOM);

            if(config.a11y.osk.onboard_height_is_percent)
            {
                GdkRectangle geometry;
                GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(greeter.ui.screen_window));
                gdk_screen_get_monitor_geometry(screen, gdk_screen_get_primary_monitor(screen), &geometry);
                gtk_widget_set_size_request(greeter.ui.onboard_layout,
                                            -1, geometry.height*config.a11y.osk.onboard_height/100);
            }
            else
                gtk_widget_set_size_request(greeter.ui.onboard_content, -1, config.a11y.osk.onboard_height);
            gtk_container_add(GTK_CONTAINER(greeter.ui.onboard_content), GTK_WIDGET(socket));
            gtk_socket_add_id(socket, atol(text));
            gtk_widget_show(GTK_WIDGET(socket));
        }
    }
    else if(error)
        g_warning("Can not read \"Onboard\" output: %s", error->message);

    g_clear_error(&error);
    g_free(text);
    g_io_channel_unref(out_channel);

    if(!socket && pid != 0)
    {
        kill(pid, SIGTERM);
        g_spawn_close_pid(pid);
    }
    else if(socket)
    {
        keyboard_onboard.pid = pid;
        keyboard_onboard.socket = socket;
    }
    return socket != NULL;
}

static void osk_open_onboard(void)
{
    if(!keyboard_onboard.socket && !spawn_onboard())
    {
        show_message_dialog(GTK_MESSAGE_ERROR, _("Onboard"),
                            _("Failed to start 'onboard', see logs for details."));
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(greeter.ui.a11y.osk_widget), FALSE);
        a11y.onscreen_keyboard = NULL;
        return;
    }
    gtk_widget_show(greeter.ui.onboard_layout);
}

static void osk_close_onboard(void)
{
    gtk_widget_hide(greeter.ui.onboard_layout);
}

static void osk_kill_onboard(void)
{
    g_return_if_fail(keyboard_onboard.pid != 0);
    gtk_widget_hide(GTK_WIDGET(greeter.ui.onboard_content));
    gtk_container_remove(GTK_CONTAINER(greeter.ui.onboard_content), gtk_bin_get_child(GTK_BIN(greeter.ui.onboard_content)));
    kill(keyboard_onboard.pid, SIGTERM);
    g_spawn_close_pid(keyboard_onboard.pid);
    keyboard_onboard.pid = 0;
}
