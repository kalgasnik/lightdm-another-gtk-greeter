/* model_menu.h
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

#ifndef _MENU_MODEL_H_INCLUDED_
#define _MENU_MODEL_H_INCLUDED_

#include <gtk/gtk.h>
#include "shares.h"

#define IS_MENU_WIDGET(widget)          (GTK_IS_MENU_BUTTON(widget) || GTK_IS_MENU_ITEM(widget))
void bind_menu_widget_model             (GtkWidget*    widget,
                                         NewWidgetFunc new_widget,
                                         GtkListStore* model,
                                         /* List of ModelPropertyBinding* */
                                         GSList*       model_bindings,
                                         GtkWidget*    label,
                                         gint          label_column,
                                         GCallback     on_changed);
GtkTreeModel* get_menu_widget_model     (GtkWidget* widget);
void set_menu_widget_active_path        (GtkWidget*   widget,
                                         GtkTreePath* path);
GtkTreePath* get_menu_widget_active_path(GtkWidget* widget);

#endif
