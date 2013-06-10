/* indicator_power.c
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
#include <glib/gi18n.h>
#include <stdlib.h>
#include <lightdm.h>

#include "shares.h"
#include "configuration.h"

#define __(x) (x)

#ifdef _DEBUG_
gboolean lightdm_suspend(GError** error) { g_message("lightdm_suspend()"); return TRUE; }
gboolean lightdm_hibernate(GError** error) { g_message("lightdm_hibernate()"); return TRUE; }
gboolean lightdm_restart(GError** error) { g_message("lightdm_restart()"); return TRUE; }
gboolean lightdm_shutdown(GError** error) { g_message("lightdm_shutdown()"); exit(EXIT_SUCCESS); }
#endif

G_MODULE_EXPORT void on_power_suspend_activate(GtkWidget* widget, gpointer* data);
G_MODULE_EXPORT void on_power_hibernate_activate(GtkWidget* widget, gpointer* data);
G_MODULE_EXPORT void on_power_restart_activate(GtkWidget* widget, gpointer* data);
G_MODULE_EXPORT void on_power_shutdown_activate(GtkWidget* widget, gpointer* data);

/* Types */

typedef struct
{
    gboolean (*action)(GError**);
    gboolean* show_prompt;
    const gchar* name;
    const gchar* prompt;
    const gchar* title;
} PowerActionData;

typedef enum
{
    POWER_SUSPEND,
    POWER_HIBERNATE,
    POWER_RESTART,
    POWER_SHUTDOWN,
    POWER_ACTIONS_COUNT
} PowerAction;

/* Static variables */

static PowerActionData POWER_ACTIONS[POWER_ACTIONS_COUNT] =
{
    {
        lightdm_suspend, &config.power.suspend_prompt,
        __("Suspend"), __("Suspend the computer?"), __("Suspend")
    },
    {
        lightdm_hibernate, &config.power.hibernate_prompt,
        __("Hibernate"), __("Hibernate the computer?"), __("Hibernate")
    },
    {
        lightdm_restart, &config.power.restart_prompt,
        __("Restart"), __("Are you sure you want to close all programs and restart the computer?"), __("Restart")
    },
    {
        lightdm_shutdown, &config.power.shutdown_prompt,
        __("Shutdown"), __("Are you sure you want to close all programs and shutdown the computer?"), __("Shutdown")
    }
};

/* Exported functions */

G_MODULE_EXPORT void on_suspend_activate(GtkWidget* widget, gpointer* data);
G_MODULE_EXPORT void on_hibernate_activate(GtkWidget* widget, gpointer* data);
G_MODULE_EXPORT void on_restart_activate(GtkWidget* widget, gpointer* data);
G_MODULE_EXPORT void on_shutdown_activate(GtkWidget* widget, gpointer* data);

/* Static functions */

/* Return TRUE if user agreed to perfom action */
static gboolean prompt(const PowerActionData* action);
static void power_action(PowerAction);

/* ------------------------------------------------------------------------- *
 * Definitions: public
 * ------------------------------------------------------------------------- */

void init_power_indicator()
{
    if(!config.power.enabled)
    {
        gtk_widget_hide(greeter.ui.power.power_widget);
        return;
    }

    gboolean allow_suspend = lightdm_get_can_suspend();
    gboolean allow_hibernate = lightdm_get_can_hibernate();
    gboolean allow_restart = lightdm_get_can_restart();
    gboolean allow_shutdown = lightdm_get_can_shutdown();

    if(!(allow_suspend || allow_hibernate ||
         allow_restart || allow_shutdown))
    {
        g_warning("Power menu: no actions allowed, hiding widget");
        gtk_widget_hide(greeter.ui.power.power_widget);
        return;
    }

    gtk_widget_set_visible(greeter.ui.power.suspend_widget, allow_suspend);
    gtk_widget_set_visible(greeter.ui.power.hibernate_widget, allow_hibernate);
    gtk_widget_set_visible(greeter.ui.power.restart_widget, allow_restart);
    gtk_widget_set_visible(greeter.ui.power.shutdown_widget, allow_shutdown);

    if(greeter.ui.power.power_menu_icon && GTK_IS_MENU_ITEM(greeter.ui.power.power_widget))
        replace_container_content(greeter.ui.power.power_widget, greeter.ui.power.power_menu_icon);

    for(int i = 0; i < POWER_ACTIONS_COUNT; ++i)
    {
        POWER_ACTIONS[i].name = gettext(POWER_ACTIONS[i].name);
        POWER_ACTIONS[i].prompt = gettext(POWER_ACTIONS[i].prompt);
        POWER_ACTIONS[i].title = gettext(POWER_ACTIONS[i].title);
    }
}

/* ------------------------------------------------------------------------- *
 * Definitions: exported
 * ------------------------------------------------------------------------- */

G_MODULE_EXPORT void on_power_suspend_activate(GtkWidget* widget, gpointer* data)
{
    power_action(POWER_SUSPEND);
}

G_MODULE_EXPORT void on_power_hibernate_activate(GtkWidget* widget, gpointer* data)
{
    power_action(POWER_HIBERNATE);
}

G_MODULE_EXPORT void on_power_restart_activate(GtkWidget* widget, gpointer* data)
{
    power_action(POWER_RESTART);
}

G_MODULE_EXPORT void on_power_shutdown_activate(GtkWidget* widget, gpointer* data)
{
    power_action(POWER_SHUTDOWN);
}

/* ------------------------------------------------------------------------- *
 * Definitions: static
 * ------------------------------------------------------------------------- */

static gboolean prompt(const PowerActionData* action)
{
    gtk_widget_hide(greeter.ui.login_window);
    gtk_widget_set_sensitive(greeter.ui.power.power_widget, FALSE);

    GtkWidget* dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_NONE,
                                    "%s", action->prompt);
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           _("Return to Login"), GTK_RESPONSE_CANCEL,
                           _(action->name), GTK_RESPONSE_OK, NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), action->name);
    gtk_widget_show_all(dialog);
    center_window(dialog);

    gboolean result = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK;

    gtk_widget_destroy(dialog);
    center_window(greeter.ui.login_window);
    gtk_widget_show(greeter.ui.login_window);
    gtk_widget_set_sensitive(greeter.ui.power.power_widget, TRUE);

    return result;
}

static void power_action(PowerAction action_num)
{
    const PowerActionData* action = &POWER_ACTIONS[action_num];
    if(!*action->show_prompt || prompt(action))
    {
        GError* error = NULL;
        /* FIX: lightdm_[action] return FALSE, even in case of successful call */
        if(!action->action(&error) && error)
        {
            gchar* error_text;
            if(error)
            {
                g_warning("Action \"%s\" failed with error: %s.", _(action->name), error->message);
                error_text = g_strdup_printf(_("Action \"%s\" failed with error: %s."), _(action->name), error->message);
            }
            else
            {
                g_warning("Action \"%s\" failed.", _(action->name));
                error_text = g_strdup_printf(_("Action \"%s\" failed."), _(action->name));
            }
            GtkWidget* dialog = gtk_message_dialog_new(NULL,
                                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                                       GTK_MESSAGE_ERROR,
                                                       GTK_BUTTONS_OK,
                                                       "%s", error_text);
            gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
            gtk_dialog_run(GTK_DIALOG(dialog));

            gtk_widget_destroy(dialog);
            g_free(error_text);
            g_clear_error(&error);
        }
    }
}

