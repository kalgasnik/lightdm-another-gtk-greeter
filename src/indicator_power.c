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
#include "indicator_power.h"

#ifdef _DEBUG_
gboolean lightdm_suspend(GError** error)   { g_message("lightdm_suspend()"); return TRUE; }
gboolean lightdm_hibernate(GError** error) { g_message("lightdm_hibernate()"); return TRUE; }
gboolean lightdm_restart(GError** error)   { g_message("lightdm_restart()"); exit(EXIT_SUCCESS); }
gboolean lightdm_shutdown(GError** error)  { g_message("lightdm_shutdown()"); exit(EXIT_SUCCESS); }
#endif

/* Types */

typedef struct
{
    gboolean (*get_allow)(void);
    gboolean (*do_action)(GError**);
    gboolean* show_prompt_ptr;
    const gchar* name;
    const gchar* prompt;
    const gchar* icon;
    const gchar* button_name;
} PowerActionData;

/* Static variables */

static PowerActionData POWER_ACTIONS[POWER_ACTIONS_COUNT] =
{
    {
        .get_allow       = lightdm_get_can_suspend,
        .do_action       = lightdm_suspend,
        .show_prompt_ptr = &config.power.prompts[POWER_ACTION_SUSPEND],
        .name            = N_("Suspend"),
        .prompt          = N_("Suspend the computer?"),
        .icon            = "gnome-session-suspend",
        .button_name     = "power_dialog_suspend"
    },
    {
        .get_allow       = lightdm_get_can_hibernate,
        .do_action       = lightdm_hibernate,
        .show_prompt_ptr = &config.power.prompts[POWER_ACTION_HIBERNATE],
        .name            = N_("Hibernate"),
        .prompt          = N_("Hibernate the computer?"),
        .icon            = "gnome-session-hibernate",
        .button_name     = "power_dialog_hibernate"
    },
    {
        .get_allow       = lightdm_get_can_restart,
        .do_action       = lightdm_restart,
        .show_prompt_ptr = &config.power.prompts[POWER_ACTION_RESTART],
        .name            = N_("Restart"),
        .prompt          = N_("Are you sure you want to close all programs and restart the computer?"),
        .icon            = "system-reboot",
        .button_name     = "power_dialog_restart"
    },
    {
        .get_allow       = lightdm_get_can_shutdown,
        .do_action       = lightdm_shutdown,
        .show_prompt_ptr = &config.power.prompts[POWER_ACTION_SHUTDOWN],
        .name            = N_("Shutdown"),
        .prompt          = N_("Are you sure you want to close all programs and shutdown the computer?"),
        .icon            = "system-shutdown",
        .button_name     = "power_dialog_shutdown"
    }
};

/* GUI callbacks */

void on_power_suspend_activate             (GtkWidget* widget,
                                            gpointer* data);
void on_power_hibernate_activate           (GtkWidget* widget,
                                            gpointer* data);
void on_power_restart_activate             (GtkWidget* widget,
                                            gpointer* data);
void on_power_shutdown_activate            (GtkWidget* widget,
                                            gpointer* data);


/* Static functions */

static void power_action(const PowerActionData* action);

/* ------------------------------------------------------------------------- *
 * Definitions: public
 * ------------------------------------------------------------------------- */

void init_power_indicator(void)
{
    gboolean allow_any = FALSE;
    for(int i = 0; i < POWER_ACTIONS_COUNT; ++i)
    {
        gtk_widget_set_visible(greeter.ui.power.actions_box[i],
                               POWER_ACTIONS[i].get_allow() && config.power.enabled);
        allow_any |= POWER_ACTIONS[i].get_allow();
    }

    if(!config.power.enabled || !allow_any)
    {
        if(!allow_any)
            g_warning("Power menu: no actions allowed, hiding widget");
        gtk_widget_hide(greeter.ui.power.box);
    }
}

void do_power_action(PowerAction action)
{
    if(action != POWER_ACTION_NONE)
        power_action(&POWER_ACTIONS[action]);
}

/* ------------------------------------------------------------------------- *
 * Definitions: static
 * ------------------------------------------------------------------------- */

static void power_action(const PowerActionData* action)
{
    g_return_if_fail(config.power.enabled && action->get_allow());
    if(*action->show_prompt_ptr)
    {
        const MessageButtonOptions buttons[] =
        {
            {.id = GTK_RESPONSE_CANCEL, .text = _("Return to Login")},
            {.id = GTK_RESPONSE_YES,    .text = g_dgettext("gtk30", "_Yes"), .name = action->button_name},
            {.id = GTK_RESPONSE_NONE}
        };
        if(show_message(_(action->name), _(action->prompt), action->icon, buttons,
                        GTK_RESPONSE_CANCEL, GTK_RESPONSE_CANCEL) != GTK_RESPONSE_YES)
            return;
    }

    GError* error = NULL;
    if(!action->do_action(&error) && error)
    {
        g_warning("Action \"%s\" failed with error: %s.", action->name, error->message);
        show_message_dialog(GTK_MESSAGE_ERROR, _(action->name),
                            _("Action \"%s\" failed with error: %s."), _(action->name), error->message);
        g_clear_error(&error);
    }
}

/* ------------------------------------------------------------------------- *
 * Definitions: exported
 * ------------------------------------------------------------------------- */

void on_power_suspend_activate(GtkWidget* widget,
                               gpointer* data)
{
    power_action(&POWER_ACTIONS[POWER_ACTION_SUSPEND]);
}

void on_power_hibernate_activate(GtkWidget* widget,
                                 gpointer* data)
{
    power_action(&POWER_ACTIONS[POWER_ACTION_HIBERNATE]);
}

void on_power_restart_activate(GtkWidget* widget,
                               gpointer* data)
{
    power_action(&POWER_ACTIONS[POWER_ACTION_RESTART]);
}

void on_power_shutdown_activate(GtkWidget* widget,
                                gpointer* data)
{
    power_action(&POWER_ACTIONS[POWER_ACTION_SHUTDOWN]);
}
