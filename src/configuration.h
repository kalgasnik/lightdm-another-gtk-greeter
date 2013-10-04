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

typedef enum
{
    USER_IMAGE_FIT_NONE,
    USER_IMAGE_FIT_ALL,
    USER_IMAGE_FIT_BIGGER,
    USER_IMAGE_FIT_SMALLER
} UserImageFit;

typedef enum
{
    USER_IMAGE_TYPE_NONE,
    USER_IMAGE_TYPE_SOURCE,
    USER_IMAGE_TYPE_FIXED
} UserImageType;

typedef struct
{
    struct
    {
        gboolean        allow_other_users;
        gboolean        show_language_selector;
        gboolean        show_session_icon;
        guint32         double_escape_time;
        gchar**         autostart_command;
        gboolean        allow_password_toggle;
    } greeter;

    struct
    {
        gchar*          ui_file;
        gchar*          css_file;
        gchar*          gtk_theme;
        gchar*          icon_theme;
        gchar*          background;
        gboolean        user_background;
        gboolean        x_background;
        gchar*          logo;
        UserNameFormat  user_name_format;
        gchar*          date_format;
        gboolean        fixed_login_button_width;
        gchar*          font;
        gchar*          hintstyle;
        gchar*          rgba;
        gboolean        antialias;
        gint            dpi;
        gboolean        transparency;
        WindowPosition  position;
        /* True: position is relative main_layout, False: is absolute */
        gboolean        position_is_relative;
        struct
        {
            gboolean     enabled;
            UserImageFit fit;
            gint         size;   /* Only for list_image */
        } user_image, list_image;
        gboolean        invert_password_state; /* set to 1 if gui provide "hide password" widget instead of "show password" */
    } appearance;

    struct
    {
        gboolean        enabled;
        PanelPosition   position;
    } panel;

    struct
    {
        gboolean        enabled;
        gboolean        prompts[POWER_ACTIONS_COUNT];
    } power;

    struct
    {
        gboolean        enabled;
        gboolean        calendar;
        gchar*          time_format;
        gchar*          date_format;
    } clock;

    struct
    {
        gboolean        enabled;

        struct
        {
            gboolean    enabled;
            gchar*      gtk_theme;
            gchar*      icon_theme;
            gboolean    initial_state;
        } contrast;

        struct
        {
            gboolean    enabled;
            gchar**     command;
            gboolean    use_onboard;
            gboolean    initial_state;

            OnboardPosition onboard_position;
            gint        onboard_height;
            gboolean    onboard_height_is_percent;
        } osk;

        struct
        {
            gboolean    enabled;
            gint        increment;
            gboolean    is_percent;
            gboolean    initial_state;
        } font;

        struct
        {
            gboolean    enabled;
            gint        increment;
            gboolean    is_percent;
            gboolean    initial_state;
        } dpi;
    } a11y;

    struct
    {
        gboolean        enabled;
        gboolean        enabled_for_one;
    } layout;
} GreeterConfig;

/* Variables */

extern GreeterConfig config;

/* Functions */

void load_settings               (void);
void read_state                  (void);

gchar* get_state_value_str       (const gchar* section,
                                  const gchar* key);
void set_state_value_str         (const gchar* section,
                                  const gchar* key,
                                  const gchar* value);
gint get_state_value_int         (const gchar* section,
                                  const gchar* key);
void set_state_value_int         (const gchar* section,
                                  const gchar* key,
                                  gint value);

#endif // _CONFIGURAION_H_INCLUDED_
