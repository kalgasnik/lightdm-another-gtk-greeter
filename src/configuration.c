/* configuration.c
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

#include "configuration.h"

#include <string.h>

/* Variables */

GreeterConfig config;

/* Static functions */

static gboolean read_value_bool                    (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    gboolean default_value);

static gint read_value_int                         (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    gint default_value);

static gint read_value_int_missing                 (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    gint missing_value,
                                                    gint empty_value);

static gchar* read_value_string                    (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const gchar* default_value);

static gchar* read_value_string_missing            (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const gchar* missing_value,
                                                    const gchar* empty_value);

static int read_value_enum                         (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const gchar** names,
                                                    int default_value);

static void set_gtk_property                       (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    GtkSettings* settings,
                                                    const gchar* property);

static void set_gtk_property_bool                  (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    GtkSettings* settings,
                                                    const gchar* property);

static gchar* set_and_save_gtk_property            (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    GtkSettings* settings,
                                                    const gchar* property);

static WindowPosition read_value_window_position   (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const WindowPosition* default_value);
static int read_value_int_percent                  (GKeyFile* config_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const gint default_value,
                                                    const gint default_is_percent,
                                                    gint* is_percent);

/* Static variables */

static GKeyFile* state_file = NULL;
static gchar* state_filename = NULL;

static const gchar* USER_NAME_FORMAT_STRINGS[] = {"name", "display-name", "both", NULL};
static const gchar* PANEL_POSITION_STRINGS[] = {"top", "bottom", NULL};
static const gchar* ONBOARD_POSITION_STRINGS[] = {"top", "bottom", "panel", "opposite", NULL};

/* ---------------------------------------------------------------------------*
 * Definitions: public
 * -------------------------------------------------------------------------- */

void load_settings(void)
{
    g_message("Reading configuration: %s", CONFIG_FILE);

    GError* error = NULL;
    GKeyFile* config_file = g_key_file_new();
    if(!g_key_file_load_from_file(config_file, CONFIG_FILE, G_KEY_FILE_NONE, &error))
    {
        g_warning("Failed to load configuration: %s", error->message);
        g_clear_error(&error);
    }

    GtkSettings* settings = gtk_settings_get_default();

    const gchar* CONFIG_SECTION = NULL;

    CONFIG_SECTION = "greeter";
    config.greeter.allow_other_users = read_value_bool(config_file, CONFIG_SECTION, "allow-other-users", FALSE);
    config.greeter.show_language_selector = read_value_bool(config_file, CONFIG_SECTION, "show-language-selector", TRUE);
    config.greeter.show_session_icon = read_value_bool(config_file, CONFIG_SECTION, "show-session-icon", FALSE);
    config.greeter.position = read_value_window_position(config_file, CONFIG_SECTION, "position", &WINDOW_POSITION_CENTER);

    CONFIG_SECTION = "appearance";
    config.appearance.ui_file = read_value_string(config_file, CONFIG_SECTION, "ui-file", "greeter.classic.ui");
    config.appearance.css_file = read_value_string(config_file, CONFIG_SECTION, "css-file", NULL);
    config.appearance.background = read_value_string(config_file, CONFIG_SECTION, "background", NULL);
    config.appearance.user_background = read_value_bool(config_file, CONFIG_SECTION, "user-background", TRUE);
    config.appearance.logo = read_value_string(config_file, CONFIG_SECTION, "logo", NULL);
    config.appearance.fixed_user_image_size = read_value_bool(config_file, CONFIG_SECTION, "fixed-user-image-size", TRUE);
    config.appearance.list_view_image_size = read_value_int(config_file, CONFIG_SECTION, "list-view-image-size", 48);
    config.appearance.default_user_image_size = read_value_int(config_file, CONFIG_SECTION, "default-user-image-size", 96);
    config.appearance.theme = set_and_save_gtk_property(config_file, CONFIG_SECTION, "theme-name", settings, "gtk-theme-name");
    config.appearance.icon_theme = set_and_save_gtk_property(config_file, CONFIG_SECTION, "icon-theme-name", settings, "gtk-icon-theme-name");
    config.appearance.font = set_and_save_gtk_property(config_file, CONFIG_SECTION, "font-name", settings, "gtk-font-name");
    config.appearance.fixed_login_button_width = read_value_bool(config_file, CONFIG_SECTION, "fixed-login-button-width", FALSE);
    set_gtk_property(config_file, CONFIG_SECTION, "xft-hintstyle", settings, "gtk-xft-hintstyle");
    set_gtk_property(config_file, CONFIG_SECTION, "xft-rgba", settings, "gtk-xft-rgba");
    set_gtk_property_bool(config_file, CONFIG_SECTION, "xft-antialias", settings, "gtk-xft-antialias");

    gint xft_dpi = g_key_file_get_integer(config_file, CONFIG_SECTION, "xft-dpi", &error);
    if(!error)
        g_object_set(settings, "gtk-xft-dpi", 1024*xft_dpi, NULL);
    g_clear_error(&error);

    config.appearance.user_name_format = read_value_enum(config_file, CONFIG_SECTION, "user-name-format",
                                                         USER_NAME_FORMAT_STRINGS, USER_NAME_FORMAT_DISPLAYNAME);
    config.appearance.date_format = read_value_string(config_file, CONFIG_SECTION, "date-format", "%A, %e %B");

    CONFIG_SECTION = "panel";
    config.panel.enabled = read_value_bool(config_file, CONFIG_SECTION, "enabled", TRUE);
    config.panel.position = read_value_enum(config_file, CONFIG_SECTION, "position",
                                              PANEL_POSITION_STRINGS, PANEL_POS_TOP);

    CONFIG_SECTION = "power";
    config.power.enabled = read_value_bool(config_file, CONFIG_SECTION, "enabled", TRUE);
    config.power.suspend_prompt = read_value_bool(config_file, CONFIG_SECTION, "suspend-prompt", FALSE);
    config.power.hibernate_prompt = read_value_bool(config_file, CONFIG_SECTION, "hibernate-prompt", FALSE);
    config.power.restart_prompt = read_value_bool(config_file, CONFIG_SECTION, "restart-prompt", TRUE);
    config.power.shutdown_prompt = read_value_bool(config_file, CONFIG_SECTION, "shutdown-prompt", TRUE);

    CONFIG_SECTION = "clock";
    config.clock.enabled = read_value_bool(config_file, CONFIG_SECTION, "enabled", TRUE);
    config.clock.calendar = read_value_bool(config_file, CONFIG_SECTION, "show-calendar", TRUE);
    config.clock.time_format = read_value_string(config_file, CONFIG_SECTION, "time-format", "%T");
    config.clock.date_format = read_value_string(config_file, CONFIG_SECTION, "date-format", "%A, %e %B %Y");

    CONFIG_SECTION = "a11y";
    config.a11y.enabled = read_value_bool(config_file, CONFIG_SECTION, "enabled", TRUE);
    config.a11y.theme_contrast = read_value_string_missing(config_file, CONFIG_SECTION, "theme-name-contrast",
                                                           "HighContrast", NULL);
    config.a11y.icon_theme_contrast = read_value_string(config_file, CONFIG_SECTION, "icon-theme-name-contrast", "HighContrast");
    config.a11y.check_theme = read_value_bool(config_file, CONFIG_SECTION, "check-theme", TRUE);
    config.a11y.font_increment = read_value_int_percent(config_file, CONFIG_SECTION, "font-increment",
                                                        40, TRUE, &config.a11y.font_increment_is_percent);
    config.a11y.osk_use_onboard = read_value_bool(config_file, CONFIG_SECTION, "osk-use-onboard", FALSE);

    gint argp;
    gchar* osk_command = read_value_string(config_file, CONFIG_SECTION, "osk-command", NULL);
    if(!osk_command || !g_shell_parse_argv(osk_command, &argp, &config.a11y.osk_command_array, NULL))
        config.a11y.osk_command_array = NULL;
    g_free(osk_command);

    CONFIG_SECTION = "layout";
    config.layout.enabled = read_value_bool(config_file, CONFIG_SECTION, "enabled", TRUE);

    CONFIG_SECTION = "onboard";
    config.onboard.position = read_value_enum(config_file, CONFIG_SECTION, "position",
                                              ONBOARD_POSITION_STRINGS, ONBOARD_POS_PANEL_OPPOSITE);
    config.onboard.height = read_value_int_percent(config_file, CONFIG_SECTION, "height",
                                                   25, TRUE, &config.onboard.height_is_percent);


    g_key_file_free(config_file);

    #ifdef _DEBUG_
    config.greeter.show_session_icon = TRUE;
    #endif

    /* Reading state file */
    gchar* state_dir = g_build_filename(g_get_user_cache_dir(), APP_NAME, NULL);
    g_mkdir_with_parents(state_dir, 0775);
    state_filename = g_build_filename(state_dir, "state", NULL);
    g_free(state_dir);

    state_file = g_key_file_new();
    g_key_file_load_from_file(state_file, state_filename, G_KEY_FILE_NONE, &error);
    if(error && !g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        g_warning("Failed to load state from %s: %s\n", state_filename, error->message);
    g_clear_error(&error);
}

gchar* get_last_logged_user(void)
{
    g_return_val_if_fail(state_file != NULL, NULL);
    return g_key_file_get_value(state_file, "greeter", "last-user", NULL);
}

void save_last_logged_user(const gchar* user_name)
{
    g_return_if_fail(state_file != NULL && user_name != NULL);

    g_key_file_set_value(state_file, "greeter", "last-user", user_name);

    gsize data_length = 0;
    GError* error = NULL;
    gchar* data = g_key_file_to_data(state_file, &data_length, &error);
    if(error)
    {
        g_warning("Failed to save state file: %s", error->message);
        g_clear_error(&error);
    }

    if(data)
    {
        g_file_set_contents(state_filename, data, data_length, &error);
        if(error)
        {
            g_warning ("Failed to save state file: %s", error->message);
            g_clear_error(&error);
        }
        g_free(data);
    }
}

/* ---------------------------------------------------------------------------*
 * Definitions: static
 * -------------------------------------------------------------------------- */

static gboolean read_value_bool(GKeyFile* config_file,
                                const gchar* section,
                                const gchar* key,
                                gboolean default_value)
{
    g_return_val_if_fail(config_file && section && key, default_value);

    GError* error = NULL;
    gboolean value = g_key_file_get_boolean(config_file, section, key, &error);
    if(error)
    {
        g_clear_error(&error);
        return default_value;
    }
    return value;
}

static gint read_value_int(GKeyFile* config_file,
                           const gchar* section,
                           const gchar* key,
                           gint default_value)
{
    g_return_val_if_fail(config_file && section && key, default_value);

    GError* error = NULL;
    gint value = g_key_file_get_integer(config_file, section, key, &error);
    if(error)
    {
        g_clear_error(&error);
        return default_value;
    }
    return value;
}

static gint read_value_int_missing(GKeyFile* config_file,
                                   const gchar* section,
                                   const gchar* key,
                                   gint missing_value,
                                   gint empty_value)
{
    g_return_val_if_fail(config_file && section && key, missing_value);

    if(!g_key_file_has_key(config_file, section, key, NULL))
        return missing_value;
    else
        return read_value_int(config_file, section, key, empty_value);
}

static gchar* read_value_string(GKeyFile* config_file,
                                const gchar* section,
                                const gchar* key,
                                const gchar* default_value)
{
    g_return_val_if_fail(config_file && section && key, default_value);

    gchar* value = g_key_file_get_string(config_file, section, key, NULL);
    return value ? value : g_strdup(default_value);
}

static gchar* read_value_string_missing(GKeyFile* config_file,
                                        const gchar* section,
                                        const gchar* key,
                                        const gchar* missing_value,
                                        const gchar* empty_value)
{
    g_return_val_if_fail(config_file && section && key, missing_value);

    if(!g_key_file_has_key(config_file, section, key, NULL))
        return missing_value ? g_strdup(missing_value) : NULL;
    else
        return read_value_string(config_file, section, key, empty_value);
}

static int read_value_enum(GKeyFile* config_file,
                           const gchar* section,
                           const gchar* key,
                           const gchar** names,
                           int default_value)
{
    g_return_val_if_fail(config_file && section && key && names, default_value);

    gchar* value = g_key_file_get_string(config_file, section, key, NULL);
    if(value)
    {
        for(int i = 0; names[i]; ++i)
            if(g_strcmp0(names[i], value) == 0)
            {
                default_value = i;
                break;
            }
        g_free(value);
    }
    return default_value;
}

static void set_gtk_property(GKeyFile* config_file,
                             const gchar* section,
                             const gchar* key,
                             GtkSettings* settings,
                             const gchar* property)
{
    g_return_if_fail(config_file && section && key && settings && property);

    gchar* value = g_key_file_get_value(config_file, section, key, NULL);
    if(value)
        g_object_set(settings, property, value, NULL);
    g_free(value);
}

static void set_gtk_property_bool(GKeyFile* config_file,
                                  const gchar* section,
                                  const gchar* key,
                                  GtkSettings* settings,
                                  const gchar* property)
{
    g_return_if_fail(config_file && section && key && settings && property);

    GError* error = NULL;
    gboolean value = g_key_file_get_boolean(config_file, section, key, &error);
    if(!error)
        g_object_set(settings, property, value, NULL);
    else
        g_clear_error(&error);
}

static gchar* set_and_save_gtk_property(GKeyFile* config_file,
                                        const gchar* section,
                                        const gchar* key,
                                        GtkSettings* settings,
                                        const gchar* property)
{
    g_return_val_if_fail(config_file && section && key && settings && property, NULL);

    gchar* value = g_key_file_get_value(config_file, section, key, NULL);
    if(value)
        g_object_set(settings, property, value, NULL);
    g_free(value);
    g_object_get(settings, property, &value, NULL);
    return value;
}

static WindowPosition read_value_window_position(GKeyFile* config_file,
                                                 const gchar* section,
                                                 const gchar* key,
                                                 const WindowPosition* default_value)
{
    g_return_val_if_fail(config_file && section && key && default_value, WINDOW_POSITION_CENTER);

    WindowPosition p = *default_value;

    gchar* value = g_key_file_get_value(config_file, section, key, NULL);
    if(value)
    {
        struct
        {
            int* value;
            gboolean* is_absolute;
        } items[] = {{&p.x, &p.x_is_absolute}, {&p.y, &p.y_is_absolute},
                     {&p.anchor.width, NULL}, {&p.anchor.height, NULL},
                     {NULL, NULL}};
        gchar** a = g_strsplit(value, ",", 4);
        gchar*  a_end;
        for(int i = 0; items[i].value && a[i]; ++i)
        {
            *items[i].value = (int)g_strtod(a[i], &a_end);
            if(items[i].is_absolute)
                *items[i].is_absolute = a_end[0] != '%';
        }
        g_strfreev(a);
    }
    g_free(value);
    return p;
}

static int read_value_int_percent(GKeyFile* config_file,
                                  const gchar* section,
                                  const gchar* key,
                                  const gint default_value,
                                  const gint default_is_percent,
                                  gint* is_percent)
{
    if(is_percent != NULL)
        *is_percent = default_is_percent;

    g_return_val_if_fail(config_file && section && key, default_value);

    GError* error = NULL;
    gchar* value_end;
    gchar* value = g_key_file_get_value(config_file, section, key, &error);
    if(error)
    {
        g_clear_error(&error);
        return default_value;
    }

    gint result = (int)g_strtod(value, &value_end);
    *is_percent = value_end[0] == '%';
    g_free(value);
    return result;
}




