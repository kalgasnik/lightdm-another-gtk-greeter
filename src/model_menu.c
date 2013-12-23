/* model_menu.c
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

#include "model_menu.h"

typedef struct
{
    GtkWidget*         owner;
    GtkWidget*         label;
    gint               label_column;
    GtkWidget*         active;
    GtkTreeModel*      model;
    GSList*            model_bindings;
    /* HashTable<GtkTreePath, GtkWidget>
       gtk_widget_destroy() used as "value_destroy_func */
    GHashTable*        model_mapping;
    GtkMenuShell*      menu;
    GSList*            menu_group;

    NewWidgetFunc      new_widget;
    GCallback          on_changed;
} PrivateMenuData;

static const gchar* MENU_WIDGET_BINDING_PROP = "private-model-data";
static const gchar* MENU_ITEM_PATH_PROP      = "private-model-path";

/* Static functions */

/* For g_slist_find_custom */
static void on_menu_widget_row_changed      (GtkTreeModel*    model,
                                             GtkTreePath*     path,
                                             GtkTreeIter*     iter,
                                             PrivateMenuData* data);
static void on_menu_widget_row_deleted      (GtkTreeModel*    model,
                                             GtkTreePath*     path,
                                             PrivateMenuData* data);
static gboolean on_menu_widget_row_inserted (GtkTreeModel*    model,
                                             GtkTreePath*     path,
                                             GtkTreeIter*     iter,
                                             PrivateMenuData* data);
static void on_menu_widget_item_toggled     (GtkWidget*       widget,
                                             PrivateMenuData* data);

static gboolean gtk_tree_path_equal(GtkTreePath* a, GtkTreePath* b)
{
    return gtk_tree_path_compare(a, b) == 0;
}

static guint gtk_tree_path_hash(GtkTreePath* path)
{
    return gtk_tree_path_get_indices(path)[0];
}

/* ---------------------------------------------------------------------------*
 * Definitions: public
 * -------------------------------------------------------------------------- */

GtkTreeModel* get_menu_widget_model(GtkWidget* widget)
{
    PrivateMenuData* data = g_object_get_data(G_OBJECT(widget), MENU_WIDGET_BINDING_PROP);
    return data ? data->model : NULL;
}

void bind_menu_widget_model(GtkWidget*    widget,
                            NewWidgetFunc new_widget,
                            GtkListStore* model,
                            /* List of ModelPropertyBinding */
                            GSList*       model_bindings,
                            GtkWidget*    label,
                            gint          label_column,
                            GCallback     on_changed)
{
    g_return_if_fail(IS_MENU_WIDGET(widget));

    PrivateMenuData* data= g_malloc(sizeof(PrivateMenuData));
    data->owner          = GTK_WIDGET(widget);
    data->label          = label;
    data->label_column   = label_column;
    data->active         = NULL;
    data->model          = GTK_TREE_MODEL(model);
    data->model_bindings = model_bindings;
    data->model_mapping  = g_hash_table_new_full((GHashFunc)gtk_tree_path_hash, (GEqualFunc)gtk_tree_path_equal,
                                                 (GDestroyNotify)gtk_tree_path_free, (GDestroyNotify)gtk_widget_destroy);
    data->menu_group     = NULL;
    data->new_widget     = new_widget;
    data->on_changed     = on_changed;

    if(GTK_IS_MENU_ITEM(widget))
        data->menu = GTK_MENU_SHELL(gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget)));
    else if(GTK_IS_MENU_BUTTON(widget))
        data->menu = GTK_MENU_SHELL(gtk_menu_button_get_popup(GTK_MENU_BUTTON(widget)));
    else
        data->menu = NULL;

    g_object_set_data(G_OBJECT(widget), MENU_WIDGET_BINDING_PROP, data);
    gtk_tree_model_foreach(GTK_TREE_MODEL(model), (GtkTreeModelForeachFunc)on_menu_widget_row_inserted, data);
    g_signal_connect(model, "row-changed", G_CALLBACK(on_menu_widget_row_changed), data);
    g_signal_connect(model, "row-deleted", G_CALLBACK(on_menu_widget_row_deleted), data);
    g_signal_connect(model, "row-inserted", G_CALLBACK(on_menu_widget_row_inserted), data);
}

void set_menu_widget_active_path(GtkWidget* widget,
                                 GtkTreePath* path)
{
    PrivateMenuData* data = g_object_get_data(G_OBJECT(widget), MENU_WIDGET_BINDING_PROP);
    GtkCheckMenuItem* item = g_hash_table_lookup(data->model_mapping, path);
    if(item)
    {
        if(gtk_check_menu_item_get_active(item))
            gtk_check_menu_item_toggled(item);
        else
            gtk_check_menu_item_set_active(item, TRUE);
    }
}

GtkTreePath* get_menu_widget_active_path(GtkWidget* widget)
{
    PrivateMenuData* data = g_object_get_data(G_OBJECT(widget), MENU_WIDGET_BINDING_PROP);
    GtkTreePath* path = g_object_get_data(G_OBJECT(data->active), MENU_ITEM_PATH_PROP);
    return gtk_tree_path_copy(path);
}

/* ---------------------------------------------------------------------------*
 * Definitions: static
 * -------------------------------------------------------------------------- */

/* Model events */

static void on_menu_widget_row_deleted(GtkTreeModel*    tree_model,
                                       GtkTreePath*     path,
                                       PrivateMenuData* data)
{
    /* TODO: update menu_group */
    g_hash_table_remove(data->model_mapping, path);
}

static void on_menu_widget_row_changed(GtkTreeModel*    model,
                                       GtkTreePath*     path,
                                       GtkTreeIter*     iter,
                                       PrivateMenuData* data)
{
    GtkWidget* item = g_hash_table_lookup(data->model_mapping, path);
    if(item)
    {
        GtkWidget* content = gtk_bin_get_child(GTK_BIN(item));
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
        if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item)))
        {
            gchar* label = NULL;
            gtk_tree_model_get(model, iter, data->label_column, &label, -1);
            set_widget_text(data->label, label);
            g_free(label);
        }
    }
}

static gboolean on_menu_widget_row_inserted(GtkTreeModel*    model,
                                            GtkTreePath*     path,
                                            GtkTreeIter*     iter,
                                            PrivateMenuData* data)
{
    const gint* indices = gtk_tree_path_get_indices(path);
    GtkWidget* item = gtk_radio_menu_item_new(data->menu_group);
    GtkWidget* content =  data->new_widget();
    GtkTreePath* path_copy = gtk_tree_path_copy(path);

    data->menu_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
    g_hash_table_insert(data->model_mapping, path_copy, item);
    g_object_set_data(G_OBJECT(item), MENU_ITEM_PATH_PROP, path_copy);
    g_signal_connect(item, "toggled", G_CALLBACK(on_menu_widget_item_toggled), data);

    gtk_container_add(GTK_CONTAINER(item), content);
    gtk_widget_show(item);
    gtk_widget_show(content);
    gtk_menu_shell_insert(data->menu, item, indices[0]);
    return FALSE;
}

static void on_menu_widget_item_toggled(GtkWidget*       widget,
                                        PrivateMenuData* data)
{
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
    {
        GtkTreeIter iter;
        if(gtk_tree_model_get_iter(data->model, &iter,
                                   g_object_get_data(G_OBJECT(widget), MENU_ITEM_PATH_PROP)))
        {
            gchar* label = NULL;
            gtk_tree_model_get(data->model, &iter, data->label_column, &label, -1);
            set_widget_text(data->label, label);
            g_free(label);
        }

        data->active = widget;
        if(data->on_changed)
            ((GtkCallback)data->on_changed)(data->owner, NULL);
    }
}
