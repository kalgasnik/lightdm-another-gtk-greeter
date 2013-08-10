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

#include <glib/gi18n.h>
#include <string.h>

#include "shares.h"
#include "configuration.h"

/* Variables */

GreeterData greeter = {NULL, };
const gchar* const USER_GUEST           = "*guest";
const gchar* const USER_OTHER           = "*other";
const gchar* const APP_NAME             = "lightdm-another-gtk-greeter";
const gchar* const DEFAULT_USER_ICON    = "avatar-default";
const gchar* const ACTION_TEXT_LOGIN    = N_("Login");
const gchar* const ACTION_TEXT_UNLOCK   = N_("Unlock");

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
gchar* GETTEXT_PACKAGE = "lightdm-another-gtk-greeter";
gchar* LOCALE_DIR = "/usr/local/share.locale";
gchar* GREETER_DATA_DIR = "../../data";
gchar* CONFIG_FILE = "../../data/lightdm-another-gtk-greeter.dev.conf";
gchar* PACKAGE_VERSION = "<DEBUG>";
#endif

/* Static functions */

static void show_message_dialog           (GtkMessageType type,
                                           const gchar* title,
                                           const gchar* message_format,
                                           va_list args);

static void on_transparent_screen_changed (GtkWidget *widget,
                                           GdkScreen *old_screen,
                                           gpointer userdata);


/* ---------------------------------------------------------------------------*
 * Definitions: public
 * -------------------------------------------------------------------------- */

void update_windows_layout(void)
{
    GdkRectangle geometry;
    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(greeter.ui.login_window));
    gdk_screen_get_monitor_geometry(screen, gdk_screen_get_primary_monitor(screen), &geometry);

    set_window_position(greeter.ui.login_window, &config.appearance.position);

    if(config.panel.enabled)
    {
        gtk_widget_set_size_request(greeter.ui.panel_window, geometry.width, -1);
        set_window_position(greeter.ui.panel_window, &greeter.state.panel.position);
    }

    if(greeter.ui.onboard && gtk_widget_get_visible(GTK_WIDGET(greeter.ui.onboard)))
    {
        gboolean panel_at_top = config.panel.position == PANEL_POS_TOP;
        gboolean at_top;
        switch(config.a11y.osk.onboard_position)
        {
            case ONBOARD_POS_PANEL: at_top = panel_at_top; break;
            case ONBOARD_POS_PANEL_OPPOSITE: at_top =  !panel_at_top; break;
            case ONBOARD_POS_BOTTOM: at_top = FALSE; break;
            case ONBOARD_POS_TOP: ;
            default: at_top = TRUE;
        };

        gint onboard_height = config.a11y.osk.onboard_height_is_percent ? geometry.height*config.a11y.osk.onboard_height/100 : config.a11y.osk.onboard_height;
        gint onboard_y = at_top ? 0 : geometry.height - onboard_height;

        if(at_top ==  panel_at_top
           && greeter.ui.panel_window
           && gtk_widget_get_visible(greeter.ui.panel_window))
        {
            gint panel_height;
            gtk_widget_get_preferred_height(greeter.ui.panel_window, NULL, &panel_height);
            onboard_y += at_top ? + panel_height : -panel_height;
        }

        gtk_window_resize(GTK_WINDOW(greeter.ui.onboard), geometry.width - geometry.x, onboard_height);
        gtk_window_move(GTK_WINDOW(greeter.ui.onboard), geometry.x, geometry.y + onboard_y);

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
    else g_return_if_reached();
}

GtkTreeModel* get_widget_model(GtkWidget* widget)
{
    if(GTK_IS_COMBO_BOX(widget))
        return gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    if(GTK_IS_TREE_VIEW(widget))
        return gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
    if(GTK_IS_ICON_VIEW(widget))
        return gtk_icon_view_get_model(GTK_ICON_VIEW(widget));
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
    if(GTK_IS_ICON_VIEW(widget))
    {
        gboolean ok = FALSE;
        GList* selection = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(widget));
        if(g_list_first(selection) != NULL)
        {
            GtkTreePath* path = (GtkTreePath*)g_list_first(selection)->data;
            ok = gtk_tree_model_get_iter(gtk_icon_view_get_model(GTK_ICON_VIEW(widget)), iter, path);
        }
        g_list_free_full(selection, (GDestroyNotify)gtk_tree_path_free);
        return ok;
    }
    g_return_val_if_reached(NULL);
}

void set_widget_active_iter(GtkWidget* widget,
                            GtkTreeIter* iter)
{
    if(GTK_IS_COMBO_BOX(widget))
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget), iter);
    else if(GTK_IS_TREE_VIEW(widget))
    {
        GtkTreePath* path = gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)), iter);
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), path, NULL, FALSE);
        gtk_tree_path_free(path);
    }
    else if(GTK_IS_ICON_VIEW(widget))
    {
        GtkTreePath* path = gtk_tree_model_get_path(gtk_icon_view_get_model(GTK_ICON_VIEW(widget)), iter);
        gtk_icon_view_set_cursor(GTK_ICON_VIEW(widget), path, NULL, FALSE);
        gtk_tree_path_free(path);
    }
    else
        g_return_val_if_reached(NULL);
}

void set_widget_active_first(GtkWidget* widget)
{
    if(GTK_IS_COMBO_BOX(widget))
        gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
    else if(GTK_IS_TREE_VIEW(widget))
    {
        GtkTreePath* path = gtk_tree_path_new_first();
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), path, NULL, FALSE);
        gtk_tree_path_free(path);
    }
    else if(GTK_IS_ICON_VIEW(widget))
    {
        GtkTreePath* path = gtk_tree_path_new_first();
        gtk_icon_view_set_cursor(GTK_ICON_VIEW(widget), path, NULL, FALSE);
        gtk_tree_path_free(path);
    }
    else
        g_return_val_if_reached(NULL);
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
        gtk_image_menu_item_set_image(widget, NULL);
        gtk_container_foreach(GTK_CONTAINER(widget), (GtkCallback)gtk_widget_destroy, NULL);
        gtk_container_add(GTK_CONTAINER(widget), image);
    }
}

gboolean get_widget_toggled(GtkWidget* widget)
{
    if(GTK_IS_TOGGLE_BUTTON(widget))
        return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    else if(GTK_IS_CHECK_MENU_ITEM(widget))
        return gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    else
        g_return_val_if_reached(FALSE);
}

void set_widget_toggled(GtkWidget* widget,
                        gboolean state,
                        GCallback suppress_callback)
{
    if(suppress_callback)
        g_signal_handlers_block_matched(widget, G_SIGNAL_MATCH_FUNC, 0, 0, 0, suppress_callback, NULL);
    if(GTK_IS_TOGGLE_BUTTON(widget))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), state);
    else if(GTK_IS_CHECK_MENU_ITEM(widget))
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), state);
    if(suppress_callback)
        g_signal_handlers_unblock_matched(widget, G_SIGNAL_MATCH_FUNC, 0, 0, 0, suppress_callback, NULL);
}

void setup_window(GtkWindow* window)
{
    g_return_if_fail(GTK_IS_WINDOW(window));
    g_signal_connect(G_OBJECT(window), "screen-changed", G_CALLBACK(on_transparent_screen_changed), NULL);
    on_transparent_screen_changed(GTK_WIDGET(window), NULL, NULL);
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
    GtkWidget* dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, "%s", message);
    g_free(message);

    const gchar* window_name = NULL;
    switch(type)
    {
        case GTK_MESSAGE_INFO: window_name = "dialog_window_info"; break;
        case GTK_MESSAGE_WARNING: window_name = "dialog_window_warning"; break;
        case GTK_MESSAGE_QUESTION: window_name = "dialog_window_question"; break;
        case GTK_MESSAGE_ERROR: window_name = "dialog_window_error"; break;
        default:
            window_name = "dialog_window";
    }
    gtk_widget_set_name(dialog, window_name);

    setup_window(GTK_WINDOW(dialog));
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_widget_show_all(dialog);
    set_window_position(dialog, &WINDOW_POSITION_CENTER);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_transparent_screen_changed(GtkWidget *widget,
                                          GdkScreen *old_screen,
                                          gpointer userdata)
{
    GdkScreen* screen = gtk_widget_get_screen(widget);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
    if(visual)
        gtk_widget_set_visual(widget, visual);
}
