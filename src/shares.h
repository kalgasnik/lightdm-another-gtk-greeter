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

        gboolean        password_required;
        gboolean        show_password;
        GdkPixbuf*      window_background;
    } state;

    struct
    {
        GtkWidget*      screen_window;
        GtkWidget*      screen_layout;

        GtkWidget*      main_content;
        GtkWidget*      main_layout;
        GtkWidget*      panel_content;
        GtkWidget*      panel_layout;
        GtkWidget*      panel_menubar;
        GtkWidget*      onboard_content;
        GtkWidget*      onboard_layout;

        GtkWidget*      messagebox_content;
        GtkWidget*      messagebox_layout;
        GtkWidget*      messagebox_title;
        GtkWidget*      messagebox_text;
        GtkWidget*      messagebox_buttons;
        GtkWidget*      messagebox_icon;

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

        GtkWidget*      password_toggle_widget;
        GtkWidget*      password_toggle_box;

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

typedef struct
{
    const gchar* text;
    const gchar* text_stock_icon;
    const gchar* stock;
    gint id;
} MessageButtonOptions;

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

#define UI_LAYOUT_WIDTH                 1

enum
{
    UI_LAYOUT_ROW_PANEL_TOP = 0,
    UI_LAYOUT_ROW_ONBOARD_TOP,
    UI_LAYOUT_ROW_MAIN,
    UI_LAYOUT_ROW_MESSAGE,
    UI_LAYOUT_ROW_ONBOARD_BOTTOM,
    UI_LAYOUT_ROW_PANEL_BOTTOM,
};

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

void show_message_dialog               (GtkMessageType type,
                                        const gchar* title,
                                        const gchar* message_format,
                                        ...) G_GNUC_PRINTF (3, 4);
gint show_message                      (const gchar* title,
                                        const gchar* message,
                                        const gchar* icon_name,
                                        const gchar* icon_stock,
                                        const MessageButtonOptions* buttons,
                                        gint default_id,
                                        gint cancel_id,
                                        ...) G_GNUC_PRINTF (1, 8);
void rearrange_grid_child              (GtkGrid* grid,
                                        GtkWidget* child,
                                        gint row);
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
void clear_container                   (GtkContainer* container);
/* Set positions of main window according to settings and current program state */
void update_main_window_layout         (void);

#endif // _SHARES_H_INCLUDED_
