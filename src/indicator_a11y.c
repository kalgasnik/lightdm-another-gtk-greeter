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

/* #define A11Y_CHECK_THEMES */

/* Types */

struct OSKInfo
{
    gboolean (*check) (void);
    void (*open) (void);
    void (*close) (void);
    void (*kill) (void);
}* OSK = NULL;

/* Exported functions */

G_MODULE_EXPORT void on_a11y_font_toggled       (GtkWidget* widget,
                                                 gpointer data);
G_MODULE_EXPORT void on_a11y_contrast_toggled   (GtkWidget* widget,
                                                 gpointer data);
G_MODULE_EXPORT void on_a11y_osk_toggled        (GtkWidget* widget,
                                                 gpointer data);

/* Static functions */

static gboolean check_program                   (const gchar* name);
static gboolean check_theme_installed           (const gchar* theme);

static gboolean update_windows_callback         (gpointer data);
static void update_windows_idle                 (void);

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
    GPid custom;
    GPid onboard;
} osk_pid;

static struct OSKInfo OSKInfoCustom =
{
    .check = osk_check_custom,
    .open = osk_open_custom,
    .close = osk_close_custom,
    .kill = osk_kill_custom
};

static struct OSKInfo OSKInfoOnboard =
{
    .check = osk_check_onboard,
    .open = osk_open_onboard,
    .close = osk_close_onboard,
    .kill = osk_kill_onboard
};

/* ------------------------------------------------------------------------- *
 * Definitions: public
 * ------------------------------------------------------------------------- */

void init_a11y_indicator(void)
{
    if(!config.a11y.enabled)
    {
        gtk_widget_hide(greeter.ui.a11y.a11y_widget);
        return;
    }

    if(greeter.ui.a11y.a11y_menu_icon && GTK_IS_MENU_ITEM(greeter.ui.a11y.a11y_widget))
        replace_container_content(greeter.ui.a11y.a11y_widget, greeter.ui.a11y.a11y_menu_icon);

    if(config.a11y.osk_use_onboard && osk_check_onboard())
        OSK = &OSKInfoOnboard;
    else if(osk_check_custom())
        OSK = &OSKInfoCustom;
    else
    {
        OSK = NULL;
        g_warning("a11y indicator: no virtual keyboard found");
    }

    gboolean allow_contrast = FALSE;
    if(!config.a11y.theme_contrast || strlen(config.a11y.theme_contrast) == 0)
        ;
    else if(config.a11y.check_theme && !check_theme_installed(config.a11y.theme_contrast))
        g_warning("a11y indicator: contrast theme is not found (%s)", config.a11y.theme_contrast);
    else
        allow_contrast = TRUE;

    if(!OSK && !allow_contrast && config.a11y.font_increment <= 0)
    {
        gtk_widget_hide(greeter.ui.a11y.a11y_widget);
        return;
    }

    gtk_widget_set_visible(greeter.ui.a11y.osk_widget, OSK != NULL);
    gtk_widget_set_visible(greeter.ui.a11y.contrast_widget, allow_contrast);
    gtk_widget_set_visible(greeter.ui.a11y.font_widget, config.a11y.font_increment > 0);
}

void a11y_toggle_osk()
{
    g_return_if_fail(GTK_IS_WIDGET(greeter.ui.a11y.osk_widget));
    set_widget_toggled(greeter.ui.a11y.osk_widget, !get_widget_toggled(greeter.ui.a11y.osk_widget));
}

void a11y_toggle_font(void)
{
    g_return_if_fail(GTK_IS_WIDGET(greeter.ui.a11y.font_widget));
    set_widget_toggled(greeter.ui.a11y.font_widget, !get_widget_toggled(greeter.ui.a11y.font_widget));
}

void a11y_toggle_contrast()
{
    g_return_if_fail(GTK_IS_WIDGET(greeter.ui.a11y.contrast_widget));
    set_widget_toggled(greeter.ui.a11y.contrast_widget, !get_widget_toggled(greeter.ui.a11y.contrast_widget));
}

void a11y_osk_open(void)
{
    g_return_if_fail(OSK != NULL);
    OSK->open();
}

void a11y_osk_close(void)
{
    g_return_if_fail(OSK != NULL);
    OSK->close();
}

void a11y_osk_kill(void)
{
    g_return_if_fail(OSK != NULL);
    OSK->kill();
}

/* ------------------------------------------------------------------------- *
 * Definitions: exported
 * ------------------------------------------------------------------------- */

G_MODULE_EXPORT void on_a11y_font_toggled(GtkWidget* widget,
                                          gpointer data)
{
    gboolean big_font = get_widget_toggled(greeter.ui.a11y.font_widget);
    GtkSettings* settings = gtk_settings_get_default();
    if(big_font)
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
                if(config.a11y.font_increment_is_percent)
                    size += (int)size*config.a11y.font_increment/100;
                else
                    size += config.a11y.font_increment;
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
    update_windows_idle();
}

G_MODULE_EXPORT void on_a11y_contrast_toggled(GtkWidget* widget,
                                              gpointer data)
{
    gboolean contrast = get_widget_toggled(widget);
    GtkSettings* settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-theme-name",
                 contrast ? config.a11y.theme_contrast : config.appearance.theme, NULL);
    g_object_set(settings, "gtk-icon-theme-name",
                 contrast ? config.a11y.icon_theme_contrast : config.appearance.icon_theme, NULL);
    update_windows_idle();
}

G_MODULE_EXPORT void on_a11y_osk_toggled(GtkWidget* widget,
                                         gpointer data)
{
    if(get_widget_toggled(greeter.ui.a11y.osk_widget))
        a11y_osk_open();
    else
        a11y_osk_close();
    update_windows_idle();
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

static gboolean check_theme_in_dir(const gchar* theme,
                                   const gchar* path,
                                   gboolean dot)
{
    gboolean found = FALSE;
    gchar* path_to_check = g_build_filename(path, dot ? ".themes/" : "themes/",
                                            theme, GTK_MAJOR_VERSION == 3 ? "gtk-3.0" : "gtk-2.0", NULL);
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
}

static gboolean update_windows_callback(gpointer data)
{
    update_windows_layout();
    return FALSE;
}

static void update_windows_idle(void)
{
    g_idle_add((GSourceFunc)update_windows_callback, NULL);
}

static gboolean osk_check_custom(void)
{
    return config.a11y.osk_command_array && check_program(config.a11y.osk_command_array[0]);
}

static void osk_open_custom(void)
{
    g_return_if_fail(config.a11y.osk_command_array != NULL);
    g_debug("Opening on-screen keyboard");

    osk_close_custom();

    GError* error = NULL;
    if(!g_spawn_async(NULL, config.a11y.osk_command_array, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &osk_pid.custom, &error))
    {
        g_critical("On-screen keyboard command error: \"%s\"", error ? error->message : "unknown");
        osk_pid.custom = 0;
        g_clear_error(&error);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(greeter.ui.a11y.osk_widget), FALSE);
    }
}

static void osk_close_custom(void)
{
    g_return_if_fail(osk_pid.custom != 0);

    g_debug("Killing on-screen keyboard");
    kill(osk_pid.custom, SIGTERM);
    g_spawn_close_pid(osk_pid.custom);
    osk_pid.custom = 0;
}

static void osk_kill_custom(void)
{
    osk_close_custom();
}

static gboolean osk_check_onboard(void)
{
    return check_program("onboard");
}

static void hide_onboard_window(GtkWidget* widget, gpointer data)
{
    gtk_widget_hide(widget);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(greeter.ui.a11y.osk_widget), FALSE);
}

static gboolean spawn_onboard(void)
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
        osk_pid.onboard = pid;
        greeter.ui.onboard = window;
    }
    return window != NULL;
}

static void osk_open_onboard(void)
{
    if(!greeter.ui.onboard && !spawn_onboard())
    {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(greeter.ui.a11y.osk_widget), FALSE);
        return;
    }

    gtk_widget_show(GTK_WIDGET(greeter.ui.onboard));
}

static void osk_close_onboard(void)
{
    g_return_if_fail(GTK_IS_WIDGET(greeter.ui.onboard));

    gtk_widget_hide(GTK_WIDGET(greeter.ui.onboard));
}

static void osk_kill_onboard(void)
{
    g_return_if_fail(osk_pid.onboard != 0);

    g_debug("Killing on-screen keyboard");
    kill(osk_pid.onboard, SIGTERM);
    g_spawn_close_pid(osk_pid.onboard);
    gtk_widget_destroy(GTK_WIDGET(greeter.ui.onboard));
    osk_pid.onboard = 0;
    greeter.ui.onboard = NULL;
}

