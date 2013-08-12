/* shares.h
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

#ifndef _SHARES_H_INCLUDED_
#define _SHARES_H_INCLUDED_

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <lightdm.h>

/* Types */

struct _WindowPosition
{
    gboolean x_is_absolute;
    gboolean y_is_absolute;

    struct
    {
        /* -1: start, 0: center, +1: end */
        int width;
        int height;
    } anchor;
    int x;
    int y;
};

typedef struct _WindowPosition WindowPosition;

typedef enum
{
    POWER_SUSPEND,
    POWER_HIBERNATE,
    POWER_RESTART,
    POWER_SHUTDOWN,
    POWER_ACTIONS_COUNT
} PowerAction;

typedef struct
{
    LightDMGreeter* greeter;
    struct
    {
        GHashTable*     users_display_names;
        gboolean        prompted;
        gboolean        cancelling;
        const gchar*    last_background;
        GPid            autostart_pid;
        struct
        {
            WindowPosition position;
        } panel;
        struct
        {
            GdkPixbuf*  default_image;
            gint        size;
        } user_image, list_image;
    } state;

    struct
    {
        GtkWidget*      login_window;
        GtkWidget*      panel_window;
        GtkWidget*      panel_menubar;

        GtkWidget*      login_widget;
        GtkWidget*      login_label;
        GtkWidget*      login_box;

        GtkWidget*      cancel_widget;
        GtkWidget*      cancel_box;

        GtkWidget*      message_widget;
        GtkWidget*      message_box;

        GtkWidget*      prompt_entry;
        GtkWidget*      prompt_text;
        GtkWidget*      prompt_box;

        GtkWidget*      users_widget;
        GtkWidget*      users_box;

        GtkWidget*      sessions_widget;
        GtkWidget*      sessions_box;

        GtkWidget*      languages_widget;
        GtkWidget*      languages_box;

        GtkWidget*      authentication_widget;
        GtkWidget*      authentication_box;

        GtkWidget*      user_image_widget;
        GtkWidget*      user_image_box;

        GtkWidget*      date_widget;
        GtkWidget*      date_box;

        GtkWidget*      host_widget;
        GtkWidget*      host_box;

        GtkWidget*      logo_image_widget;
        GtkWidget*      logo_image_box;

        GtkWidget*      onboard;
        GtkWidget*      calendar;

        struct
        {
            GtkWidget*  widget;
            GtkWidget*  box;
            GtkWidget*  menu;
            GtkWidget*  actions[POWER_ACTIONS_COUNT];
            GtkWidget*  actions_box[POWER_ACTIONS_COUNT];
        } power;

        struct
        {
            GtkWidget*  widget;
            GtkWidget*  box;
            GtkWidget*  menu;
            GtkWidget*  osk_widget;
            GtkWidget*  osk_box;
            GtkWidget*  contrast_widget;
            GtkWidget*  contrast_box;
            GtkWidget*  font_widget;
            GtkWidget*  font_box;
            GtkWidget*  dpi_widget;
            GtkWidget*  dpi_box;
        } a11y;

        struct
        {
            GtkWidget*  time_widget;
            GtkWidget*  time_box;
            GtkWidget*  time_menu;
            GtkWidget*  date_widget;
            GtkWidget*  date_box;
            GtkWidget*  calendar_widget;
        } clock;

        struct
        {
            GtkWidget*  widget;
            GtkWidget*  box;
            GtkWidget*  menu;
        } layout;
    } ui;
} GreeterData;

typedef enum
{
    USER_COLUMN_NAME = 0,
    USER_COLUMN_TYPE,
    USER_COLUMN_DISPLAY_NAME,
    USER_COLUMN_WEIGHT,
    USER_COLUMN_USER_IMAGE,
    USER_COLUMN_LIST_IMAGE
} UsersModelColumn;

typedef enum
{
    USER_TYPE_REGULAR = 0,
    USER_TYPE_GUEST,
    USER_TYPE_OTHER
} UserType;

typedef enum
{
    SESSION_COLUMN_NAME = 0,
    SESSION_COLUMN_DISPLAY_NAME,
    SESSION_COLUMN_IMAGE,
    SESSION_COLUMN_COMMENT
} SessionsModelColumn;

typedef enum
{
    LANGUAGE_COLUMN_CODE = 0,
    LANGUAGE_COLUMN_DISPLAY_NAME
} LanguagesModelColumn;

/* Variables */

extern GreeterData greeter;
extern const gchar* const USER_GUEST;
extern const gchar* const USER_OTHER;
extern const gchar* const APP_NAME;
extern const gchar* const DEFAULT_USER_ICON;
extern const gchar* const ACTION_TEXT_LOGIN;
extern const gchar* const ACTION_TEXT_UNLOCK;

extern const WindowPosition WINDOW_POSITION_CENTER;
extern const WindowPosition WINDOW_POSITION_TOP;
extern const WindowPosition WINDOW_POSITION_BOTTOM;

#ifdef _DEBUG_
extern gchar* GETTEXT_PACKAGE;
extern gchar* LOCALE_DIR;
extern gchar* GREETER_DATA_DIR;
extern gchar* CONFIG_FILE;
extern gchar* PACKAGE_VERSION;
#endif

/* Functions */

/* Set positions of all application windows according to settings and current program state */
void update_windows_layout             (void);

void show_message                      (const gchar* title,
                                        const gchar* message_format,
                                        ...) G_GNUC_PRINTF (2, 3);
void show_error                        (const gchar* title,
                                        const gchar* message_format,
                                        ...) G_GNUC_PRINTF (2, 3);

void set_window_position               (GtkWidget* window,
                                        const WindowPosition* p);
void set_widget_text                   (GtkWidget* widget,
                                        const gchar* text);
GtkTreeModel* get_widget_model         (GtkWidget* widget);
gchar* get_widget_selection_str        (GtkWidget* widget,
                                        gint column,
                                        const gchar* default_value);
GdkPixbuf* get_widget_selection_image  (GtkWidget* widget,
                                        gint column,
                                        GdkPixbuf* default_value);
gint get_widget_selection_int          (GtkWidget* widget,
                                        gint column,
                                        gint default_value);
gboolean get_widget_active_iter        (GtkWidget* widget,
                                        GtkTreeIter* iter);
void set_widget_active_iter            (GtkWidget* widget,
                                        GtkTreeIter* iter);
void set_widget_active_first           (GtkWidget* widget);

gboolean get_model_iter_match          (GtkTreeModel* model,
                                        int column,
                                        gboolean f(const gpointer),
                                        GtkTreeIter* iter);
gboolean get_model_iter_str            (GtkTreeModel* model,
                                        int column,
                                        const gchar* value,
                                        GtkTreeIter* iter);
void fix_image_menu_item_if_empty      (GtkImageMenuItem* widget);
gboolean get_widget_toggled            (GtkWidget* widget);
void set_widget_toggled                (GtkWidget* widget,
                                        gboolean state,
                                        GCallback suppress_callback);

void setup_window                      (GtkWindow* window);

#endif // _SHARES_H_INCLUDED_
