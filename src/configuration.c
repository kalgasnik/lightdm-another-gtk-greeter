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

#include <glib/gi18n.h>
#include <string.h>

/* Variables */

GreeterConfig config =
{
    .greeter =
    {
        .double_escape_time = 300
    },
    .layout =
    {
        .enabled_for_one = TRUE
    }
};

/* Types */

/* Catched config data */
typedef struct
{
    /* Path to config directory */
    gchar* path;
    /* Path to config .css file */
    gchar* css_path;
} LoadedGreeterConfig;

/* Static functions */

static void save_key_file                          (GKeyFile* key_file,
                                                    const gchar* path);

static void read_appearance_section                (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* path,
                                                    GtkSettings* settings,
                                                    const gchar* default_theme);

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
static gboolean read_value_wp_dimension            (const gchar *s,
                                                    WindowPositionDimension *x);

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

static gchar* read_value_path                      (GKeyFile* key_file,
                                                    const gchar* section,
                                                    const gchar* key,
                                                    const gchar* default_value,
                                                    const gchar* dir);

static GtkStyleProvider* read_css_file             (const gchar* path,
                                                    GdkScreen* screen);

static void read_templates                         (void);

void update_default_user_image                     (void);

/* Static variables */

static struct
{
    GKeyFile* config;
    gchar* path;
} state_data;

/* Hash set used to control infinite recursive configs reading
   initialization and updating: read_appearance_section()
   freed: load_settings() */
static GHashTable* loaded_themes = NULL;

/* Static constants */

static const gchar* USER_NAME_FORMAT_STRINGS[] = {"name", "display-name", "both", NULL};
static const gchar* PANEL_POSITION_STRINGS[]   = {"top", "bottom", NULL};
static const gchar* ONBOARD_POSITION_STRINGS[] = {"top", "bottom", "panel", "opposite", NULL};
static const gchar* USER_IMAGE_FIT_STRINGS[]   = {"none", "all", "bigger", "smaller", NULL};
static const gchar* POWER_ACTION_STRINGS[]     = {"none", "suspend", "hibernate", "restart", "shutdown", NULL};

#if GTK_CHECK_VERSION(3, 10, 0)
static const gchar* SESSION_COLUMN_STRINGS[]   = {"name", "display-name", "image", "comment", NULL};
static const gchar* LANGUAGE_COLUMN_STRINGS[]  = {"code", "display-name", NULL};
static const gchar* USER_COLUMN_STRINGS[]      = {"name", "type", "display-name", "font-weight", "user-image", "list-image",
                                                  "logged-in", NULL};
#endif

static const ModelPropertyBinding SESSION_TEMPLATE_DEFAULT_BINDINGS[]  = {{NULL, "text",   SESSION_COLUMN_DISPLAY_NAME},
                                                                          {NULL, "pixbuf", SESSION_COLUMN_IMAGE},
                                                                          {NULL, NULL,     -1}};

static const ModelPropertyBinding LANGUAGE_TEMPLATE_DEFAULT_BINDINGS[] = {{NULL, "text", LANGUAGE_COLUMN_DISPLAY_NAME},
                                                                          {NULL, NULL,   -1}};

static const ModelPropertyBinding USER_TEMPLATE_DEFAULT_BINDINGS[]     = {{NULL, "text",        USER_COLUMN_DISPLAY_NAME},
                                                                          {NULL, "text-weight", USER_COLUMN_WEIGHT},
                                                                          {NULL, "pixbuf",      USER_COLUMN_LIST_IMAGE},
                                                                          {NULL, NULL,     -1}};
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
    config.greeter.allow_password_toggle      = read_value_bool    (cfg, SECTION, "allow-password-toggle",  FALSE);

    SECTION = "appearance";
    config.appearance.themes_stack            = NULL;
    config.appearance.ui_file                 = "themes/default/greeter.ui";
    config.appearance.background              = NULL;
    config.appearance.user_background         = TRUE;
    config.appearance.x_background            = FALSE;
    config.appearance.logo                    = NULL;
    config.appearance.user_image.enabled      = TRUE;
    config.appearance.user_image.fit          = USER_IMAGE_FIT_BIGGER;
    config.appearance.list_image.enabled      = TRUE;
    config.appearance.list_image.fit          = USER_IMAGE_FIT_BIGGER;
    config.appearance.list_image.size         = 48;
    config.appearance.gtk_theme               = NULL;
    config.appearance.icon_theme              = NULL;
    config.appearance.font                    = NULL;
    config.appearance.fixed_login_button_width= FALSE;
    config.appearance.hintstyle               = NULL;
    config.appearance.rgba                    = NULL;
    config.appearance.antialias               = FALSE;
    config.appearance.dpi                     = 0;
    config.appearance.user_name_format        = USER_NAME_FORMAT_DISPLAYNAME;
    config.appearance.date_format             = "%A, %e %B";
    config.appearance.position                = WINDOW_POSITION_CENTER;
    config.appearance.position_is_relative    = FALSE;
    config.appearance.hide_prompt_text        = FALSE;
    config.appearance.default_user_image      = "#avatar-default";

    config.appearance.templates.session.bindings  = NULL;
    config.appearance.templates.language.bindings = NULL;
    config.appearance.templates.user.bindings     = NULL;
    #if GTK_CHECK_VERSION(3, 10, 0)
    config.appearance.templates.session.data      = NULL;
    config.appearance.templates.session.ui_file   = NULL;
    config.appearance.templates.language.data     = NULL;
    config.appearance.templates.language.ui_file  = NULL;
    config.appearance.templates.user.data         = NULL;
    config.appearance.templates.user.ui_file      = NULL;
    #endif

    read_appearance_section(cfg, SECTION, GREETER_DATA_DIR, settings, "default");
    config.appearance.themes_stack = g_slist_reverse(config.appearance.themes_stack);

    read_templates();
    if(loaded_themes)
    {
        g_hash_table_unref(loaded_themes);
        loaded_themes = NULL;
    }

    SECTION = "panel";
    config.panel.enabled                      = read_value_bool    (cfg, SECTION, "enabled", TRUE);
    config.panel.position                     = read_value_enum    (cfg, SECTION, "position",
                                                                    PANEL_POSITION_STRINGS, PANEL_POS_TOP);
    SECTION = "power";
    config.power.enabled                      = read_value_bool    (cfg, SECTION, "enabled", TRUE);
    config.power.button_press_action          = read_value_enum    (cfg, SECTION, "button-press-action",
                                                                    POWER_ACTION_STRINGS, POWER_ACTION_SHUTDOWN + 1) - 1;
    config.power.prompts[POWER_ACTION_SUSPEND]   = read_value_bool    (cfg, SECTION, "suspend-prompt",   FALSE);
    config.power.prompts[POWER_ACTION_HIBERNATE] = read_value_bool    (cfg, SECTION, "hibernate-prompt", FALSE);
    config.power.prompts[POWER_ACTION_RESTART]   = read_value_bool    (cfg, SECTION, "restart-prompt",   TRUE);
    config.power.prompts[POWER_ACTION_SHUTDOWN]  = read_value_bool    (cfg, SECTION, "shutdown-prompt",  TRUE);

    SECTION = "clock";
    config.clock.enabled                      = read_value_bool    (cfg, SECTION, "enabled", TRUE);
    config.clock.calendar                     = read_value_bool    (cfg, SECTION, "show-calendar", TRUE);
    config.clock.time_format                  = read_value_str     (cfg, SECTION, "time-format", "%T");
    config.clock.date_format                  = read_value_str     (cfg, SECTION, "date-format", "%A, %e %B %Y");

    SECTION = "a11y";
    config.a11y.enabled                       = read_value_bool    (cfg, SECTION, "enabled", TRUE);

    SECTION = "a11y.contrast";
    config.a11y.contrast.gtk_theme            = read_value_str     (cfg, SECTION, "gtk-theme", "HighContrast");
    config.a11y.contrast.icon_theme           = read_value_str     (cfg, SECTION, "icon-theme", "HighContrast");
    config.a11y.contrast.initial_state        = read_value_bool    (cfg, SECTION, "initial-state", FALSE);
    config.a11y.contrast.enabled              = config.a11y.contrast.gtk_theme && (strlen(config.a11y.contrast.gtk_theme) > 0);

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
}

void read_state(void)
{
    g_message("Reading state");

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

gchar* get_state_value_str(const gchar* section,
                           const gchar* key)
{
    return read_value_str(state_data.config, section, key, NULL);
}

void set_state_value_str(const gchar* section,
                         const gchar* key,
                         const gchar* value)
{
    g_key_file_set_value(state_data.config, section, key, value);
    save_key_file(state_data.config, state_data.path);
}

gint get_state_value_int(const gchar* section,
                         const gchar* key)
{
    return read_value_int(state_data.config, section, key, 0);

}

void set_state_value_int(const gchar* section,
                         const gchar* key,
                         gint value)
{
    g_key_file_set_integer(state_data.config, section, key, value);
    save_key_file(state_data.config, state_data.path);
}

void apply_gtk_theme(GtkSettings* settings,
                     const gchar* gtk_theme)
{
    GdkScreen* screen = gdk_screen_get_default();

    for(GSList* item = greeter.state.theming.style_providers; item != NULL; item = item->next)
    {
        gtk_style_context_remove_provider_for_screen(screen, GTK_STYLE_PROVIDER(item->data));
        g_object_unref(item->data);
    }
    g_slist_free(greeter.state.theming.style_providers);
    greeter.state.theming.style_providers = NULL;

    for(GSList* item = config.appearance.themes_stack; item != NULL; item = g_slist_next(item))
    {
        LoadedGreeterConfig* theme = item->data;
        gchar* fix_css_path_wo_ext = g_build_filename(theme->path, "gtk-themes-fixes", gtk_theme, NULL);
        gchar* fix_css_path = g_strconcat(fix_css_path_wo_ext, ".css", NULL);
        if(theme->css_path)
            greeter.state.theming.style_providers = g_slist_prepend(greeter.state.theming.style_providers,
                                                                    read_css_file(theme->css_path, screen));
        if(g_file_test(fix_css_path, G_FILE_TEST_IS_REGULAR))
            greeter.state.theming.style_providers = g_slist_prepend(greeter.state.theming.style_providers,
                                                                    read_css_file(fix_css_path, screen));
        g_free(fix_css_path);
        g_free(fix_css_path_wo_ext);
    }
    g_object_set(settings, "gtk-theme-name", gtk_theme, NULL);
    greeter.state.theming.gtk_theme_applied = TRUE;
}

void apply_icon_theme(GtkSettings* settings,
                      const gchar* icon_theme)
{
    g_object_set(settings, "gtk-icon-theme-name", icon_theme, NULL);
    update_default_user_image();
}

/* ---------------------------------------------------------------------------*
 * Definitions: static
 * -------------------------------------------------------------------------- */

static void save_key_file(GKeyFile* key_file,
                          const gchar* path)
{
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
#if GTK_CHECK_VERSION(3, 10, 0)
static GSList* read_template_section(GKeyFile* cfg,
                                     const gchar* SECTION,
                                     const gchar** COLUMN_STRINGS,
                                     GSList* default_value)
{
    gchar** keys = g_key_file_get_keys(cfg, SECTION, NULL, NULL);
    if(!keys)
        return default_value;

    GSList* list = NULL;
    gint list_size = 0;
    for(gchar** key = keys; *key != NULL; ++key)
    {
        if(g_strcmp0(*key, "ui-file") == 0)
            continue;
        gint column = read_value_enum(cfg, SECTION, *key, COLUMN_STRINGS, -1);
        if(column >= 0)
        {
            ModelPropertyBinding* binding = g_malloc(sizeof(ModelPropertyBinding));
            list = g_slist_prepend(list, binding);
            list_size++;

            /* split key to "widget" and "property" */
            const gchar* key_del = g_strstr_len(*key, -1, ".");
            binding->widget = key_del ? g_strndup(*key, key_del - *key) : NULL;
            binding->prop = key_del ? g_strdup(key_del + 1) : g_strdup(*key);
            binding->column = column;
        }
    }

    g_strfreev(keys);
    g_slist_free_full(default_value, free_model_property_binding);
    return list;
}
#endif

static void read_appearance_section(GKeyFile* cfg,
                                    const gchar* SECTION,
                                    const gchar* path,
                                    GtkSettings* settings,
                                    const gchar* default_theme)
{
    gchar* base_theme_name = read_value_str(cfg, SECTION, "greeter-theme", default_theme);
    if(loaded_themes && base_theme_name && g_hash_table_contains(loaded_themes, base_theme_name))
    {
        const gchar* first_load_path = g_hash_table_lookup(loaded_themes, base_theme_name);
        g_warning("Reading base theme failed: theme \"%s\" already loaded, stopped to prevent infinite recursion. "
                  "Current path: \"%s\", first load: \"%s\"", base_theme_name, path, first_load_path);
        show_message_dialog(GTK_MESSAGE_ERROR, _("Error"),
                            _("Reading base theme failed: theme \"%s\" already loaded, stopped to prevent infinite recursion.\n\n"
                              "Current path: \"%s\"\n"
                              "first load: \"%s\""), base_theme_name, path, first_load_path);
        g_free(base_theme_name);
        base_theme_name = NULL;
    }

    LoadedGreeterConfig* theme = g_malloc(sizeof(LoadedGreeterConfig));
    if(base_theme_name)
    {
        GError* error = NULL;
        GKeyFile* theme_cfg = g_key_file_new();
        gchar* theme_filename = g_build_filename(GREETER_DATA_DIR, "themes", base_theme_name, "theme.conf", NULL);
        if(g_key_file_load_from_file(theme_cfg, theme_filename, G_KEY_FILE_NONE, &error))
        {
            if(!loaded_themes)
                loaded_themes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
            g_hash_table_insert(loaded_themes, g_strdup(base_theme_name), g_strdup(path));

            gchar* base_path = g_build_filename(GREETER_DATA_DIR, "themes", base_theme_name, NULL);
            read_appearance_section(theme_cfg, SECTION, base_path, settings, NULL);
            g_free(base_path);
        }
        else
        {
            g_warning("Failed to load theme: %s.", error->message);
            g_clear_error(&error);
        }
        g_free(theme_filename);
        g_key_file_free(theme_cfg);
        g_free(base_theme_name);
    }

    if(read_value_bool(cfg, SECTION, "css-file-reset", FALSE))
    {
        void free_theme(LoadedGreeterConfig* theme)
        {
            g_free(theme->css_path);
            g_free(theme->path);
            g_free(theme);
        }
        g_slist_free_full(config.appearance.themes_stack, (GDestroyNotify)free_theme);
        config.appearance.themes_stack = NULL;
    }

    theme->path = g_strdup(path);
    theme->css_path = read_value_path(cfg, SECTION, "css-file", NULL, path);

    config.appearance.themes_stack            = g_slist_prepend    (config.appearance.themes_stack, theme);
    config.appearance.ui_file                 = read_value_path    (cfg, SECTION, "ui-file", config.appearance.ui_file, path);
    config.appearance.background              = read_value_path    (cfg, SECTION, "background", config.appearance.background, path);
    config.appearance.user_background         = read_value_bool    (cfg, SECTION, "user-background", config.appearance.user_background);
    config.appearance.x_background            = read_value_bool    (cfg, SECTION, "x-background", config.appearance.x_background);
    config.appearance.logo                    = read_value_path    (cfg, SECTION, "logo", config.appearance.logo, path);
    config.appearance.user_image.enabled      = read_value_bool    (cfg, SECTION, "user-image", config.appearance.user_image.enabled);
    config.appearance.user_image.fit          = read_value_enum    (cfg, SECTION, "user-image-fit",
                                                                    USER_IMAGE_FIT_STRINGS, config.appearance.user_image.fit);
    config.appearance.list_image.enabled      = read_value_bool    (cfg, SECTION, "list-image", config.appearance.list_image.enabled);
    config.appearance.list_image.fit          = read_value_enum    (cfg, SECTION, "user-image-fit",
                                                                    USER_IMAGE_FIT_STRINGS, config.appearance.list_image.fit);
    config.appearance.list_image.size         = read_value_int     (cfg, SECTION, "list-image-size", config.appearance.list_image.size);

    config.appearance.gtk_theme               = read_value_str     (cfg, SECTION, "gtk-theme", config.appearance.gtk_theme);
    config.appearance.icon_theme              = read_value_str_gtk (cfg, SECTION, "icon-theme", settings, "gtk-icon-theme-name", config.appearance.icon_theme, FALSE);
    config.appearance.font                    = read_value_str_gtk (cfg, SECTION, "font-name", settings, "gtk-font-name", config.appearance.font, FALSE);
    config.appearance.fixed_login_button_width= read_value_bool    (cfg, SECTION, "fixed-login-button-width", config.appearance.fixed_login_button_width);
    config.appearance.hintstyle               = read_value_str_gtk (cfg, SECTION, "xft-hintstyle", settings, "gtk-xft-hintstyle", config.appearance.hintstyle, FALSE);
    config.appearance.rgba                    = read_value_str_gtk (cfg, SECTION, "xft-rgba", settings, "gtk-xft-rgba", config.appearance.rgba, FALSE);
    config.appearance.antialias               = read_value_bool_gtk(cfg, SECTION, "xft-antialias", settings, "gtk-xft-antialias", config.appearance.antialias, FALSE);
    config.appearance.dpi                     = read_value_dpi_gtk (cfg, SECTION, "xft-dpi", settings, "gtk-xft-dpi");
    config.appearance.user_name_format        = read_value_enum    (cfg, SECTION, "user-name-format",
                                                                    USER_NAME_FORMAT_STRINGS, config.appearance.user_name_format);
    config.appearance.date_format             = read_value_str     (cfg, SECTION, "date-format", config.appearance.date_format);
    config.appearance.position                = read_value_wp      (cfg, SECTION, "position", &config.appearance.position);
    config.appearance.position_is_relative    = read_value_bool    (cfg, SECTION, "position-is-relative", config.appearance.position_is_relative);
    config.appearance.hide_prompt_text        = read_value_bool    (cfg, SECTION, "hide-prompt-text", config.appearance.hide_prompt_text);
    config.appearance.default_user_image      = read_value_path    (cfg, SECTION, "default-user-image", config.appearance.default_user_image, GREETER_DATA_DIR);

    #if GTK_CHECK_VERSION(3, 10, 0)
    config.appearance.templates.session.bindings  = read_template_section(cfg, "appearance.session-template", SESSION_COLUMN_STRINGS,
                                                                          config.appearance.templates.session.bindings);
    config.appearance.templates.session.ui_file   = read_value_path(cfg, "appearance.session-template", "ui-file",
                                                                    config.appearance.templates.session.ui_file, path);
    config.appearance.templates.language.bindings = read_template_section(cfg, "appearance.language-template", LANGUAGE_COLUMN_STRINGS,
                                                                          config.appearance.templates.language.bindings);
    config.appearance.templates.language.ui_file  = read_value_path(cfg, "appearance.language-template", "ui-file",
                                                                    config.appearance.templates.language.ui_file, path);
    config.appearance.templates.user.bindings     = read_template_section(cfg, "appearance.user-template", USER_COLUMN_STRINGS,
                                                                          config.appearance.templates.user.bindings);
    config.appearance.templates.user.ui_file      = read_value_path(cfg, "appearance.user-template", "ui-file",
                                                                    config.appearance.templates.user.ui_file, path);
    #endif
}

static gboolean read_value_bool(GKeyFile* key_file,
                                const gchar* section,
                                const gchar* key,
                                gboolean default_value)
{
    GError* error = NULL;
    gboolean value = g_key_file_get_boolean(key_file, section, key, &error);
    if(!error)
        return value;
    g_clear_error(&error);
    return default_value;
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
    GError* error = NULL;
    gint value = g_key_file_get_integer(key_file, section, key, &error);
    if(!error)
        return value;
    g_clear_error(&error);
    return default_value;
}

static gchar* read_value_str(GKeyFile* key_file,
                             const gchar* section,
                             const gchar* key,
                             const gchar* default_value)
{
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
    gchar* value = g_key_file_get_value(key_file, section, key, NULL);
    if(value)
    {
        WindowPosition p;
        gchar *x = value;
        gchar *y = strchr(value, ' ');
        if(y)
            (y++)[0] = '\0';

        if(read_value_wp_dimension(x, &p.x))
            /* If there is no y-part then y = x */
            if (!y || !read_value_wp_dimension(y, &p.y))
                p.y = p.x;

        g_free(value);
        return p;
    }
    else
        return *default_value;
}

static gboolean read_value_wp_dimension(const gchar *s,
                                        WindowPositionDimension *x)
{
    WindowPositionDimension p;
    gchar *end = NULL;
    gchar **parts = g_strsplit(s, ",", 2);
    if(parts[0])
    {
        p.value = g_ascii_strtoll(parts[0], &end, 10);
        p.percentage = end && end[0] == '%';
        p.sign = (p.value < 0 || (p.value == 0 && parts[0][0] == '-')) ? -1 : +1;
        if(p.value < 0)
            p.value *= -1;
        if(g_strcmp0(parts[1], "start") == 0 || g_strcmp0(parts[1], "left") == 0)
            p.anchor = -1;
        else if(g_strcmp0(parts[1], "center") == 0)
            p.anchor = 0;
        else if(g_strcmp0(parts[1], "end") == 0 || g_strcmp0(parts[1], "right") == 0)
            p.anchor = +1;
        else
            p.anchor = p.sign > 0 ? -1 : +1;
        *x = p;
    }
    else
        x = NULL;
    g_strfreev (parts);
    return x != NULL;
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
    gchar* value = read_value_str(key_file, section, key, NULL);
    if(value)
    {
        gchar** array = NULL;
        g_shell_parse_argv(value, NULL, &array, NULL);
        g_free(value);
        return array;
    }
    return NULL;
}

static gint read_value_dpi_gtk(GKeyFile* key_file,
                                  const gchar* section,
                                  const gchar* key,
                                  GtkSettings* settings,
                                  const gchar* property)
{
    g_return_val_if_fail(settings != NULL, 0);
    g_return_val_if_fail(property != NULL, 0);

    GError* error = NULL;
    gint value = g_key_file_get_integer(key_file, section, key, &error);
    if(!error)
        g_object_set(settings, property, 1024*value, NULL);
    g_object_get(settings, property, &value, NULL);
    g_clear_error(&error);
    return value/1024;
}

static gchar* read_value_path(GKeyFile* key_file,
                              const gchar* section,
                              const gchar* key,
                              const gchar* default_value,
                              const gchar* dir)
{
    gchar* value = read_value_str(key_file, section, key, default_value);
    if(dir && value && value[0] && value[0] != '#' && !g_path_is_absolute(value))
    {
        gchar* abs_path = g_build_filename(dir, value, NULL);
        g_free(value);
        value = abs_path;
    }
    return value;
}

static GtkStyleProvider* read_css_file(const gchar* path,
                                       GdkScreen* screen)
{
    g_message("Loading CSS file: %s", path);
    GError* error = NULL;
    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    if(!gtk_css_provider_load_from_path(provider, path, &error))
    {
        g_warning("Error loading CSS: %s", error->message);
        g_clear_error(&error);
    }
    return GTK_STYLE_PROVIDER(provider);
}

/* TODO: move it to shares.c and mark "conf" chnages */
static void read_templates(void)
{
    struct Template
    {
        GSList** bindings;
        const ModelPropertyBinding* default_bindings;
        #if GTK_CHECK_VERSION(3, 10, 0)
        GBytes** data;
        gchar* ui_file;
        #endif
    };

    struct Template templates[] =
    {
        {
            &config.appearance.templates.session.bindings,
            SESSION_TEMPLATE_DEFAULT_BINDINGS,
            #if GTK_CHECK_VERSION(3, 10, 0)
            &config.appearance.templates.session.data,
            config.appearance.templates.session.ui_file
            #endif

        },
        {
            &config.appearance.templates.language.bindings,
            LANGUAGE_TEMPLATE_DEFAULT_BINDINGS,
            #if GTK_CHECK_VERSION(3, 10, 0)
            &config.appearance.templates.language.data,
            config.appearance.templates.language.ui_file
            #endif

        },
        {
            &config.appearance.templates.user.bindings,
            USER_TEMPLATE_DEFAULT_BINDINGS,
            #if GTK_CHECK_VERSION(3, 10, 0)
            &config.appearance.templates.user.data,
            config.appearance.templates.user.ui_file
            #endif
        },
        #if GTK_CHECK_VERSION(3, 10, 0)
        {NULL, NULL, NULL, NULL}
        #else
        {NULL, NULL}
        #endif
    };

    #if GTK_CHECK_VERSION(3, 10, 0)
    GHashTableIter iter;
    gchar* key;
    GHashTable* table = g_hash_table_new(g_str_hash, g_str_equal);

    for(struct Template* template = templates; template->data != NULL; template++)
        if(template->ui_file)
            g_hash_table_insert(table, template->ui_file, NULL);

    g_hash_table_iter_init(&iter, table);
    while(g_hash_table_iter_next(&iter, (gpointer)&key, NULL))
        if(key)
        {
            gchar* str;
            gsize str_size;
            if(g_file_get_contents(key, &str, &str_size, NULL))
                g_hash_table_iter_replace(&iter, g_bytes_new_take(str, str_size));
        }
    #endif

    for(struct Template* template = templates; template->bindings != NULL; template++)
    {
        #if GTK_CHECK_VERSION(3, 10, 0)
        if(template->ui_file)
            *template->data = g_hash_table_lookup(table, template->ui_file);
        /* Default bindings for non-composite implementations */
        if(!*template->data && template->default_bindings)
        #else
        if(template->default_bindings)
        #endif
        {
            if(*template->bindings)
            {
                g_slist_free_full(*template->bindings, free_model_property_binding);
                *template->bindings = NULL;
            }
            for(const ModelPropertyBinding* bind = template->default_bindings; bind->prop != NULL; bind++)
                *template->bindings = g_slist_prepend(*template->bindings, g_memdup(bind, sizeof(ModelPropertyBinding)));
        }
    }
    #if GTK_CHECK_VERSION(3, 10, 0)
    g_hash_table_unref(table);
    #endif
}
