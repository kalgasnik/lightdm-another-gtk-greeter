/* configuration.h
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

#ifndef _CONFIGURAION_H_INCLUDED_
#define _CONFIGURAION_H_INCLUDED_

#include <glib.h>

#include "shares.h"

/* Types */

typedef enum
{
    USER_NAME_FORMAT_NAME,
    USER_NAME_FORMAT_DISPLAYNAME,
    USER_NAME_FORMAT_BOTH
} UserNameFormat;

typedef enum
{
    ONBOARD_POS_TOP,
    ONBOARD_POS_BOTTOM,
    ONBOARD_POS_PANEL,
    ONBOARD_POS_PANEL_OPPOSITE
} OnboardPosition;

typedef enum
{
    PANEL_POS_TOP,
    PANEL_POS_BOTTOM
} PanelPosition;

struct _GreeterConfig
{
    struct
    {
        gboolean allow_other_users;
        gboolean show_language_selector;
        gboolean show_session_icon;
        WindowPosition position;
    } greeter;
    struct
    {
        gchar* ui_file;
        gchar* css_file;
        gchar* font;
        gchar* theme;
        gchar* icon_theme;
        gchar* background;
        gboolean user_background;
        gchar* logo;
        gboolean fixed_user_image_size;
        gint list_view_image_size;
        gint default_user_image_size;
        UserNameFormat user_name_format;
        gchar* date_format;
        gboolean fixed_login_button_width;
    } appearance;

    struct
    {
        gboolean enabled;
        PanelPosition position;
    } panel;

    struct
    {
        gboolean enabled;
        gboolean suspend_prompt;
        gboolean hibernate_prompt;
        gboolean restart_prompt;
        gboolean shutdown_prompt;
    } power;
    struct
    {
        gboolean enabled;
        gboolean calendar;
        gchar* time_format;
        gchar* date_format;
    } clock;
    struct
    {
        gboolean enabled;
        gchar* theme_contrast;
        gchar* icon_theme_contrast;
        gboolean check_theme;
        gchar** osk_command_array;
        gboolean osk_use_onboard;

        gint font_increment;
        gboolean font_increment_is_percent;
    } a11y;
    struct
    {
        gboolean enabled;
    } layout;
    struct
    {
        OnboardPosition position;
        gint height;
        gboolean height_is_percent;
    } onboard;
};

typedef struct _GreeterConfig GreeterConfig;

/* Variables */

extern GreeterConfig config;

/* Functions */

gboolean load_settings(void);

gchar* get_last_logged_user(void);
void save_last_logged_user(const gchar* user);

#endif // _CONFIGURAION_H_INCLUDED_
