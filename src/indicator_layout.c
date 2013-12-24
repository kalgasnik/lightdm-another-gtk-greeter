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
#include <X11/Xatom.h>
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

static KeyboardInfo* init_xkb          (void);
static void start_monitoring           (KeyboardInfo* kbd);
static gchar** get_layouts             (Display* display);
static GroupInfo* get_groups           (Display* display,
                                        int* count);
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

    KeyboardInfo* kbd = init_xkb();
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
        kbd->groups[i].menu_item = menu_item;
        g_signal_connect(G_OBJECT(menu_item), "toggled", G_CALLBACK(on_layout_clicked), (gpointer)&kbd->groups[i]);
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

static KeyboardInfo* init_xkb()
{
    Display* display;
    XklEngine* engine;
    GroupInfo* groups;
    int groups_count;

    display = XkbOpenDisplay(NULL, NULL, NULL, NULL, NULL, NULL);
    g_return_val_if_fail(display != NULL, NULL);
    engine = xkl_engine_get_instance(display);
    g_return_val_if_fail(display != NULL, NULL);
    groups = get_groups(display, &groups_count);
    g_return_val_if_fail(display != NULL, NULL);

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
    xkl_engine_start_listen(kbd->engine, XKLL_TRACK_KEYBOARD_STATE);
}

static gchar** get_layouts(Display* display)
{
    const int LAYOUTS_PROP_NUM = 2;
	Atom type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop;
    int status = XGetWindowProperty(display, RootWindow(display, DefaultScreen(display)),
                                    XInternAtom(display, "_XKB_RULES_NAMES", 1),
                                    0, 1024,          /* Offset, length */
                                    FALSE,            /* Delete */
                                    XA_STRING, &type, /* Requared type, actual type */
                                    &format,
                                    &nitems, &bytes_after,
                                    &prop);

    g_return_val_if_fail(status == Success, NULL);
    g_return_val_if_fail((int)nitems >= LAYOUTS_PROP_NUM, NULL);
    g_return_val_if_fail(format == 8, NULL);
    g_return_val_if_fail(type == XA_STRING, NULL);

    unsigned char *p = prop;
    for(int prop_n = 0; prop_n < LAYOUTS_PROP_NUM; ++prop_n)
        p += strlen((const char*)p) + 1;
    gchar** layouts = g_strsplit((const gchar*)p, ",", 0);
    XFree(prop);
    return layouts;
}

static GroupInfo* get_groups(Display* display,
                             int* count)
{
    gchar** short_names = get_layouts(display);
    g_return_val_if_fail(short_names != NULL, NULL);
    *count = g_strv_length(short_names);
    g_return_val_if_fail(*count > (config.layout.enabled_for_one ? 0 : 1), NULL);
    XkbDescPtr xkb = XkbAllocKeyboard();
    XkbGetNames(display, XkbGroupNamesMask, xkb);

    GroupInfo* groups = g_malloc(sizeof(GroupInfo)*(*count));
    for(int i = 0; i < *count; ++i)
    {
        char* name = XGetAtomName(display, xkb->names->groups[i]);
        groups[i].group_id = i;
        groups[i].name = g_strdup(name);
        groups[i].short_name = g_strdup(short_names[i]);
        XFree(name);
    }
    g_strfreev(short_names);
    XFree(xkb);
    return groups;
}

static void update_menu(GroupInfo* group_info)
{
    set_widget_text(greeter.ui.layout.widget, group_info->short_name);
    set_widget_toggled(group_info->menu_item, TRUE, G_CALLBACK(on_layout_clicked));
}
