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
gboolean lightdm_restart(GError** error)   { g_message("lightdm_restart()"); return TRUE; }
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
} PowerActionData;

/* Static variables */

static PowerActionData POWER_ACTIONS[POWER_ACTIONS_COUNT] =
{
    {
        .get_allow       = lightdm_get_can_suspend,
        .do_action       = lightdm_suspend,
        .show_prompt_ptr = &config.power.prompts[POWER_SUSPEND],
        .name            = N_("Suspend"),
        .prompt          = N_("Suspend the computer?"),
        .icon            = "gnome-session-suspend"
    },
    {
        .get_allow       = lightdm_get_can_hibernate,
        .do_action       = lightdm_hibernate,
        .show_prompt_ptr = &config.power.prompts[POWER_HIBERNATE],
        .name            = N_("Hibernate"),
        .prompt          = N_("Hibernate the computer?"),
        .icon            = "gnome-session-hibernate"
    },
    {
        .get_allow       = lightdm_get_can_restart,
        .do_action       = lightdm_restart,
        .show_prompt_ptr = &config.power.prompts[POWER_RESTART],
        .name            = N_("Restart"),
        .prompt          = N_("Are you sure you want to close all programs and restart the computer?"),
        .icon            = "system-reboot"
    },
    {
        .get_allow       = lightdm_get_can_shutdown,
        .do_action       = lightdm_shutdown,
        .show_prompt_ptr = &config.power.prompts[POWER_SHUTDOWN],
        .name            = N_("Shutdown"),
        .prompt          = N_("Are you sure you want to close all programs and shutdown the computer?"),
        .icon            = "system-shutdown"
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
        gtk_widget_set_visible(greeter.ui.power.actions_box[i], POWER_ACTIONS[i].get_allow());
        allow_any |= POWER_ACTIONS[i].get_allow();
    }

    if(!config.power.enabled || !allow_any)
    {
        if(!allow_any)
            g_warning("Power menu: no actions allowed, hiding widget");
        gtk_widget_hide(greeter.ui.power.box);
    }
    else if(GTK_IS_IMAGE_MENU_ITEM(greeter.ui.power.widget))
    {
        fix_image_menu_item_if_empty(GTK_IMAGE_MENU_ITEM(greeter.ui.power.widget));
    }
}

void power_shutdown(void)
{
    on_power_shutdown_activate(NULL, NULL);
}

/* ------------------------------------------------------------------------- *
 * Definitions: static
 * ------------------------------------------------------------------------- */

static void power_action(const PowerActionData* action)
{
    g_return_if_fail(config.power.enabled);

    if(*action->show_prompt_ptr)
    {
        gtk_widget_hide(greeter.ui.login_window);
        gtk_widget_set_sensitive(greeter.ui.power.widget, FALSE);

        GtkWidget* dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_QUESTION,
                                                   GTK_BUTTONS_NONE,
                                                   "%s", _(action->prompt));
        gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                               _("Return to Login"), GTK_RESPONSE_CANCEL,
                               _(action->name), GTK_RESPONSE_OK, NULL);
        gtk_widget_set_name(dialog, "power_dialog");
        gtk_window_set_title(GTK_WINDOW(dialog), action->name);
        setup_window(GTK_WINDOW(dialog));
        if(action->icon && gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), action->icon))
        {
            GtkWidget* image = gtk_image_new_from_icon_name(action->icon, GTK_ICON_SIZE_DIALOG);
            gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dialog), image);
        }
        gtk_widget_show_all(dialog);
        set_window_position(dialog, &WINDOW_POSITION_CENTER);

        gboolean result = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK;

        gtk_widget_destroy(dialog);
        update_windows_layout();
        gtk_widget_show(greeter.ui.login_window);
        gtk_widget_set_sensitive(greeter.ui.power.widget, TRUE);

        if(!result)
            return;
    }

    GError* error = NULL;
    if(!action->do_action(&error) && error)
    {
        g_warning(_("Action \"%s\" failed with error: %s."), _(action->name), error->message);
        show_error(_(action->name), _("Action \"%s\" failed with error: %s."), _(action->name), error->message);
        g_clear_error(&error);
    }
}

/* ------------------------------------------------------------------------- *
 * Definitions: exported
 * ------------------------------------------------------------------------- */

void on_power_suspend_activate(GtkWidget* widget,
                               gpointer* data)
{
    power_action(&POWER_ACTIONS[POWER_SUSPEND]);
}

void on_power_hibernate_activate(GtkWidget* widget,
                                 gpointer* data)
{
    power_action(&POWER_ACTIONS[POWER_HIBERNATE]);
}

void on_power_restart_activate(GtkWidget* widget,
                               gpointer* data)
{
    power_action(&POWER_ACTIONS[POWER_RESTART]);
}

void on_power_shutdown_activate(GtkWidget* widget,
                                gpointer* data)
{
    power_action(&POWER_ACTIONS[POWER_SHUTDOWN]);
}
