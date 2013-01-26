/* indicator_clock.c
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

#ifndef _DEBUG_
    #include "config.h"
#endif

#include <glib/gi18n.h>

#ifdef CLOCK_USE_IDO_CALENDAR
    #include <libido/libido.h>
#endif

#include "shares.h"
#include "configuration.h"

/* Static variables */

static gboolean clock_stopped = FALSE;
static gulong visibility_notify_id = 0;

/* Static functions */

static gboolean clock_handler(gpointer* data);
#ifndef CLOCK_USE_IDO_CALENDAR
static GtkWidget* create_simple_calendar_item(GtkWidget** calendar_out);
#endif
static gboolean on_visibility_notify(GtkWidget* widget, GdkEvent* event, gpointer data);

/* ---------------------------------------------------------------------------*
 * Definitions: public
 * -------------------------------------------------------------------------- */

void init_clock_indicator()
{
    if(!config.clock.enabled)
    {
        gtk_widget_hide(greeter.ui.clock.time_widget);
        return;
    }

    if(config.clock.calendar)
    {
        GtkWidget* calendar_menuitem = NULL;

        #ifdef CLOCK_USE_IDO_CALENDAR
        calendar_menuitem = ido_calendar_menu_item_new();
        greeter.ui.clock.calendar_widget = ido_calendar_menu_item_get_calendar(IDO_CALENDAR_MENU_ITEM(calendar_menuitem));
        #else
        calendar_menuitem = create_simple_calendar_item(&greeter.ui.clock.calendar_widget);
        #endif
        greeter.ui.clock.date_widget = gtk_menu_item_new_with_label("");
        gtk_menu_shell_append(GTK_MENU_SHELL(greeter.ui.clock.time_menu), greeter.ui.clock.date_widget);
        gtk_menu_shell_append(GTK_MENU_SHELL(greeter.ui.clock.time_menu), calendar_menuitem);

        gtk_widget_add_events(greeter.ui.clock.date_widget, GDK_VISIBILITY_NOTIFY_MASK);
        visibility_notify_id = g_signal_connect(greeter.ui.clock.date_widget,
                                                "visibility-notify-event",
                                                G_CALLBACK(on_visibility_notify),
                                                NULL);

        gtk_widget_show_all(greeter.ui.clock.time_menu);
        if(GTK_IS_MENU_ITEM(greeter.ui.clock.time_widget))
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(greeter.ui.clock.time_widget), greeter.ui.clock.time_menu);
    }
    else if(GTK_IS_MENU_ITEM(greeter.ui.clock.time_widget))
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(greeter.ui.clock.time_widget), NULL);
    else
        gtk_widget_hide(greeter.ui.clock.time_menu);

    g_timeout_add_seconds(0, (GSourceFunc)clock_handler, NULL);
    g_timeout_add_seconds(1, (GSourceFunc)clock_handler, (gpointer)TRUE);
    gtk_widget_show(greeter.ui.clock.time_widget);
}

 /* ---------------------------------------------------------------------------*
 * Definitions: static
 * -------------------------------------------------------------------------- */

static gboolean clock_handler(gpointer* data)
{
    if(clock_stopped)
        return FALSE;
    GDateTime* datetime = g_date_time_new_now_local();
    if(!datetime)
    {
        set_widget_text(greeter.ui.clock.time_widget, "[time]");
        clock_stopped = TRUE;
        return FALSE;
    }
    gchar* str = g_date_time_format(datetime, config.clock.time_format);
    set_widget_text(greeter.ui.clock.time_widget, str);
    g_free(str);
    g_date_time_unref(datetime);
    return data != NULL;
}

#ifndef CLOCK_USE_IDO_CALENDAR
static GtkWidget* create_simple_calendar_item(GtkWidget** calendar_out)
{
    g_assert(calendar_out != NULL);

    GtkWidget* calendar = *calendar_out = gtk_calendar_new();
    gtk_calendar_set_display_options(GTK_CALENDAR(calendar),
                                     GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_NO_MONTH_CHANGE |
                                     GTK_CALENDAR_SHOW_DAY_NAMES | GTK_CALENDAR_SHOW_DETAILS);

    GtkWidget* menu_item = gtk_menu_item_new();
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_container_add(GTK_CONTAINER(menu_item), calendar);
    return menu_item;
}
#endif

static gboolean on_visibility_notify(GtkWidget* widget, GdkEvent* event, gpointer data)
{
    GDateTime* datetime = g_date_time_new_now_local();
    if(!datetime)
    {
        gtk_menu_item_set_label(GTK_MENU_ITEM(greeter.ui.clock.date_widget), "[date]");
        g_signal_handler_disconnect(greeter.ui.clock.date_widget, visibility_notify_id);
        return TRUE;
    }
    gchar* str = g_date_time_format(datetime, config.clock.date_format);
    gtk_menu_item_set_label(GTK_MENU_ITEM(greeter.ui.clock.date_widget), str);
    gtk_calendar_select_month(GTK_CALENDAR(greeter.ui.clock.calendar_widget),
                              g_date_time_get_month(datetime) - 1,
                              g_date_time_get_year(datetime));
    gtk_calendar_select_day(GTK_CALENDAR(greeter.ui.clock.calendar_widget), g_date_time_get_day_of_month(datetime));
    g_free(str);
    g_date_time_unref(datetime);
    return TRUE;
}

