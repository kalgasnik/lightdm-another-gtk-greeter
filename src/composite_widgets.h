/* composite_widgets.h
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

#ifndef COMPOSITE_WIDGETS_H_INCLUDED
#define COMPOSITE_WIDGETS_H_INCLUDED

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define COMPOSITE_WIDGET_TYPE              (composite_widget_get_type())
#define COMPOSITE_WIDGET(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), COMPOSITE_WIDGET_TYPE, CompositeWidget))
#define COMPOSITE_WIDGET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  COMPOSITE_WIDGET_TYPE, CompositeWidgetClass))
#define IS_COMPOSITE_WIDGET(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), COMPOSITE_WIDGET_TYPE))
#define IS_COMPOSITE_WIDGET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  COMPOSITE_WIDGET_TYPE))
#define COMPOSITE_WIDGET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  COMPOSITE_WIDGET_TYPE, CompositeWidgetClass))

typedef struct _CompositeWidget           CompositeWidget;
typedef struct _CompositeWidgetClass      CompositeWidgetClass;
typedef struct _CompositeWidgetPrivate    CompositeWidgetPrivate;

struct _CompositeWidget
{
    GtkBox box;
    struct _CompositeWidgetPrivate* priv;
};

struct _CompositeWidgetClass
{
    GtkBoxClass parent_class;
};

GtkWidget* composite_widget_new            (void);

void composite_widget_set_text             (CompositeWidget* widget,
                                            const gchar*     text);
const gchar* composite_widget_get_text     (CompositeWidget* widget);
void composite_widget_set_image_from_pixbuf(CompositeWidget* widget,
                                            GdkPixbuf*       pixbuf);
GdkPixbuf* composite_widget_get_pixbuf     (CompositeWidget*   widget);

/* Session widget */

#define SESSION_WIDGET_TYPE             (session_widget_get_type ())
#define SESSION_WIDGET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SESSION_WIDGET_TYPE, SessionWidget))
#define SESSION_WIDGET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SESSION_WIDGET_TYPE, SessionWidgetClass))
#define IS_SESSION_WIDGET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SESSION_WIDGET_TYPE))
#define IS_SESSION_WIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SESSION_WIDGET_TYPE))
#define SESSION_WIDGET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SESSION_WIDGET_TYPE, SessionWidgetClass))

typedef struct _SessionWidget            SessionWidget;
typedef struct _SessionWidgetClass       SessionWidgetClass;

struct _SessionWidget
{
    CompositeWidget composite;
};

struct _SessionWidgetClass
{
    CompositeWidgetClass parent_class;
};

GtkWidget* session_widget_new           (void);

/* Language widget */

#define LANGUAGE_WIDGET_TYPE            (language_widget_get_type ())
#define LANGUAGE_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LANGUAGE_WIDGET_TYPE, LanguageWidget))
#define LANGUAGE_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LANGUAGE_WIDGET_TYPE, LanguageWidgetClass))
#define IS_LANGUAGE_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LANGUAGE_WIDGET_TYPE))
#define IS_LANGUAGE_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LANGUAGE_WIDGET_TYPE))
#define LANGUAGE_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LANGUAGE_WIDGET_TYPE, LanguageWidgetClass))

typedef struct _LanguageWidget           LanguageWidget;
typedef struct _LanguageWidgetClass      LanguageWidgetClass;

struct _LanguageWidget
{
    CompositeWidget composite;
};

struct _LanguageWidgetClass
{
    CompositeWidgetClass parent_class;
};

GtkWidget* language_widget_new          (void);

G_END_DECLS


#endif
