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

struct _GreeterData
{
    LightDMGreeter* greeter;

    struct
    {
        GHashTable* users_display_names;
        gboolean prompted;
        gboolean cancelling;
        GdkPixbuf* default_user_image;
        GdkPixbuf* default_user_image_scaled;
    } state;

    struct
    {
        GtkWidget* login_window;
        GtkWidget* panel_window;
        GtkWidget* login_widget;
        GtkWidget* cancel_widget;
        GtkWidget* message_widget;
        GtkWidget* prompt_widget;
        GtkWidget* user_image;
        GtkWidget* date_widget;

        GtkWidget* user_view;
        GtkWidget* session_view;
        GtkWidget* language_view;
        GtkWidget* user_view_box;

        GtkWidget* calendar;

        struct
        {
            GtkWidget* power_widget;
            GtkWidget* power_menu;
            GtkWidget* power_menu_icon;
            GtkWidget* suspend_widget;
            GtkWidget* hibernate_widget;
            GtkWidget* restart_widget;
            GtkWidget* shutdown_widget;
        } power;

        struct
        {
            GtkWidget* a11y_widget;
            GtkWidget* a11y_menu;
            GtkWidget* a11y_menu_icon;
            GtkWidget* osk_widget;
        } a11y;

        struct
        {
            GtkWidget* time_widget;
            GtkWidget* time_menu;
            GtkWidget* date_widget;
            GtkWidget* calendar_widget;
        } clock;

        struct
        {
            GtkWidget* layout_widget;
            GtkWidget* layout_menu;
        } layout;

        struct
        {
            GtkWidget* menubar;
        } panel;

        GtkWidget* login_box;
        GtkWidget* prompt_box;
        GtkWidget* prompt_entry;
        GtkWidget* host_widget;
        GtkWidget* logo_image;
    } ui;
};

typedef struct _GreeterData GreeterData;

typedef enum
{
    USER_COLUMN_NAME = 0,
    USER_COLUMN_TYPE,
    USER_COLUMN_DISPLAY_NAME,
    USER_COLUMN_WEIGHT,
    USER_COLUMN_IMAGE,
    USER_COLUMN_IMAGE_SCALED
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
    SESSION_COLUMN_IMAGE
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
#ifdef _DEBUG_
extern const gchar* const GETTEXT_PACKAGE;
extern const gchar* const LOCALE_DIR;
extern const gchar* const GREETER_DATA_DIR;
extern const gchar* const CONFIG_FILE;
extern const gchar* const PACKAGE_VERSION;
#endif

/* Functions */

void show_error_and_exit(const gchar* message_format, ...) G_GNUC_PRINTF (1, 2);
void center_window(GtkWidget* window);
void set_widget_text(GtkWidget* widget, const gchar* text);
GdkPixbuf* scale_image(GdkPixbuf* source, int new_width);
void grab_widget_focus(GtkWidget* widget);

GtkTreeModel* get_widget_model(GtkWidget* widget);
gchar* get_widget_selection_str(GtkWidget* widget, gint column, const gchar* default_value);
GdkPixbuf* get_widget_selection_image(GtkWidget* widget, gint column, GdkPixbuf* default_value);
gint get_widget_selection_int(GtkWidget* widget, gint column, gint default_value);
gboolean get_widget_active_iter(GtkWidget* widget, GtkTreeIter* iter);
void set_widget_active_iter(GtkWidget* widget, GtkTreeIter* iter);
void set_widget_active_first(GtkWidget* widget);

gboolean get_model_iter_match(GtkTreeModel* model, int column, gboolean f(const gpointer), GtkTreeIter* iter);
gboolean get_model_iter_str(GtkTreeModel* model, int column, const gchar* value, GtkTreeIter* iter);
void replace_container_content(GtkWidget* widget, GtkWidget* new_content);

#endif // _SHARES_H_INCLUDED_
