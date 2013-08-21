/* indicator_layout.c
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

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libxklavier/xklavier.h>

#include "shares.h"
#include "configuration.h"
#include "indicator_layout.h"
/* Types */

struct _KeyboardInfo;

typedef struct _GroupInfo
{
    struct _KeyboardInfo* kbd;
    int group_id;
    const gchar* name;
    const gchar* short_name;
    GtkWidget* menu_item;
    gulong menu_signal_id;
} GroupInfo;

typedef struct _KeyboardInfo
{
    Display* display;
    XklEngine* engine;
    GroupInfo* groups;
    gint groups_count;
    gulong state_callback_id;
} KeyboardInfo;

/* Events */

static void on_layout_clicked          (GtkRadioMenuItem* menuitem,
                                        GroupInfo* group);
static void state_callback             (XklEngine* engine,
                                        XklEngineStateChange changeType,
                                        gint group,
                                        gboolean restore,
                                        KeyboardInfo* kbd);
static GdkFilterReturn xkb_evt_filter  (GdkXEvent* xev,
                                        GdkEvent* event,
                                        KeyboardInfo* kbd);

/* Static functions */

static KeyboardInfo* init_kbd          (void);
static void start_monitoring           (KeyboardInfo* kbd);
static unsigned char* get_short_names  (Display* display);
static GroupInfo* get_groups           (Display* display,
                                        int* size);
static void update_menu                (GroupInfo* group_info);

/* ------------------------------------------------------------------------- *
 * Definitions: public
 * ------------------------------------------------------------------------- */

void init_layout_indicator(void)
{
    if(!config.layout.enabled)
    {
        gtk_widget_hide(greeter.ui.layout.box);
        return;
    }

    KeyboardInfo* kbd = init_kbd();
    if(!kbd)
    {
        g_critical("Layout indicator: initialization failed, hiding widget");
        gtk_widget_hide(greeter.ui.layout.box);
        return;
    }

    GSList* menu_group = NULL;
    GtkWidget* menu_item = NULL;
    for(int i = 0; i < kbd->groups_count; i++)
    {
        menu_item = gtk_radio_menu_item_new_with_label(menu_group, kbd->groups[i].name);
        menu_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menu_item));

        kbd->groups[i].menu_signal_id = g_signal_connect(G_OBJECT(menu_item),
                                                         "toggled",
                                                         G_CALLBACK(on_layout_clicked),
                                                         (gpointer)&kbd->groups[i]);
        kbd->groups[i].menu_item = menu_item;

        gtk_menu_shell_append(GTK_MENU_SHELL(greeter.ui.layout.menu), menu_item);
    }

    start_monitoring(kbd);

    gtk_widget_show_all(greeter.ui.layout.box);
    gtk_widget_show_all(greeter.ui.layout.menu);
}

/* ------------------------------------------------------------------------- *
 * Definitions: events
 * ------------------------------------------------------------------------- */

static void on_layout_clicked(GtkRadioMenuItem* menu_item,
                              GroupInfo* group)
{
    if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)))
    {
        xkl_engine_allow_one_switch_to_secondary_group(group->kbd->engine);
        xkl_engine_lock_group(group->kbd->engine, group->group_id);
    }
}

static void state_callback(XklEngine* engine,
                           XklEngineStateChange changeType,
                           gint group,
                           gboolean restore,
                           KeyboardInfo* kbd)
{
	if(changeType == GROUP_CHANGED)
        update_menu(&kbd->groups[group]);
}

static GdkFilterReturn xkb_evt_filter(GdkXEvent* xev,
                                      GdkEvent* event,
                                      KeyboardInfo* kbd)
{
	xkl_engine_filter_events(kbd->engine, (XEvent*)xev);
	return GDK_FILTER_CONTINUE;
}

/* ------------------------------------------------------------------------- *
 * Definitions: static
 * ------------------------------------------------------------------------- */

static KeyboardInfo* init_kbd()
{
    Display* display = XkbOpenDisplay(NULL, NULL, NULL, NULL, NULL, NULL);
    if(!display)
    {
        g_warning("Layout indicator is not inited: cann't open display");
        return NULL;
    }

    XklEngine* engine = xkl_engine_get_instance(display);
    if(!engine)
    {
        g_warning("Layout indicator is not inited: cann't get Xkl engine");
        return NULL;
    }

    int groups_count = 0;
    GroupInfo* groups = get_groups(display, &groups_count);
    if(!groups)
    {
        g_warning("Layout indicator is not inited: cann't get layouts");
        return NULL;
    }

    KeyboardInfo* kbd = g_malloc(sizeof(KeyboardInfo));
    kbd->display = display;
    kbd->engine = engine;
    kbd->groups = groups;
    kbd->groups_count = groups_count;
    kbd->state_callback_id = g_signal_connect(kbd->engine, "X-state-changed", G_CALLBACK(state_callback), (gpointer)kbd);

    for(int i = 0; i < groups_count; ++i)
        groups[i].kbd = kbd;

    return kbd;
}

static void start_monitoring(KeyboardInfo* kbd)
{
    XkbStateRec state;
    XkbGetState(kbd->display, XkbUseCoreKbd, &state);
    update_menu(&kbd->groups[state.group]);
    gdk_window_add_filter(NULL, (GdkFilterFunc)xkb_evt_filter, (gpointer)kbd);
    xkl_engine_start_listen(kbd->engine, XKLL_MANAGE_WINDOW_STATES);
}

static unsigned char* get_short_names(Display* display)
{
	Atom type;
    int format;
    unsigned long nitems;
    unsigned long nbytes;
    unsigned long bytes_after;
    unsigned char *prop;
    int status = XGetWindowProperty(
                    display,                                        // Display
                    RootWindow(display, DefaultScreen(display)),    // Window
                    XInternAtom(display, "_XKB_RULES_NAMES", 1),    // Property
                    0,              // Offset
                    1024,           // Length
                    FALSE,          // Delete
                    AnyPropertyType,// Requared type
                    &type,          // Actual type
                    &format,
                    &nitems, &bytes_after,
                    &prop);

    g_return_val_if_fail(status == Success, NULL);

	if(format == 32)
		nbytes = sizeof(long);
	else if(format == 16)
		nbytes = sizeof(short);
	else if(format == 8)
		nbytes = sizeof(char);
	else
		return NULL;

    int prop_count = -1;
	long length = nitems*nbytes;
	unsigned char *prop_value = NULL;

    // Keyboard Layouts: property #2
    while(length >= format/8)
    {
        prop_value = prop;
        for(;*prop++ != '\0'; length--)
            continue;
        if(++prop_count == 2)
            break;
    }
    return prop_value;
}

static GroupInfo* get_groups(Display* display,
                             int* size)
{
    unsigned char* short_names_str = get_short_names(display);
    if(!short_names_str)
        return NULL;

    gchar** short_names = g_strsplit((char*)short_names_str, ",", 0);
    int groups_count = 0;
    while(short_names[groups_count])
        groups_count++;

    XkbDescPtr xkb = XkbAllocKeyboard();
    XkbGetNames(display, XkbGroupNamesMask, xkb);

    GroupInfo* groups = g_malloc(sizeof(GroupInfo)*groups_count);
    for(int i = 0; i < groups_count; ++i)
    {
        char* name = XGetAtomName(display, xkb->names->groups[i]);
        groups[i].group_id = i;
        groups[i].name = g_strdup(name);
        groups[i].short_name = g_strdup(short_names[i]);
        XFree(name);
    }
    g_strfreev(short_names);
    XFree(xkb);

    *size = groups_count;
    return groups;
}

static void update_menu(GroupInfo* group_info)
{
    set_widget_text(greeter.ui.layout.widget, group_info->short_name);

    g_signal_handler_block(group_info->menu_item, group_info->menu_signal_id);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(group_info->menu_item), TRUE);
    g_signal_handler_unblock(group_info->menu_item, group_info->menu_signal_id);
}


