/* model_box.c
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

#include "model_box.h"
#include "composite_widgets.h"

#if GTK_CHECK_VERSION(3, 10, 0)

typedef struct
{
    GtkBox*            widget;
    /* Selected widget (content) */
    GtkWidget*         active;
    GtkTreeModel*      model;
    GSList*            model_bindings;
    /* HashTable<GtkTreePath, GtkWidget>
       gtk_widget_destroy() used as "value_destroy_func */
    GHashTable*        model_mapping;
    NewWidgetFunc      new_widget;
    GCallback          on_changed;
    GCallback          on_activated;
} PrivateBoxData;

typedef struct
{
    GtkTreePath* path;
    GHashTable*  children;
} PrivateBoxItemData;

static const gchar* BOX_BINDING_PROP     = "private-model-data";
static const gchar* BOX_ITEM_DATA_PROP   = "private-item-data";

/* Static functions */

static void update_box_selection            (PrivateBoxData* data,
                                             GtkWidget* new_selection,
                                             gboolean emit_changed);

static void on_child_focus                  (GtkWidget* widget,
                                             GtkWidget* child,
                                             PrivateBoxData* data);

/* Model */
static void on_box_row_changed              (GtkTreeModel*    model,
                                             GtkTreePath*     path,
                                             GtkTreeIter*     iter,
                                             PrivateBoxData*  data);
static void on_box_row_deleted              (GtkTreeModel*    model,
                                             GtkTreePath*     path,
                                             PrivateBoxData*  data);
static gboolean on_box_row_inserted         (GtkTreeModel*    model,
                                             GtkTreePath*     path,
                                             GtkTreeIter*     iter,
                                             PrivateBoxData*  data);

static gboolean gtk_tree_path_equal         (GtkTreePath* a,
                                             GtkTreePath* b)
{
    return gtk_tree_path_compare(a, b) == 0;
}

static guint gtk_tree_path_hash             (GtkTreePath* path)
{
    return gtk_tree_path_get_indices(path)[0];
}

/* ---------------------------------------------------------------------------*
 * Definitions: public
 * -------------------------------------------------------------------------- */

void bind_box_model(GtkBox*       widget,
                    NewWidgetFunc new_widget,
                    GtkListStore* model,
                    GSList*       model_bindings,
                    GCallback     on_changed,
                    GCallback     on_activated)
{
    g_return_if_fail(GTK_IS_BOX(widget));

    PrivateBoxData* data = g_malloc(sizeof(PrivateBoxData));
    data->widget         = widget;
    data->active         = NULL;
    data->model          = GTK_TREE_MODEL(model);
    data->model_bindings = model_bindings;
    data->model_mapping  = g_hash_table_new_full((GHashFunc)gtk_tree_path_hash, (GEqualFunc)gtk_tree_path_equal,
                                                 (GDestroyNotify)gtk_tree_path_free, (GDestroyNotify)gtk_widget_destroy);
    data->new_widget     = new_widget;
    data->on_changed     = on_changed;
    data->on_activated   = on_activated;

    g_object_set_data(G_OBJECT(widget), BOX_BINDING_PROP, data);
    gtk_tree_model_foreach(GTK_TREE_MODEL(model), (GtkTreeModelForeachFunc)on_box_row_inserted, data);
    g_signal_connect(model, "row-changed", G_CALLBACK(on_box_row_changed), data);
    g_signal_connect(model, "row-deleted", G_CALLBACK(on_box_row_deleted), data);
    g_signal_connect(model, "row-inserted", G_CALLBACK(on_box_row_inserted), data);
}

GtkTreeModel* get_box_model(GtkBox* widget)
{
    g_return_val_if_fail(GTK_IS_BOX(widget), NULL);
    PrivateBoxData* data = g_object_get_data(G_OBJECT(widget), BOX_BINDING_PROP);
    return data ? data->model : NULL;
}

void set_box_active_path(GtkBox*      widget,
                         GtkTreePath* path)
{
    g_return_if_fail(GTK_IS_BOX(widget));
    PrivateBoxData* data = g_object_get_data(G_OBJECT(widget), BOX_BINDING_PROP);
    GtkWidget* content = g_hash_table_lookup(data->model_mapping, path);

    update_box_selection(data, content, FALSE);
    if(content)
        gtk_widget_grab_focus(content);
}

GtkTreePath* get_box_active_path(GtkBox* widget)
{
    PrivateBoxData* data = g_object_get_data(G_OBJECT(widget), BOX_BINDING_PROP);
    PrivateBoxItemData* item_data = g_object_get_data(G_OBJECT(data->active), BOX_ITEM_DATA_PROP);
    return item_data ? gtk_tree_path_copy(item_data->path) : NULL;
}

/* ---------------------------------------------------------------------------*
 * Definitions: static
 * -------------------------------------------------------------------------- */

static void update_box_selection(PrivateBoxData* data,
                                 GtkWidget* new_selection,
                                 gboolean emit_changed)
{
    if(new_selection == data->active)
        return;
    if(data->active)
        composite_widget_set_selected_style(COMPOSITE_WIDGET(data->active), FALSE);
    composite_widget_set_selected_style(COMPOSITE_WIDGET(new_selection), TRUE);
    data->active = new_selection;
    if(emit_changed && data->on_changed)
        ((GtkCallback)data->on_changed)(GTK_WIDGET(data->widget), NULL);
}

static void on_child_focus(GtkWidget* content,
                           GtkWidget* child,
                           PrivateBoxData* data)
{
    update_box_selection(data, content, TRUE);
}

/* Model signals */

static void on_box_row_deleted(GtkTreeModel*   tree_model,
                               GtkTreePath*    path,
                               PrivateBoxData* data)
{
    g_hash_table_remove(data->model_mapping, path);
}

static void on_box_row_changed(GtkTreeModel*   model,
                               GtkTreePath*    path,
                               GtkTreeIter*    iter,
                               PrivateBoxData* data)
{

    //set_composite_widget_props(g_hash_table_lookup(data->model_mapping, path), model, iter, data->model_bindings);
    GtkWidget* content = g_hash_table_lookup(data->model_mapping, path);
    if(content)
    {
        for(GSList* item = data->model_bindings; item != NULL; item = item->next)
        {
            const ModelPropertyBinding* bind = item->data;
            GValue value = G_VALUE_INIT;
            gtk_tree_model_get_value(GTK_TREE_MODEL(model), iter, bind->column, &value);
            #if GTK_CHECK_VERSION(3, 10, 0)
            if(bind->widget)
            {
                GObject* child = gtk_widget_get_template_child(content, G_TYPE_FROM_INSTANCE(content), bind->widget);
                if(child)
                    g_object_set_property(child, bind->prop, &value);
            }
            else
            #else
            if(!bind->widget)
            #endif
            {
                g_object_set_property(G_OBJECT(content), bind->prop, &value);
            }
            g_value_unset(&value);
        }
    }
}

static gboolean on_box_row_inserted(GtkTreeModel*       model,
                                        GtkTreePath*    path,
                                        GtkTreeIter*    iter,
                                        PrivateBoxData* data)
{
    const gint* indices = gtk_tree_path_get_indices(path);
    GtkWidget* content = data->new_widget();
    PrivateBoxItemData* content_data = g_malloc(sizeof(PrivateBoxItemData));
    content_data->path = gtk_tree_path_copy(path);
    content_data->children = g_hash_table_new(g_direct_hash, g_direct_equal);

    g_signal_connect(content, "set-focus-child", G_CALLBACK(on_child_focus), data);
    g_object_set_data_full(G_OBJECT(content), BOX_ITEM_DATA_PROP, content_data, g_free);
    g_hash_table_insert(data->model_mapping, content_data->path, content);
    gtk_widget_show(content);
    gtk_box_pack_end(data->widget, content, TRUE, TRUE, 0);
    gtk_box_reorder_child(data->widget, content, indices[0]);
    return FALSE;
}

#endif
