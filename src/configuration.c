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

GreeterConfig config =
{
    .greeter =
    {
        .double_escape_time = 300
    }
};

/* Static functions */

static void save_key_file                          (GKeyFile* key_file,
                                                    const gchar* path);

static gboolean read_value_bool                    (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    gboolean default_value);

static gboolean read_value_bool_gtk                (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    GtkSettings* settings,
                                                    const gchar* property,
                                                    gboolean default_value,
                                                    gboolean apply_default);

static gint read_value_int                         (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    gint default_value);

static gchar* read_value_str                       (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const gchar* default_value);

static gchar* read_value_str_gtk                   (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    GtkSettings* settings,
                                                    const gchar* property,
                                                    const gchar* default_value,
                                                    gboolean apply_default);

static gint read_value_enum                        (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const gchar** names,
                                                    gint default_value);

static WindowPosition read_value_wp                (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const WindowPosition* default_value);

static int read_value_percents                     (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const gint default_value,
                                                    const gint default_is_percent,
                                                    gint* is_percent);

static gchar** read_value_command                  (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key);

static gint read_value_dpi_gtk                     (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    GtkSettings* settings,
                                                    const gchar* property);


/* Static variables */

static struct
{
    GKeyFile* config;
    gchar* path;
} state_data;

static const gchar* USER_NAME_FORMAT_STRINGS[] = {"name", "display-name", "both", NULL};
static const gchar* PANEL_POSITION_STRINGS[]   = {"top", "bottom", NULL};
static const gchar* ONBOARD_POSITION_STRINGS[] = {"top", "bottom", "panel", "opposite", NULL};

/* ---------------------------------------------------------------------------*
 * Definitions: public
 * -------------------------------------------------------------------------- */

void load_settings(void)
{
    g_message("Reading configuration: %s", CONFIG_FILE);

    GError* error = NULL;
    GKeyFile* cfg = g_key_file_new();
    if(!g_key_file_load_from_file(cfg, CONFIG_FILE, G_KEY_FILE_NONE, &error))
    {
        g_warning("Failed to load configuration: %s. Using default values.", error->message);
        g_clear_error(&error);
    }

    const gchar* SECTION = NULL;
    GtkSettings* settings = gtk_settings_get_default();

    SECTION = "greeter";
    config.greeter.allow_other_users          = read_value_bool    (cfg, SECTION, "allow-other-users",      FALSE);
    config.greeter.show_language_selector     = read_value_bool    (cfg, SECTION, "show-language-selector", TRUE);
    config.greeter.show_session_icon          = read_value_bool    (cfg, SECTION, "show-session-icon",      FALSE);
    config.greeter.position                   = read_value_wp      (cfg, SECTION, "position",              &WINDOW_POSITION_CENTER);

    SECTION = "appearance";
    config.appearance.ui_file                 = read_value_str     (cfg, SECTION, "ui-file", "greeter.classic.ui");
    config.appearance.css_file                = read_value_str     (cfg, SECTION, "css-file", NULL);
    config.appearance.background              = read_value_str     (cfg, SECTION, "background", NULL);
    config.appearance.user_background         = read_value_bool    (cfg, SECTION, "user-background", TRUE);
    config.appearance.logo                    = read_value_str     (cfg, SECTION, "logo", NULL);
    config.appearance.fixed_user_image_size   = read_value_bool    (cfg, SECTION, "fixed-user-image-size", TRUE);
    config.appearance.list_view_image_size    = read_value_int     (cfg, SECTION, "list-view-image-size", 48);
    config.appearance.default_user_image_size = read_value_int     (cfg, SECTION, "default-user-image-size", 96);
    config.appearance.theme                   = read_value_str_gtk (cfg, SECTION, "theme", settings, "gtk-theme-name", NULL, FALSE);
    config.appearance.icon_theme              = read_value_str_gtk (cfg, SECTION, "icon-theme", settings, "gtk-icon-theme-name", NULL, FALSE);
    config.appearance.font                    = read_value_str_gtk (cfg, SECTION, "font-name", settings, "gtk-font-name", NULL, FALSE);
    config.appearance.fixed_login_button_width= read_value_bool    (cfg, SECTION, "fixed-login-button-width", FALSE);

    config.appearance.hintstyle               = read_value_str_gtk (cfg, SECTION, "xft-hintstyle", settings, "gtk-xft-hintstyle", NULL, FALSE);
    config.appearance.rgba                    = read_value_str_gtk (cfg, SECTION, "xft-rgba", settings, "gtk-xft-rgba", NULL, FALSE);
    config.appearance.antialias               = read_value_bool_gtk(cfg, SECTION, "xft-antialias", settings, "gtk-xft-antialias", FALSE, FALSE);
    config.appearance.dpi                     = read_value_dpi_gtk (cfg, SECTION, "xft-dpi", settings, "gtk-xft-dpi");

    config.appearance.user_name_format        = read_value_enum    (cfg, SECTION, "user-name-format",
                                                                    USER_NAME_FORMAT_STRINGS, USER_NAME_FORMAT_DISPLAYNAME);
    config.appearance.date_format             = read_value_str     (cfg, SECTION, "date-format", "%A, %e %B");

    SECTION = "panel";
    config.panel.enabled                      = read_value_bool    (cfg, SECTION, "enabled", TRUE);
    config.panel.position                     = read_value_enum    (cfg, SECTION, "position",
                                                                    PANEL_POSITION_STRINGS, PANEL_POS_TOP);

    SECTION = "power";
    config.power.enabled                      = read_value_bool    (cfg, SECTION, "enabled", TRUE);
    config.power.prompts[POWER_SUSPEND]       = read_value_bool    (cfg, SECTION, "suspend-prompt",   FALSE);
    config.power.prompts[POWER_HIBERNATE]     = read_value_bool    (cfg, SECTION, "hibernate-prompt", FALSE);
    config.power.prompts[POWER_RESTART]       = read_value_bool    (cfg, SECTION, "restart-prompt",   TRUE);
    config.power.prompts[POWER_SHUTDOWN]      = read_value_bool    (cfg, SECTION, "shutdown-prompt",  TRUE);

    SECTION = "clock";
    config.clock.enabled                      = read_value_bool    (cfg, SECTION, "enabled", TRUE);
    config.clock.calendar                     = read_value_bool    (cfg, SECTION, "show-calendar", TRUE);
    config.clock.time_format                  = read_value_str     (cfg, SECTION, "time-format", "%T");
    config.clock.date_format                  = read_value_str     (cfg, SECTION, "date-format", "%A, %e %B %Y");

    SECTION = "a11y";
    config.a11y.enabled                       = read_value_bool    (cfg, SECTION, "enabled", TRUE);

    SECTION = "a11y.contrast";
    config.a11y.contrast.theme                = read_value_str     (cfg, SECTION, "theme", "HighContrast");
    config.a11y.contrast.icon_theme           = read_value_str     (cfg, SECTION, "icon-theme", "HighContrast");
    config.a11y.contrast.check_theme          = read_value_bool    (cfg, SECTION, "check-theme", TRUE);
    config.a11y.contrast.initial_state        = read_value_bool    (cfg, SECTION, "initial-state", FALSE);
    config.a11y.contrast.enabled              = config.a11y.contrast.theme && (strlen(config.a11y.contrast.theme) > 0);

    SECTION = "a11y.font";
    config.a11y.font.increment                = read_value_percents(cfg, SECTION, "increment",
                                                                    40, TRUE, &config.a11y.font.is_percent);
    config.a11y.font.initial_state            = read_value_bool    (cfg, SECTION, "initial-state", FALSE);
    config.a11y.font.enabled                  = config.a11y.font.increment > 0;

    SECTION = "a11y.dpi";
    config.a11y.dpi.increment                 = read_value_percents(cfg, SECTION, "increment",
                                                                    -1, TRUE, &config.a11y.dpi.is_percent);
    config.a11y.dpi.initial_state             = read_value_bool    (cfg, SECTION, "initial-state", FALSE);
    config.a11y.dpi.enabled                   = config.a11y.dpi.increment > 0;

    SECTION = "a11y.osk";
    config.a11y.osk.command                   = !config.a11y.osk.use_onboard ? read_value_command(cfg, SECTION, "command") : NULL;
    config.a11y.osk.use_onboard               = read_value_bool    (cfg, SECTION, "use-onboard", FALSE);
    config.a11y.osk.onboard_position          = read_value_enum    (cfg, SECTION, "onboard-position",
                                                                    ONBOARD_POSITION_STRINGS, ONBOARD_POS_PANEL_OPPOSITE);
    config.a11y.osk.onboard_height            = read_value_percents(cfg, SECTION, "onboard-height",
                                                                    25, TRUE, &config.a11y.osk.onboard_height_is_percent);
    config.a11y.osk.enabled                   = config.a11y.osk.use_onboard || config.a11y.osk.command;

    SECTION = "layout";
    config.layout.enabled                     = read_value_bool    (cfg, SECTION, "enabled", TRUE);

    g_key_file_free(cfg);

    #ifdef _DEBUG_
    config.greeter.show_session_icon = TRUE;
    #endif
}

void read_state(void)
{
    g_message("Reading state: %s", CONFIG_FILE);

    GError* error = NULL;
    gchar* state_dir = g_build_filename(g_get_user_cache_dir(), APP_NAME, NULL);

    g_mkdir_with_parents(state_dir, 0775);
    state_data.path = g_build_filename(state_dir, "state", NULL);
    state_data.config = g_key_file_new();
    g_key_file_load_from_file(state_data.config, state_data.path, G_KEY_FILE_NONE, &error);
    if(error && !g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        g_warning("Failed to load state from %s: %s\n", state_data.path, error->message);
    g_clear_error(&error);
    g_free(state_dir);
}

gchar* get_last_logged_user(void)
{
    return read_value_str(state_data.config, "greeter", "last-user", NULL);
}

void save_last_logged_user(const gchar* user_name)
{
    g_return_if_fail(state_data.config != NULL);
    g_return_if_fail(user_name != NULL);

    g_key_file_set_value(state_data.config, "greeter", "last-user", user_name);

    save_key_file(state_data.config, state_data.path);
}

gboolean a11y_get_font_state(void)
{
    return read_value_bool(state_data.config, "a11y", "font", FALSE);
}

void a11y_save_font_state(gboolean state)
{
    g_return_if_fail(state_data.config != NULL);
    g_key_file_set_boolean(state_data.config, "a11y", "font", state);
    save_key_file(state_data.config, state_data.path);
}

gboolean a11y_get_dpi_state(void)
{
    return read_value_bool(state_data.config, "a11y", "dpi", config.a11y.dpi.initial_state);
}

void a11y_save_dpi_state(gboolean state)
{
    g_return_if_fail(state_data.config != NULL);
    g_key_file_set_boolean(state_data.config, "a11y", "dpi", state);
    save_key_file(state_data.config, state_data.path);
}

gboolean a11y_get_contrast_state(void)
{
    return read_value_bool(state_data.config, "a11y", "contrast", config.a11y.contrast.initial_state);
}

void a11y_save_contrast_state(gboolean state)
{
    g_return_if_fail(state_data.config != NULL);
    g_key_file_set_boolean(state_data.config, "a11y", "contrast", state);
    save_key_file(state_data.config, state_data.path);
}

/* ---------------------------------------------------------------------------*
 * Definitions: static
 * -------------------------------------------------------------------------- */

static void save_key_file(GKeyFile* key_file,
                          const gchar* path)
{
    g_return_if_fail(key_file != NULL);
    g_return_if_fail(path != NULL);

    gsize data_length = 0;
    gchar* data = g_key_file_to_data(key_file, &data_length, NULL);

    g_return_if_fail(data != NULL);

    GError* error = NULL;
    if(!g_file_set_contents(path, data, data_length, &error))
    {
        g_warning("Failed to save state file: %s", error->message);
        g_clear_error(&error);
    }
    g_free(data);
}

static gboolean read_value_bool(GKeyFile* key_file,
                                const gchar* section,
                                const gchar* key,
                                gboolean default_value)
{
    g_return_val_if_fail(key_file != NULL, default_value);
    g_return_val_if_fail(section != NULL, default_value);
    g_return_val_if_fail(key != NULL, default_value);

    GError* error = NULL;
    gboolean value = g_key_file_get_boolean(key_file, section, key, &error);
    if(error)
    {
        g_clear_error(&error);
        return default_value;
    }
    return value;
}

static gboolean read_value_bool_gtk(GKeyFile* key_file,
                                    const gchar* section,
                                    const gchar* key,
                                    GtkSettings* settings,
                                    const gchar* property,
                                    gboolean default_value,
                                    gboolean apply_default)
{
    g_return_val_if_fail(settings != NULL, default_value);
    g_return_val_if_fail(property != NULL, default_value);
    g_return_val_if_fail(key_file != NULL, default_value);
    g_return_val_if_fail(section != NULL, default_value);
    g_return_val_if_fail(key != NULL, default_value);

    GError* error = NULL;
    gboolean value = g_key_file_get_boolean(key_file, section, key, &error);

    if(!error || apply_default)
        g_object_set(settings, property, error ? default_value : value, NULL);
    g_object_get(settings, property, &value, NULL);
    g_clear_error(&error);
    return value;
}

static gint read_value_int(GKeyFile* key_file,
                           const gchar* section,
                           const gchar* key,
                           gint default_value)
{
    g_return_val_if_fail(key_file != NULL, default_value);
    g_return_val_if_fail(section != NULL, default_value);
    g_return_val_if_fail(key != NULL, default_value);

    GError* error = NULL;
    gint value = g_key_file_get_integer(key_file, section, key, &error);
    if(error)
    {
        g_clear_error(&error);
        return default_value;
    }
    return value;
}

static gchar* read_value_str(GKeyFile* key_file,
                             const gchar* section,
                             const gchar* key,
                             const gchar* default_value)
{
    g_return_val_if_fail(key_file != NULL, g_strdup(default_value));
    g_return_val_if_fail(section != NULL, g_strdup(default_value));
    g_return_val_if_fail(key != NULL, g_strdup(default_value));

    gchar* value = g_key_file_get_string(key_file, section, key, NULL);
    return value ? value : g_strdup(default_value);
}

static gchar* read_value_str_gtk(GKeyFile* key_file,
                                 const gchar* section,
                                 const gchar* key,
                                 GtkSettings* settings,
                                 const gchar* property,
                                 const gchar* default_value,
                                 gboolean apply_default)
{
    g_return_val_if_fail(settings != NULL, g_strdup(default_value));
    g_return_val_if_fail(property != NULL, g_strdup(default_value));
    g_return_val_if_fail(key_file != NULL, g_strdup(default_value));
    g_return_val_if_fail(section != NULL, g_strdup(default_value));
    g_return_val_if_fail(key != NULL, g_strdup(default_value));

    GError* error = NULL;
    gchar* value = g_key_file_get_string(key_file, section, key, &error);

    if(!error || apply_default)
        g_object_set(settings, property, error ? default_value : value, NULL);
    g_object_get(settings, property, &value, NULL);
    g_clear_error(&error);
    return value;
}

static int read_value_enum(GKeyFile* key_file,
                           const gchar* section,
                           const gchar* key,
                           const gchar** names,
                           gint default_value)
{
    g_return_val_if_fail(key_file != NULL, default_value);
    g_return_val_if_fail(section != NULL, default_value);
    g_return_val_if_fail(key != NULL, default_value);
    g_return_val_if_fail(names != NULL, default_value);

    gchar* value = g_key_file_get_string(key_file, section, key, NULL);
    if(value)
    {
        for(int i = 0; names[i] != NULL; ++i)
            if(g_strcmp0(names[i], value) == 0)
            {
                default_value = i;
                break;
            }
        g_free(value);
    }
    return default_value;
}

static WindowPosition read_value_wp(GKeyFile* key_file,
                                    const gchar* section,
                                    const gchar* key,
                                    const WindowPosition* default_value)
{
    g_return_val_if_fail(default_value != NULL, WINDOW_POSITION_CENTER);
    g_return_val_if_fail(key_file != NULL, *default_value);
    g_return_val_if_fail(section != NULL, *default_value);
    g_return_val_if_fail(key != NULL, *default_value);

    WindowPosition p = *default_value;

    gchar* value = g_key_file_get_value(key_file, section, key, NULL);
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
        g_free(value);
    }
    return p;
}

static int read_value_percents(GKeyFile* key_file,
                               const gchar* section,
                               const gchar* key,
                               const gint default_value,
                               const gint default_is_percent,
                               gint* is_percent)
{
    if(is_percent != NULL)
        *is_percent = default_is_percent;

    g_return_val_if_fail(key_file != NULL, default_value);
    g_return_val_if_fail(section != NULL, default_value);
    g_return_val_if_fail(key != NULL, default_value);

    GError* error = NULL;
    gchar* value_end;
    gchar* value = g_key_file_get_value(key_file, section, key, &error);
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

static gchar** read_value_command(GKeyFile* key_file,
                                  const gchar* section,
                                  const gchar* key)
{
    g_return_val_if_fail(key_file != NULL, NULL);
    g_return_val_if_fail(section != NULL, NULL);
    g_return_val_if_fail(key != NULL, NULL);

    gchar** array = NULL;
    gchar* s = read_value_str(key_file, section, key, NULL);
    g_shell_parse_argv(s, NULL, &array, NULL);
    g_free(s);
    return array;
}

static gint read_value_dpi_gtk(GKeyFile* key_file,
                                  const gchar* section,
                                  const gchar* key,
                                  GtkSettings* settings,
                                  const gchar* property)
{
    g_return_val_if_fail(settings != NULL, 0);
    g_return_val_if_fail(property != NULL, 0);
    g_return_val_if_fail(key_file != NULL, 0);
    g_return_val_if_fail(section != NULL, 0);
    g_return_val_if_fail(key != NULL, 0);

    GError* error = NULL;
    gint value = g_key_file_get_integer(key_file, section, key, &error);

    if(!error)
        g_object_set(settings, property, 1024*value, NULL);
    g_object_get(settings, property, &value, NULL);
    g_clear_error(&error);
    return value/1024;
}

