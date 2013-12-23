/* model_listbox.c
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

#include "model_listbox.h"

typedef struct
{
    GtkListBox*        widget;
    GtkListBoxRow*     active;
    GtkTreeModel*      model;
    GSList*            model_bindings;

    NewWidgetFunc      new_widget;
    GCallback          on_changed;
    GCallback          on_activated;
} PrivateListBoxData;

static const gchar* LISTBOX_BINDING_PROP    = "private-model-data";
static const gchar* LISTBOX_ITEM_PATH_PROP  = "private-model-path";

/* Static functions */

static void on_listbox_row_selected         (GtkListBox*      list_box,
                                             GtkListBoxRow*   row,
                                             PrivateListBoxData* data);
static void on_listbox_row_changed          (GtkTreeModel*    model,
                                             GtkTreePath*     path,
                                             GtkTreeIter*     iter,
                                             PrivateListBoxData* data);
static void on_listbox_row_deleted          (GtkTreeModel*    model,
                                             GtkTreePath*     path,
                                             PrivateListBoxData* data);
static gboolean on_listbox_row_inserted     (GtkTreeModel*    model,
                                             GtkTreePath*     path,
                                             GtkTreeIter*     iter,
                                             PrivateListBoxData* data);

/* ---------------------------------------------------------------------------*
 * Definitions: public
 * -------------------------------------------------------------------------- */

void bind_listbox_model(GtkListBox*   widget,
                        NewWidgetFunc new_widget,
                        GtkListStore* model,
                        GSList*       model_bindings,
                        GCallback     on_changed,
                        GCallback     on_activated)
{
    g_return_if_fail(GTK_IS_LIST_BOX(widget));

    PrivateListBoxData* data = g_malloc(sizeof(PrivateListBoxData));
    data->widget         = widget;
    data->active         = NULL;
    data->model          = GTK_TREE_MODEL(model);
    data->model_bindings = model_bindings;
    data->new_widget     = new_widget;
    data->on_changed     = on_changed;
    data->on_activated   = on_activated;

    g_object_set_data(G_OBJECT(widget), LISTBOX_BINDING_PROP, data);
    g_signal_connect(widget, "row-selected", G_CALLBACK(on_listbox_row_selected), data);
    gtk_tree_model_foreach(GTK_TREE_MODEL(model), (GtkTreeModelForeachFunc)on_listbox_row_inserted, data);
    g_signal_connect(model, "row-changed", G_CALLBACK(on_listbox_row_changed), data);
    g_signal_connect(model, "row-deleted", G_CALLBACK(on_listbox_row_deleted), data);
    g_signal_connect(model, "row-inserted", G_CALLBACK(on_listbox_row_inserted), data);
}

GtkTreeModel* get_listbox_model(GtkListBox* widget)
{
    PrivateListBoxData* data = g_object_get_data(G_OBJECT(widget), LISTBOX_BINDING_PROP);
    return data ? data->model : NULL;
}

void set_listbox_active_path(GtkListBox*  widget,
                             GtkTreePath* path)
{
    GtkListBoxRow* row = gtk_list_box_get_row_at_index(widget, gtk_tree_path_get_indices(path)[0]);
    if(row)
        gtk_list_box_select_row(widget, row);
}

GtkTreePath* get_listbox_active_path(GtkListBox* widget)
{
    PrivateListBoxData* data = g_object_get_data(G_OBJECT(widget), LISTBOX_BINDING_PROP);
    GtkTreePath* path = g_object_get_data(G_OBJECT(data->active), LISTBOX_ITEM_PATH_PROP);
    return gtk_tree_path_copy(path);
}

/* ---------------------------------------------------------------------------*
 * Definitions: static
 * -------------------------------------------------------------------------- */

static void on_listbox_row_selected(GtkListBox*         list_box,
                                    GtkListBoxRow*      row,
                                    PrivateListBoxData* data)
{
    data->active = row;
    if(data->on_changed)
        ((GtkCallback)data->on_changed)(GTK_WIDGET(data->widget), NULL);
}

/* Model signals */

static void on_listbox_row_deleted(GtkTreeModel*       tree_model,
                                   GtkTreePath*        path,
                                   PrivateListBoxData* data)
{
    GtkListBoxRow* row = gtk_list_box_get_row_at_index(data->widget, gtk_tree_path_get_indices(path)[0]);
    if(row)
         gtk_widget_destroy(GTK_WIDGET(row));
}

static void on_listbox_row_changed(GtkTreeModel*       model,
                                   GtkTreePath*        path,
                                   GtkTreeIter*        iter,
                                   PrivateListBoxData* data)
{
    GtkListBoxRow* row = gtk_list_box_get_row_at_index(data->widget, gtk_tree_path_get_indices(path)[0]);
    if(row)
    {
        GtkWidget* content = gtk_bin_get_child(GTK_BIN(row));
        for(GSList* item = data->model_bindings; item != NULL; item = item->next)
        {
            const ModelPropertyBinding* bind = item->data;
            GValue value = G_VALUE_INIT;
            gtk_tree_model_get_value(GTK_TREE_MODEL(model), iter, bind->column, &value);
            if(bind->widget)
            {
                GObject* child = gtk_widget_get_template_child(content, G_TYPE_FROM_INSTANCE(content), bind->widget);
                if(child)
                    g_object_set_property(child, bind->prop, &value);
            }
            else
                g_object_set_property(G_OBJECT(content), bind->prop, &value);
            g_value_unset(&value);
        }
    }
}

static gboolean on_listbox_row_inserted(GtkTreeModel*       model,
                                        GtkTreePath*        path,
                                        GtkTreeIter*        iter,
                                        PrivateListBoxData* data)
{
    const gint* indices = gtk_tree_path_get_indices(path);
    GtkWidget* row = gtk_list_box_row_new();
    GtkWidget* content = data->new_widget();
    gtk_container_add(GTK_CONTAINER(row), content);
    g_object_set_data(G_OBJECT(row), LISTBOX_ITEM_PATH_PROP, gtk_tree_model_get_path(data->model, iter));
    gtk_list_box_insert(data->widget, row, indices[0]);
    gtk_widget_show_all(row);
    return FALSE;
}
