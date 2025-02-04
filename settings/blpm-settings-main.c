/*
 * * Copyright (C) 2008-2011 Ali <aliov@xfce.org>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>

#include <libbladeutil/libbladeutil.h>
#include <libbladeui/libbladeui.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <blconf/blconf.h>

#include "common/blpm-common.h"

#include "blade-pm-dbus-client.h"
#include "blpm-settings.h"
#include "blpm-config.h"
#include "blpm-dbus.h"
#include "blpm-debug.h"
#include "blpm-unique.h"

int main (int argc, char **argv)
{
    GtkWidget *dialog;
    XfpmUnique *unique;

    GError *error = NULL;
    DBusGConnection *bus;
    GHashTable *config_hash;
    
    gboolean has_battery;
    gboolean auth_suspend;
    gboolean auth_hibernate;
    gboolean can_suspend;
    gboolean can_hibernate;
    gboolean can_shutdown;
    gboolean has_lcd_brightness;
    gboolean has_sleep_button;
    gboolean has_hibernate_button;
    gboolean has_power_button;
    gboolean has_lid;
    gboolean start_blpm_if_not_running;
    gboolean debug = FALSE;

    GdkNativeWindow socket_id = 0;
    gchar *device_id = NULL;

    BlconfChannel *channel;
    DBusGProxy *proxy;
    
    GOptionEntry option_entries[] = 
    {
	{ "socket-id", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &socket_id, N_("Settings manager socket"), N_("SOCKET ID") },
	{ "device-id", 'd', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_STRING, &device_id, N_("Display a specific device by UpDevice object path"), N_("UpDevice object path") },
	{ "debug",    '\0', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &debug, N_("Enable debugging"), NULL },
	{ NULL, },
    };

    xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");
    
    if( !gtk_init_with_args (&argc, &argv, (gchar *)"", option_entries, (gchar *)PACKAGE, &error)) 
    {
        if( error) 
        {
            g_printerr("%s: %s.\n", G_LOG_DOMAIN, error->message);
            g_printerr(_("Type '%s --help' for usage."), G_LOG_DOMAIN);
            g_printerr("\n");
            g_error_free(error);
        }
        else
        {
            g_error("Unable to open display.");
	}
        return EXIT_FAILURE;
    }
    
    bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    
    if ( error )
    {
	g_error ("%s\n",error->message);
    }

    while ( !blpm_dbus_name_has_owner (dbus_g_connection_get_connection(bus), "org.blade.PowerManager") )
    {
	g_print(_("Xfce power manager is not running"));
	g_print("\n");
	start_blpm_if_not_running =
	    xfce_dialog_confirm (NULL, 
				 GTK_STOCK_EXECUTE,
				 _("Run"), NULL,
				 _("Xfce4 Power Manager is not running, do you want to launch it now?"));

	if ( start_blpm_if_not_running ) 
	{
	    g_spawn_command_line_async("blade-pm",NULL);
	    /* wait 2 seconds for blpm to startup */
	    g_usleep ( 2 * 1000000 );
	}
	else
	{
	    /* continue without starting blpm, this will probably error out */
	    break;
	}
    }

    unique = blpm_unique_new ("org.blade.PowerManager.Config");

    if ( !blpm_unique_app_is_running (unique) )
    {
	if ( !blconf_init(&error) )
	{
	    g_critical("blconf init failed: %s using default settings\n", error->message);
	    xfce_dialog_show_warning (NULL,
				      _("Xfce Power Manager"),
				      "%s",
				      _("Failed to load power manager configuration, using defaults"));
	    g_error_free (error);
	    error = NULL;
	    return EXIT_FAILURE;
	}

#if !GLIB_CHECK_VERSION (2, 32, 0)
    if ( !g_thread_supported () )
        g_thread_init (NULL);
#endif

	dbus_g_thread_init ();

	channel = blconf_channel_new(XFPM_CHANNEL_CFG);

	proxy = dbus_g_proxy_new_for_name(bus,
					   "org.blade.PowerManager",
					   "/org/blade/PowerManager",
					   "org.blade.Power.Manager");

	blpm_manager_dbus_client_get_config (proxy,
					     &config_hash,
					     &error);

	if ( error )
	{
	    g_critical ("Unable to get configuration information from xfce power manager: %s", error->message);
	    xfce_dialog_show_error (NULL, error, "%s", _("Unable to connect to Xfce Power Manager"));
	    g_error_free (error);
	    return EXIT_FAILURE;
	}

	blpm_debug_init (debug);

	has_battery = blpm_string_to_bool (g_hash_table_lookup (config_hash, "has-battery"));
	has_lid = blpm_string_to_bool (g_hash_table_lookup (config_hash, "has-lid"));
	can_suspend = blpm_string_to_bool (g_hash_table_lookup (config_hash, "can-suspend"));
	can_hibernate = blpm_string_to_bool (g_hash_table_lookup (config_hash, "can-hibernate"));
	auth_suspend = blpm_string_to_bool (g_hash_table_lookup (config_hash, "auth-suspend"));
	auth_hibernate = blpm_string_to_bool (g_hash_table_lookup (config_hash, "auth-hibernate"));
	has_lcd_brightness = blpm_string_to_bool (g_hash_table_lookup (config_hash, "has-brightness"));
	has_sleep_button = blpm_string_to_bool (g_hash_table_lookup (config_hash, "sleep-button"));
	has_power_button = blpm_string_to_bool (g_hash_table_lookup (config_hash, "power-button"));
	has_hibernate_button = blpm_string_to_bool (g_hash_table_lookup (config_hash, "hibernate-button"));
	can_shutdown = blpm_string_to_bool (g_hash_table_lookup (config_hash, "can-shutdown"));

	g_hash_table_destroy (config_hash);

	dialog = blpm_settings_dialog_new (channel, auth_suspend, auth_hibernate,
					   can_suspend, can_hibernate, can_shutdown, has_battery, has_lcd_brightness,
					   has_lid, has_sleep_button, has_hibernate_button, has_power_button,
					   socket_id, device_id);

	g_signal_connect_swapped (unique, "ping-received",
				  G_CALLBACK (gtk_window_present), dialog);

	gtk_main();

	blpm_dbus_release_name(dbus_g_connection_get_connection(bus), "org.blade.PowerManager.Config");
	dbus_g_connection_unref (bus);
	g_object_unref (proxy);

	g_object_unref (unique);

	return EXIT_SUCCESS;
    }
    
    return EXIT_SUCCESS;
}
