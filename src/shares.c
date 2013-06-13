/* shares.c
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

#include "shares.h"
#include <string.h>

/* Variables */

GreeterData greeter = {NULL, };
const gchar* const USER_GUEST = "*guest";
const gchar* const USER_OTHER = "*other";

const gchar* const APP_NAME = "lightdm-another-gtk-greeter";
const gchar* const DEFAULT_USER_ICON = "avatar-default";

const WindowPosition WINDOW_POSITION_CENTER =
{
    .x_is_absolute = FALSE,
    .x = 50,
    .y_is_absolute = FALSE,
    .y = 50,
    .anchor = {.width=0, .height=0}
};

#ifdef _DEBUG_
const gchar* const GETTEXT_PACKAGE = "lightdm-another-gtk-greeter";
const gchar* const LOCALE_DIR = "/usr/local/share.locale";
const gchar* const GREETER_DATA_DIR = "../../data";
const gchar* const CONFIG_FILE = "../../data/lightdm-another-gtk-greeter.conf";
const gchar* const PACKAGE_VERSION = "<DEBUG>";
#endif

/* Static functions */

static void UNSUPPORTED_WIDGET(const gchar* func,
                               GtkWidget* widget)
{
    if(widget)
        g_critical("%s(%s): unsupported widget", func, gtk_widget_get_name(widget));
}

static gboolean _grab_focus(GtkWidget* widget)
{
    gtk_widget_grab_focus(widget);
    return FALSE;
}

/* ---------------------------------------------------------------------------*
 * Definitions: public
 * -------------------------------------------------------------------------- */

void show_error_and_exit(const gchar* message_format, ...)
{
    GtkWidget* dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_NONE,
                                    "%s", message_format);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           "cancel", GTK_RESPONSE_CANCEL,
                           "ok", GTK_RESPONSE_OK, NULL);
    gtk_widget_show_all(dialog);
    center_window(dialog);

    gtk_dialog_run(GTK_DIALOG(dialog));
}

void center_window(GtkWidget* window)
{
    set_window_position(window, &WINDOW_POSITION_CENTER);
}

void set_window_position(GtkWidget* window,
                         const WindowPosition* p)
{
    GdkScreen* screen;
    GtkRequisition size;
    GdkRectangle monitor_geometry;
    gint dx, dy;

    screen = gtk_window_get_screen(GTK_WINDOW(window));
    gdk_screen_get_monitor_geometry(screen, gdk_screen_get_primary_monitor(screen), &monitor_geometry);
    gtk_widget_get_preferred_size(window, NULL, &size);

    dx = p->x_is_absolute ? (p->x < 0 ? monitor_geometry.width + p->x : p->x) : (monitor_geometry.width)*p->x/100.0;
    dy = p->y_is_absolute ? (p->y < 0 ? monitor_geometry.height + p->y : p->y) : (monitor_geometry.height)*p->y/100.0;

    if(p->anchor.width == 0)
        dx -= size.width/2;
    else if(p->anchor.width > 0)
        dx -= size.width;

    if(p->anchor.height == 0)
        dy -= size.height/2;
    else if(p->anchor.height > 0)
        dy -= size.height;

    gtk_window_move(GTK_WINDOW(window), monitor_geometry.x + dx, monitor_geometry.y + dy);
}

void set_widget_text(GtkWidget* widget,
                     const gchar* text)
{
    if(GTK_IS_MENU_ITEM(widget))
        gtk_menu_item_set_label(GTK_MENU_ITEM(widget), text);
    else if(GTK_IS_BUTTON(widget))
        gtk_button_set_label(GTK_BUTTON(widget), text);
    else if(GTK_IS_LABEL(widget))
        gtk_label_set_label(GTK_LABEL(widget), text);
    else UNSUPPORTED_WIDGET(__func__, widget);
}

GdkPixbuf* scale_image(GdkPixbuf* source,
                       int new_width)
{
    GdkPixbuf* image = source;
    if(new_width > 0)
    {
        int old_w = gdk_pixbuf_get_width(source);
        if(old_w > new_width)
        {
            int new_h = (1.0*new_width/old_w)*gdk_pixbuf_get_height(source);
            image = gdk_pixbuf_scale_simple(source, new_width, new_h, GDK_INTERP_HYPER);
        }
    }
    return image;
}

void grab_widget_focus(GtkWidget* widget)
{
    g_idle_add((GSourceFunc)_grab_focus, widget);
}

GtkTreeModel* get_widget_model(GtkWidget* widget)
{
    if(GTK_IS_COMBO_BOX(widget))
        return gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    if(GTK_IS_TREE_VIEW(widget))
        return gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
    UNSUPPORTED_WIDGET(__func__, widget);
    return NULL;
}

gchar* get_widget_selection_str(GtkWidget* widget,
                                gint column,
                                const gchar* default_value)
{
    GtkTreeIter iter;
    if(!get_widget_active_iter(widget, &iter))
        return default_value ? g_strdup(default_value) : NULL;
    gchar* value;
    gtk_tree_model_get(get_widget_model(widget), &iter, column, &value, -1);
    return value;
}

GdkPixbuf* get_widget_selection_image(GtkWidget* widget,
                                      gint column,
                                      GdkPixbuf* default_value)
{
    GtkTreeIter iter;
    if(!get_widget_active_iter(widget, &iter))
        return default_value;
    GdkPixbuf* value;
    gtk_tree_model_get(get_widget_model(widget), &iter, column, &value, -1);
    return value;
}

gint get_widget_selection_int(GtkWidget* widget,
                              gint column,
                              gint default_value)
{
    GtkTreeIter iter;
    if(get_widget_active_iter(widget, &iter))
        gtk_tree_model_get(get_widget_model(widget), &iter, column, &default_value, -1);
    return default_value;
}

gboolean get_widget_active_iter(GtkWidget* widget,
                                GtkTreeIter* iter)
{
    if(GTK_IS_COMBO_BOX(widget))
        return gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), iter);
    if(GTK_IS_TREE_VIEW(widget))
        return gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), NULL, iter);
    UNSUPPORTED_WIDGET(__func__, widget);
    return FALSE;
}

void set_widget_active_iter(GtkWidget* widget,
                            GtkTreeIter* iter)
{
    if(GTK_IS_COMBO_BOX(widget))
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), iter);
    else if(GTK_IS_TREE_VIEW(widget))
    {
        gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), iter);
        GtkTreePath* path = gtk_tree_model_get_path(get_widget_model(widget), iter);
        if(path)
        {
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(widget), path, NULL, FALSE, 0.0, 0.0);
            gtk_tree_path_free(path);
        }
    }
    else UNSUPPORTED_WIDGET(__func__, widget);
}

void set_widget_active_first(GtkWidget* widget)
{
    if(GTK_IS_COMBO_BOX(widget))
        gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
    else if(GTK_IS_TREE_VIEW(widget))
    {
        GtkTreeIter iter;
        if(gtk_tree_model_get_iter_first(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)), &iter))
            gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), &iter);
        gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(widget), 0, 0);
    }
    else UNSUPPORTED_WIDGET(__func__, widget);
}

gboolean get_model_iter_str(GtkTreeModel* model,
                            int column,
                            const gchar* value,
                            GtkTreeIter* iter)
{
    if(!gtk_tree_model_get_iter_first(model, iter))
        return FALSE;

    gchar* iter_value;
    do
    {
        gtk_tree_model_get(model, iter, column, &iter_value, -1);
        gboolean matched = g_strcmp0(iter_value, value) == 0;
        g_free(iter_value);
        if(matched)
            return TRUE;
    } while(gtk_tree_model_iter_next(model, iter));

    return FALSE;
}

void replace_container_content(GtkWidget* widget,
                               GtkWidget* new_content)
{
    gtk_container_foreach(GTK_CONTAINER(widget), (GtkCallback)gtk_widget_destroy, NULL);
    gtk_widget_reparent(new_content, widget);
}

/* ---------------------------------------------------------------------------*
 * Definitions: static
 * -------------------------------------------------------------------------- */


