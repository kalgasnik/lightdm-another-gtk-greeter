/* composite_widget.h
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

#include <gtk/gtk.h>
#include "shares.h"
#include "configuration.h"
#include "composite_widgets.h"

struct _CompositeWidgetPrivate
{
    GtkWidget* label;
    GtkWidget* image;
    GtkWidget* image_box;
};

enum
{
    COMPOSITE_PROP_0,
    COMPOSITE_PROP_TEXT,
    COMPOSITE_PROP_TEXT_WEIGHT,
    COMPOSITE_PROP_PIXBUF
};

G_DEFINE_TYPE_WITH_PRIVATE(CompositeWidget, composite_widget, GTK_TYPE_BOX);

static void composite_widget_set_property(GObject*      object,
                                          guint         prop_id,
                                          const GValue* value,
                                          GParamSpec*   pspec)
{
    CompositeWidget* widget = COMPOSITE_WIDGET(object);
    switch(prop_id)
    {
        case COMPOSITE_PROP_TEXT:
            composite_widget_set_text(widget, g_value_get_string(value));
            break;
        case COMPOSITE_PROP_TEXT_WEIGHT:
            composite_widget_set_text_weight(widget, g_value_get_int(value));
            break;
        case COMPOSITE_PROP_PIXBUF:
            composite_widget_set_image_from_pixbuf(widget, g_value_get_object(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void composite_widget_get_property(GObject*    object,
			                              guint       prop_id,
			                              GValue*     value,
			                              GParamSpec* pspec)
{
    CompositeWidget* widget = COMPOSITE_WIDGET(object);
    switch(prop_id)
    {
        case COMPOSITE_PROP_TEXT:
            g_value_set_string(value, composite_widget_get_text(widget));
            break;
        case COMPOSITE_PROP_PIXBUF:
            g_value_set_object(value, composite_widget_get_pixbuf(widget));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void composite_widget_class_init_successor(GtkWidgetClass*  widget_class,
                                                  GBytes*          template_data,
                                                  GSList*          bindings)
{
    if(template_data)
    {
        gtk_widget_class_set_template(widget_class, template_data);

        gtk_widget_class_bind_template_child_private(widget_class, CompositeWidget, label);
        gtk_widget_class_bind_template_child_private(widget_class, CompositeWidget, image);
        gtk_widget_class_bind_template_child_private(widget_class, CompositeWidget, image_box);

        for(GSList* item = bindings; item != NULL; item = item->next)
        {
            ModelPropertyBinding* bind = item->data;
            if(bind->widget)
                gtk_widget_class_bind_template_child_full(widget_class, bind->widget, FALSE, 0);
        }
    }
}

static void session_widget_init_successor(CompositeWidget* widget,
                                          GBytes*          template_data,
                                          GSList*          bindings)
{
    if(template_data)
    {
        gtk_widget_init_template(GTK_WIDGET(widget));
    }
    else
    {
        widget->priv->label = gtk_label_new(NULL);
        widget->priv->image = gtk_image_new();
        widget->priv->image_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_pack_start(GTK_BOX(widget->priv->image_box), widget->priv->image, FALSE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(widget), widget->priv->label, TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(widget), widget->priv->image_box, FALSE, TRUE, 0);
        gtk_widget_set_halign(widget->priv->label, GTK_ALIGN_START);
        gtk_widget_show(widget->priv->label);
        gtk_widget_show(widget->priv->image);
    }
}

static void composite_widget_class_init(CompositeWidgetClass* klass)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->set_property = composite_widget_set_property;
    gobject_class->get_property = composite_widget_get_property;

    g_object_class_install_property(gobject_class,
                                    COMPOSITE_PROP_TEXT,
                                    g_param_spec_string("text", "Text",
                                                        "Text of the widget",
                                                        NULL,
                                                        G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class,
                                    COMPOSITE_PROP_TEXT_WEIGHT,
                                    g_param_spec_int("text-weight", "Text weight",
                                                     "Text weight of the widget",
                                                      0, G_MAXINT, 0,
                                                      G_PARAM_WRITABLE));
    g_object_class_install_property(gobject_class,
                                    COMPOSITE_PROP_PIXBUF,
                                    g_param_spec_object("pixbuf", "Pixbuf",
                                                        "Pixbuf for widget image",
                                                        GDK_TYPE_PIXBUF,
                                                        G_PARAM_READWRITE));

    g_type_class_add_private(gobject_class, sizeof(CompositeWidgetPrivate));
}

static void composite_widget_init(CompositeWidget* widget)
{
    widget->priv = G_TYPE_INSTANCE_GET_PRIVATE(widget, COMPOSITE_WIDGET_TYPE, CompositeWidgetPrivate);
}

GtkWidget* composite_widget_new(void)
{
    return g_object_new(COMPOSITE_WIDGET_TYPE, NULL);
}

void composite_widget_set_text(CompositeWidget* widget,
                               const gchar* text)
{
    g_return_if_fail(IS_COMPOSITE_WIDGET(widget));
    set_widget_text(widget->priv->label, text);
    g_object_notify(G_OBJECT(widget), "text");
}

const gchar* composite_widget_get_text(CompositeWidget* widget)
{
    g_return_val_if_fail(IS_COMPOSITE_WIDGET(widget), NULL);
    return gtk_label_get_label(GTK_LABEL(widget->priv->label));
}

void composite_widget_set_text_weight(CompositeWidget* widget,
                                      PangoWeight weight)
{
    g_return_if_fail(IS_COMPOSITE_WIDGET(widget));
    g_return_if_fail(GTK_IS_LABEL(widget->priv->label));

    GtkLabel* label = GTK_LABEL(widget->priv->label);
    PangoAttribute* attribute = pango_attr_weight_new(weight);
    PangoAttrList* attributes_to_unref = NULL;
    PangoAttrList* attributes = gtk_label_get_attributes(label);
    if(!attributes)
        attributes = attributes_to_unref = pango_attr_list_new();
    pango_attr_list_change(attributes, attribute);
    gtk_label_set_attributes(label, attributes);
    pango_attr_list_unref(attributes_to_unref);

    g_object_notify(G_OBJECT(widget), "text-weight");
}

void composite_widget_set_image_from_pixbuf(CompositeWidget* widget,
                                          GdkPixbuf*     pixbuf)
{
    g_return_if_fail(IS_COMPOSITE_WIDGET(widget));
    g_return_if_fail(GTK_IS_IMAGE(widget->priv->image));

    if(widget->priv->image)
    {
        gtk_image_set_from_pixbuf(GTK_IMAGE(widget->priv->image), pixbuf);
        if(widget->priv->image_box)
            gtk_widget_set_visible(widget->priv->image_box, pixbuf != NULL);
    }
    g_object_notify(G_OBJECT(widget), "pixbuf");
}

GdkPixbuf* composite_widget_get_pixbuf(CompositeWidget* widget)
{
    g_return_val_if_fail(IS_COMPOSITE_WIDGET(widget), NULL);
    g_return_val_if_fail(GTK_IS_IMAGE(widget->priv->image), NULL);
    return gtk_image_get_pixbuf(GTK_IMAGE(widget->priv->image));
}

/* SessionWidget */

G_DEFINE_TYPE(SessionWidget, session_widget, COMPOSITE_WIDGET_TYPE)

static void session_widget_class_init(SessionWidgetClass* klass)
{
    composite_widget_class_init_successor(GTK_WIDGET_CLASS(klass),
                                          config.appearance.templates.session.data,
                                          config.appearance.templates.session.bindings);
}

static void session_widget_init(SessionWidget* widget)
{
    session_widget_init_successor(COMPOSITE_WIDGET(widget),
                                  config.appearance.templates.session.data,
                                  config.appearance.templates.session.bindings);
}

GtkWidget* session_widget_new(void)
{
    return g_object_new(SESSION_WIDGET_TYPE, NULL);
}

/* LanguageWidget */

G_DEFINE_TYPE(LanguageWidget, language_widget, COMPOSITE_WIDGET_TYPE)

static void language_widget_class_init(LanguageWidgetClass* klass)
{
    composite_widget_class_init_successor(GTK_WIDGET_CLASS(klass),
                                          config.appearance.templates.language.data,
                                          config.appearance.templates.language.bindings);
}

static void language_widget_init(LanguageWidget* widget)
{
    session_widget_init_successor(COMPOSITE_WIDGET(widget),
                                  config.appearance.templates.language.data,
                                  config.appearance.templates.language.bindings);
}

GtkWidget* language_widget_new(void)
{
    return g_object_new(LANGUAGE_WIDGET_TYPE, NULL);
}
