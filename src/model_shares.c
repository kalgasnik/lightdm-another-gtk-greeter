/* model_shares.c
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

#include "model_shares.h"
#include "composite_widgets.h"

void set_composite_widget_props(CompositeWidget* widget,
                                GtkTreeModel*    model,
                                GtkTreeIter*     iter,
                                GSList*          model_bindings)
{
    g_return_if_fail(IS_COMPOSITE_WIDGET(widget));

    for(GSList* item = model_bindings; item != NULL; item = item->next)
    {
        const ModelPropertyBinding* bind = item->data;
        GValue value = G_VALUE_INIT;
        gtk_tree_model_get_value(GTK_TREE_MODEL(model), iter, bind->column, &value);
        if(bind->widget)
        {
            GObject* child = gtk_widget_get_template_child(GTK_WIDGET(widget), G_TYPE_FROM_INSTANCE(widget), bind->widget);
            if(child)
                g_object_set_property(child, bind->prop, &value);
        }
        else
            g_object_set_property(G_OBJECT(widget), bind->prop, &value);
        g_value_unset(&value);
    }
}

