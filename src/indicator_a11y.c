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

//#define A11Y_CHECK_THEMES

/* Types */

struct OSKInfo
{
    gboolean (*check)();
    void (*open)();
    void (*close)();
    void (*kill)();
}* OSK = NULL;

/* Exported functions */

G_MODULE_EXPORT void on_a11y_font_toggled(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_a11y_contrast_toggled(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_a11y_osk_toggled(GtkWidget* widget, gpointer data);

/* Static functions */

static gboolean check_program(const gchar* name);
#ifdef A11Y_CHECK_THEMES
static gboolean check_theme_installed(const gchar* theme);
#endif

static gboolean osk_check_custom();
static void osk_open_custom();
static void osk_close_custom();
static void osk_kill_custom();

static gboolean osk_check_onboard();
static void osk_open_onboard();
static void osk_close_onboard();
static void osk_kill_onboard();

/* Static variables */

static GPid keyboard_pid = 0;

static struct
{
    GPid pid;
    GtkWindow* window;
} onboard;

static struct OSKInfo OSKInfoCustom =
{
    osk_check_custom, osk_open_custom, osk_close_custom, osk_kill_custom
};

static struct OSKInfo OSKInfoOnboard =
{
    osk_check_onboard, osk_open_onboard, osk_close_onboard, osk_kill_onboard
};

/* ------------------------------------------------------------------------- *
 * Definitions: public
 * ------------------------------------------------------------------------- */

void init_a11y_indicator()
{
    gtk_widget_set_visible(greeter.ui.a11y.a11y_widget, config.a11y.enabled);
    if(!config.a11y.enabled)
        return;

    if(greeter.ui.a11y.a11y_menu_icon && GTK_IS_MENU_ITEM(greeter.ui.a11y.a11y_widget))
        replace_container_content(greeter.ui.a11y.a11y_widget, greeter.ui.a11y.a11y_menu_icon);

    if(config.a11y.osk_use_onboard && osk_check_onboard())
        OSK = &OSKInfoOnboard;
    else if(osk_check_custom())
        OSK = &OSKInfoCustom;
    else
        OSK = NULL;

    if(!OSK)
    {
        g_warning("a11y indicator: no virtual keyboard found");
        gtk_widget_hide(greeter.ui.a11y.osk_widget);
    }

    if(!config.a11y.theme_contrast || strlen(config.a11y.theme_contrast) == 0)
        gtk_widget_hide(greeter.ui.a11y.contrast_widget);
    #ifdef A11Y_CHECK_THEMES
    else if(!check_theme_installed(config.a11y.theme_contrast))
    {
        g_warning("a11y indicator: contrast theme is not found (%s)", config.a11y.theme_contrast);
        gtk_widget_hide(greeter.ui.a11y.contrast_widget);
    }
    #endif
}

void a11y_osk_open()
{
    if(OSK) OSK->open();
}

void a11y_osk_close()
{
    if(OSK) OSK->close();
}

void a11y_osk_kill()
{
    if(OSK) OSK->kill();
}

/* ------------------------------------------------------------------------- *
 * Definitions: exported
 * ------------------------------------------------------------------------- */

gboolean center_window_callback(GtkWidget* widget)
{
    center_window(widget);
    return False;
}

G_MODULE_EXPORT void on_a11y_font_toggled(GtkWidget* widget, gpointer data)
{
    gtk_widget_hide(greeter.ui.login_window);

    GtkSettings* settings = gtk_settings_get_default();
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
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
                g_free(tokens[length - 1]);
                tokens[length - 1] = g_strdup_printf("%d", size + config.a11y.font_increment);
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

    gtk_widget_show(greeter.ui.login_window);

    g_idle_add((GSourceFunc)center_window_callback, greeter.ui.login_window);
    //center_window(greeter.ui.login_window);
}

G_MODULE_EXPORT void on_a11y_contrast_toggled(GtkWidget* widget, gpointer data)
{
    gtk_widget_hide(greeter.ui.login_window);

    GtkSettings* settings = gtk_settings_get_default();
    gboolean contrast = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    g_object_set(settings, "gtk-theme-name",
                 contrast ? config.a11y.theme_contrast : config.appearance.theme, NULL);
    g_object_set(settings, "gtk-icon-theme-name",
                 contrast ? config.a11y.icon_theme_contrast : config.appearance.icon_theme, NULL);

    gtk_widget_show(greeter.ui.login_window);
    center_window(greeter.ui.login_window);
}

G_MODULE_EXPORT void on_a11y_osk_toggled(GtkWidget* widget, gpointer data)
{
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
        a11y_osk_open();
    else
        a11y_osk_close();
}

/* ------------------------------------------------------------------------- *
 * Definitions: static
 * ------------------------------------------------------------------------- */

static gboolean check_program(const gchar* name)
{
    gchar* cmd = g_strdup_printf("which %s", name);
    int result = system(cmd);
    g_free(cmd);
    return result == 0;
}

#ifdef A11Y_CHECK_THEMES
static gboolean check_theme_in_dir(const gchar* theme, const gchar* path, gboolean dot)
{
    gboolean found = FALSE;
    gchar* path_to_check = g_build_filename(path,
                                            dot ? ".themes/" : "themes/",
                                            theme,
                                            GTK_MAJOR_VERSION == 3 ? "gtk-3.0" : "gtk-2.0", NULL);
    if(g_file_test(path_to_check, G_FILE_TEST_IS_DIR))
        found = TRUE;
    g_free(path_to_check);
    return found;
}

static gboolean check_theme_installed(const gchar* theme)
{
    for(const gchar* const* path = g_get_system_data_dirs(); *path; ++path)
        if(check_theme_in_dir(theme, *path, FALSE))
            return TRUE;

    return check_theme_in_dir(theme, g_get_user_data_dir(), FALSE) ||
           check_theme_in_dir(theme, g_get_home_dir(), TRUE);
    exit(0);
}
#endif

static gboolean osk_check_custom()
{
    return config.a11y.osk_command_array && check_program(config.a11y.osk_command_array[0]);
}

static void osk_open_custom()
{
    if(!config.a11y.osk_command_array)
    {
        g_debug("On-screen keyboard command is undefined");
        return;
    }
    g_debug("Opening on-screen keyboard");
    osk_close_custom();

    GError* error = NULL;
    if(!g_spawn_async(NULL, config.a11y.osk_command_array, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &keyboard_pid, &error))
    {
        g_debug("On-screen keyboard command error: \"%s\"", error ? error->message : "unknown");
        keyboard_pid = 0;
        g_clear_error(&error);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(greeter.ui.a11y.osk_widget), FALSE);
    }
}

static void osk_close_custom()
{
    if(keyboard_pid != 0)
    {
        g_debug("Killing on-screen keyboard");
        kill(keyboard_pid, SIGTERM);
        g_spawn_close_pid(keyboard_pid);
        keyboard_pid = 0;
    }
}

static void osk_kill_custom()
{
    osk_close_custom();
}

static gboolean osk_check_onboard()
{
    return check_program("onboard");
}

static void hide_onboard_window(GtkWidget* widget, gpointer data)
{
    gtk_widget_hide(widget);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(greeter.ui.a11y.osk_widget), FALSE);
}

static gboolean spawn_onboard()
{
    gint argp;
    gchar** argv = NULL;
    if(!g_shell_parse_argv("onboard --xid", &argp, &argv, NULL))
        return FALSE;

    GPid pid = 0;
    GtkWindow* window = NULL;
    GError* error = NULL;

    gint out_fd = 0;
    gboolean spawned = g_spawn_async_with_pipes(NULL,
                                 argv,
                                 NULL,
                                 G_SPAWN_SEARCH_PATH,
                                 NULL,
                                 NULL,
                                 &pid,
                                 NULL, &out_fd, NULL,
                                 &error);
    g_strfreev(argv);

    if(!spawned)
    {
        g_warning("'onboard' command failed: %s", error->message);
        g_clear_error(&error);
        return FALSE;
    }

    gchar* text = NULL;
    GIOChannel* out_channel = g_io_channel_unix_new(out_fd);
    if(out_channel && g_io_channel_read_line(out_channel, &text, NULL, NULL, &error) == G_IO_STATUS_NORMAL)
    {
        GtkSocket* socket = NULL;
        gchar* end_ptr = NULL;

        text = g_strstrip(text);
        gint id = g_ascii_strtoll(text, &end_ptr, 0);

        if(id == 0 || (end_ptr && *end_ptr != '\0'))
        {
            g_critical("Unrecognized output from 'onboard': '%s'", text);
        }
        else if(!(socket = (GtkSocket*)gtk_socket_new()))
        {
            g_critical("Can not connect to 'onboard': %s", error->message);
            g_clear_error(&error);
        }
        else
        {
            g_debug("'onboard' socket: %d", id);

            window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
            g_signal_connect(window, "delete-event", G_CALLBACK(hide_onboard_window), NULL);
            gtk_window_set_accept_focus(window, FALSE);
            gtk_window_set_decorated(window, FALSE);
            gtk_window_set_focus_on_map(window, FALSE);
            gtk_widget_show(GTK_WIDGET(socket));
            gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(socket));
            gtk_socket_add_id(socket, atol(text));

            GdkScreen* screen = gdk_screen_get_default();
            gint monitor = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(greeter.ui.login_window));
            GdkRectangle geometry;
            gdk_screen_get_monitor_geometry(screen, monitor, &geometry);
            gtk_window_move(window, geometry.x, geometry.y + geometry.height - 200);
            gtk_window_resize(window, geometry.width - geometry.x, 200);
        }
    }
    else
    {
        g_critical("Can not read 'onboard' output: %s", error->message);
        g_clear_error(&error);
    }
    g_free(text);
    g_io_channel_unref(out_channel);

    if(window == 0 && pid != 0)
    {
        kill(pid, SIGTERM);
        g_spawn_close_pid(pid);
    }
    else if(window != 0)
    {
        onboard.pid = pid;
        onboard.window = window;
    }

    return window != NULL;
}

static void osk_open_onboard()
{
    if(!onboard.window && !spawn_onboard())
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(greeter.ui.a11y.osk_widget), FALSE);
        return;
    }

    gtk_widget_show(GTK_WIDGET(onboard.window));
}

static void osk_close_onboard()
{
    if(onboard.window)
        gtk_widget_hide(GTK_WIDGET(onboard.window));
}

static void osk_kill_onboard()
{
    if(onboard.pid != 0)
    {
        g_debug("Killing on-screen keyboard");
        kill(onboard.pid, SIGTERM);
        g_spawn_close_pid(onboard.pid);
        gtk_widget_destroy(GTK_WIDGET(onboard.window));
        onboard.pid = 0;
        onboard.window = NULL;
    }
}

