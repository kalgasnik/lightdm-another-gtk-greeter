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

#include <string.h>
#include "shares.h"
#include "configuration.h"

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

const WindowPosition WINDOW_POSITION_TOP =
{
    .x_is_absolute = FALSE,
    .x = 50,
    .y_is_absolute = TRUE,
    .y = 0,
    .anchor = {.width=0, .height=-1}
};

const WindowPosition WINDOW_POSITION_BOTTOM =
{
    .x_is_absolute = FALSE,
    .x = 50,
    .y_is_absolute = FALSE,
    .y = 100,
    .anchor = {.width=0, .height=1}
};

#ifdef _DEBUG_
const gchar* const GETTEXT_PACKAGE = "lightdm-another-gtk-greeter";
const gchar* const LOCALE_DIR = "/usr/local/share.locale";
const gchar* const GREETER_DATA_DIR = "../../data";
const gchar* const CONFIG_FILE = "../../data/lightdm-another-gtk-greeter.conf";
const gchar* const PACKAGE_VERSION = "<DEBUG>";
#endif

/* Static functions */

static void show_message_dialog        (GtkMessageType type,
                                        const gchar* title,
                                        const gchar* message_format,
                                        va_list args);

static gboolean _grab_focus            (GtkWidget* widget);


/* ---------------------------------------------------------------------------*
 * Definitions: public
 * -------------------------------------------------------------------------- */

void update_windows_layout(void)
{
    GdkRectangle geometry;
    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(greeter.ui.login_window));
    gdk_screen_get_monitor_geometry(screen, gdk_screen_get_primary_monitor(screen), &geometry);

    //gtk_widget_set_size_request(greeter.ui.login_window,-1, -1);
    set_window_position(greeter.ui.login_window, &config.greeter.position);

    if(config.panel.enabled)
    {
        gtk_widget_set_size_request(greeter.ui.panel_window, geometry.width, -1);
        set_window_position(greeter.ui.panel_window, &greeter.state.panel.position);
    }

    if(greeter.ui.onboard && gtk_widget_get_visible(GTK_WIDGET(greeter.ui.onboard)))
    {
        gboolean panel_at_top = config.panel.position == PANEL_POS_TOP;
        gboolean at_top;
        switch(config.onboard.position)
        {
            case ONBOARD_POS_PANEL: at_top = panel_at_top; break;
            case ONBOARD_POS_PANEL_OPPOSITE: at_top =  !panel_at_top; break;
            case ONBOARD_POS_BOTTOM: at_top = FALSE; break;
            case ONBOARD_POS_TOP: ;
            default: at_top = TRUE;
        };

        gint onboard_height = config.onboard.height_is_percent ? geometry.height*config.onboard.height/100 : config.onboard.height;
        gint onboard_y = at_top ? 0 : geometry.height - onboard_height;

        if(at_top ==  panel_at_top
           && greeter.ui.panel_window
           && gtk_widget_get_visible(greeter.ui.panel_window))
        {
            gint panel_height;
            gtk_widget_get_preferred_height(greeter.ui.panel_window, NULL, &panel_height);
            onboard_y += at_top ? + panel_height : -panel_height;
        }

        gtk_window_resize(greeter.ui.onboard, geometry.width - geometry.x, onboard_height);
        gtk_window_move(greeter.ui.onboard, geometry.x, geometry.y + onboard_y);

        gint login_x, login_y, login_height;

        gtk_window_get_position(GTK_WINDOW(greeter.ui.login_window), &login_x, &login_y);
        gtk_widget_get_preferred_height(greeter.ui.login_window, NULL, &login_height);

        gint new_login_y = login_y;
        if(at_top && login_y < onboard_y + onboard_height)
            new_login_y = onboard_y + onboard_height + 5;
        else if(!at_top && login_y + login_height > onboard_y)
            new_login_y = onboard_y - login_height - 5;
        gtk_window_move(GTK_WINDOW(greeter.ui.login_window), login_x, new_login_y);
    }
}

void show_message(const gchar* title,
                  const gchar* message_format,
                  ...)
{
    va_list argptr;
    va_start(argptr, message_format);
    show_message_dialog(GTK_MESSAGE_INFO, title, message_format, argptr);
    va_end(argptr);
}

void show_error(const gchar* title,
                const gchar* message_format,
                ...)
{
    va_list argptr;
    va_start(argptr, message_format);
    show_message_dialog(GTK_MESSAGE_ERROR, title, message_format, argptr);
    va_end(argptr);
}

void set_window_position(GtkWidget* window,
                         const WindowPosition* p)
{
    GdkScreen* screen;
    GtkRequisition size;
    GdkRectangle geometry;
    gint dx, dy;

    screen = gtk_window_get_screen(GTK_WINDOW(window));
    gdk_screen_get_monitor_geometry(screen, gdk_screen_get_primary_monitor(screen), &geometry);
    gtk_widget_get_preferred_size(window, NULL, &size);

    dx = p->x_is_absolute ? (p->x < 0 ? geometry.width + p->x : p->x) : (geometry.width)*p->x/100.0;
    dy = p->y_is_absolute ? (p->y < 0 ? geometry.height + p->y : p->y) : (geometry.height)*p->y/100.0;

    if(p->anchor.width == 0)
        dx -= size.width/2;
    else if(p->anchor.width > 0)
        dx -= size.width;

    if(p->anchor.height == 0)
        dy -= size.height/2;
    else if(p->anchor.height > 0)
        dy -= size.height;

    gtk_window_move(GTK_WINDOW(window), geometry.x + dx, geometry.y + dy);
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
    else g_return_val_if_reached(NULL);
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
    g_return_val_if_reached(NULL);
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
    g_return_val_if_reached(NULL);
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
    else g_return_val_if_reached(NULL);
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
    else g_return_val_if_reached(NULL);
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

void fix_image_menu_item_if_empty(GtkImageMenuItem* widget)
{
    if(GTK_IS_IMAGE_MENU_ITEM(widget) &&
       (!gtk_menu_item_get_label(GTK_MENU_ITEM(widget)) ||
        strlen(gtk_menu_item_get_label(GTK_MENU_ITEM(widget))) == 0))
    {
        GtkWidget* image = gtk_image_menu_item_get_image(widget);
        if(!image)
            return;
        //gtk_widget_reparent(image, NULL);
        gtk_image_menu_item_set_image(widget, NULL);
        gtk_container_foreach(GTK_CONTAINER(widget), (GtkCallback)gtk_widget_destroy, NULL);
        gtk_container_add(GTK_CONTAINER(widget), image);
    }
}

gboolean get_widget_toggled(GtkWidget* widget)
{
    g_return_val_if_fail(GTK_IS_TOGGLE_BUTTON(widget) || GTK_IS_CHECK_MENU_ITEM(widget), FALSE);
    if(GTK_IS_TOGGLE_BUTTON(widget))
        return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
}

void set_widget_toggled(GtkWidget* widget,
                        gboolean state)
{
    g_return_if_fail(GTK_IS_TOGGLE_BUTTON(widget) || GTK_IS_CHECK_MENU_ITEM(widget));
    if(GTK_IS_TOGGLE_BUTTON(widget))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), state);
    return gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), state);
}

/* ---------------------------------------------------------------------------*
 * Definitions: static
 * -------------------------------------------------------------------------- */

static void show_message_dialog(GtkMessageType type,
                                const gchar* title,
                                const gchar* message_format,
                                va_list args)
{
    gchar* message = g_strdup_vprintf(message_format, args);
    GtkWidget* dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK,
                                               "%s", message);
    g_free(message);

    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_widget_show_all(dialog);
    set_window_position(dialog, &WINDOW_POSITION_CENTER);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static gboolean _grab_focus(GtkWidget* widget)
{
    gtk_widget_grab_focus(widget);
    return FALSE;
}
