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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>

#include <gtk/gtk.h>
#include <glib.h>

#include <libbladeutil/libbladeutil.h>
#include <libbladeui/libbladeui.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "blpm-dbus.h"
#include "blpm-debug.h"
#include "blpm-common.h"

#include "blade-pm-dbus-client.h"
#include "blpm-manager.h"

static void G_GNUC_NORETURN
show_version (void)
{
    g_print (_("\n"
             "Xfce Power Manager %s\n\n"
             "Part of the Xfce Goodies Project\n"
             "http://goodies.xfce.org\n\n"
             "Licensed under the GNU GPL.\n\n"), VERSION);

    exit (EXIT_SUCCESS);
}

static void
blpm_quit_signal (gint sig, gpointer data)
{
    XfpmManager *manager = (XfpmManager *) data;
    
    XFPM_DEBUG ("sig %d", sig);
    
    if ( sig != SIGHUP )
	blpm_manager_stop (manager);
}

static const gchar *
blpm_bool_to_local_string (gboolean value)
{
    return value == TRUE ? _("True") : _("False");
}

static void
blpm_dump (GHashTable *hash)
{
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
    
    has_battery = blpm_string_to_bool (g_hash_table_lookup (hash, "has-battery"));
    has_lid = blpm_string_to_bool (g_hash_table_lookup (hash, "has-lid"));
    can_suspend = blpm_string_to_bool (g_hash_table_lookup (hash, "can-suspend"));
    can_hibernate = blpm_string_to_bool (g_hash_table_lookup (hash, "can-hibernate"));
    auth_suspend = blpm_string_to_bool (g_hash_table_lookup (hash, "auth-suspend"));
    auth_hibernate = blpm_string_to_bool (g_hash_table_lookup (hash, "auth-hibernate"));
    has_lcd_brightness = blpm_string_to_bool (g_hash_table_lookup (hash, "has-brightness"));
    has_sleep_button = blpm_string_to_bool (g_hash_table_lookup (hash, "sleep-button"));
    has_power_button = blpm_string_to_bool (g_hash_table_lookup (hash, "power-button"));
    has_hibernate_button = blpm_string_to_bool (g_hash_table_lookup (hash, "hibernate-button"));
    can_shutdown = blpm_string_to_bool (g_hash_table_lookup (hash, "can-shutdown"));
	
    g_print ("---------------------------------------------------\n");
    g_print ("       Xfce power manager version %s\n", VERSION);
#ifdef ENABLE_POLKIT
    g_print (_("With policykit support\n"));
#else
    g_print (_("Without policykit support\n"));
#endif
#ifdef WITH_NETWORK_MANAGER
    g_print (_("With network manager support\n"));
#else
    g_print (_("Without network manager support\n"));
#endif
    g_print ("---------------------------------------------------\n");
    g_print ( "%s: %s\n"
	      "%s: %s\n"
	      "%s: %s\n"
	      "%s: %s\n"
	      "%s: %s\n"
	      "%s: %s\n"
	      "%s: %s\n"
	      "%s: %s\n"
	      "%s: %s\n"
	      "%s: %s\n"
	      "%s: %s\n",
	     _("Can suspend"),
	     blpm_bool_to_local_string (can_suspend),
	     _("Can hibernate"),
	     blpm_bool_to_local_string (can_hibernate),
	     _("Authorized to suspend"),
	     blpm_bool_to_local_string (auth_suspend),
	     _("Authorized to hibernate"),
	     blpm_bool_to_local_string (auth_hibernate),
	     _("Authorized to shutdown"),
	     blpm_bool_to_local_string (can_shutdown),
	     _("Has battery"),
	     blpm_bool_to_local_string (has_battery),
	     _("Has brightness bar"),
	     blpm_bool_to_local_string (has_lcd_brightness),
	     _("Has power button"),
	     blpm_bool_to_local_string (has_power_button),
	     _("Has hibernate button"),
	     blpm_bool_to_local_string (has_hibernate_button),
	     _("Has sleep button"),
	      blpm_bool_to_local_string (has_sleep_button),
	     _("Has LID"),
	      blpm_bool_to_local_string (has_lid));
}

static void
blpm_dump_remote (DBusGConnection *bus)
{
    DBusGProxy *proxy;
    GError *error = NULL;
    GHashTable *hash;
    
    proxy = dbus_g_proxy_new_for_name (bus,
				       "org.blade.PowerManager",
				       "/org/blade/PowerManager",
				       "org.blade.Power.Manager");
	
    blpm_manager_dbus_client_get_config (proxy, 
					 &hash,
					 &error);
					     
    g_object_unref (proxy);
    
    if ( error )
    {
	g_error ("%s", error->message);
	exit (EXIT_FAILURE);
    }
    
    blpm_dump (hash);
    g_hash_table_destroy (hash);
}

static void G_GNUC_NORETURN
blpm_start (DBusGConnection *bus, const gchar *client_id, gboolean dump)
{
    XfpmManager *manager;
    GError *error = NULL;
    
    XFPM_DEBUG ("Starting the power manager");
    
    manager = blpm_manager_new (bus, client_id);

    if ( xfce_posix_signal_handler_init (&error)) 
    {
        xfce_posix_signal_handler_set_handler (SIGHUP,
                                               blpm_quit_signal,
                                               manager, NULL);

        xfce_posix_signal_handler_set_handler (SIGINT,
                                               blpm_quit_signal,
					       manager, NULL);

        xfce_posix_signal_handler_set_handler (SIGTERM,
                                               blpm_quit_signal,
                                               manager, NULL);
    } 
    else 
    {
        g_warning ("Unable to set up POSIX signal handlers: %s", error->message);
        g_error_free (error);
    }

    blpm_manager_start (manager);
    
    if ( dump )
    {
	GHashTable *hash;
	hash = blpm_manager_get_config (manager);
	blpm_dump (hash);
	g_hash_table_destroy (hash);
    }

    
    gtk_main ();
    
    g_object_unref (manager);

    exit (EXIT_SUCCESS);
}

int main (int argc, char **argv)
{
    DBusGConnection *bus;
    GError *error = NULL;
    DBusGProxy *proxy;
    GOptionContext *octx;
     
    gboolean run        = FALSE;
    gboolean quit       = FALSE;
    gboolean config     = FALSE;
    gboolean version    = FALSE;
    gboolean reload     = FALSE;
    gboolean no_daemon  = FALSE;
    gboolean debug      = FALSE;
    gboolean dump       = FALSE;
    gchar   *client_id  = NULL;
    
    GOptionEntry option_entries[] = 
    {
	{ "run",'r', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &run, NULL, NULL },
	{ "no-daemon",'\0' , G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &no_daemon, N_("Do not daemonize"), NULL },
	{ "debug",'\0' , G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &debug, N_("Enable debugging"), NULL },
	{ "dump",'\0' , G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &dump, N_("Dump all information"), NULL },
	{ "restart", '\0', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &reload, N_("Restart the running instance of Xfce power manager"), NULL},
	{ "customize", 'c', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &config, N_("Show the configuration dialog"), NULL },
	{ "quit", 'q', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &quit, N_("Quit any running xfce power manager"), NULL },
	{ "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &version, N_("Version information"), NULL },
	{ "sm-client-id", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &client_id, NULL, NULL },
	{ NULL, },
    };

#if !GLIB_CHECK_VERSION (2, 32, 0)
    if ( !g_thread_supported () )
        g_thread_init (NULL);
#endif

    /* Parse the options */
    octx = g_option_context_new("");
    g_option_context_set_ignore_unknown_options(octx, TRUE);
    g_option_context_add_main_entries(octx, option_entries, NULL);
    g_option_context_add_group(octx, xfce_sm_client_get_option_group(argc, argv));
    /* We can't add the following command because it will invoke gtk_init
       before we have a chance to fork.
       g_option_context_add_group(octx, gtk_get_option_group(TRUE));
     */

    if(!g_option_context_parse(octx, &argc, &argv, &error)) {
        g_printerr(_("Failed to parse arguments: %s\n"), error->message);
        g_option_context_free(octx);
        g_error_free(error);

        return EXIT_FAILURE;
    }

    g_option_context_free(octx);

    if ( version )    
        show_version ();

    /* Fork if needed */
    if ( dump == FALSE && debug == FALSE && no_daemon == FALSE && daemon(0,0) )
    {
        g_critical ("Could not daemonize");
    }

    /* Initialize */
    dbus_g_thread_init ();

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    g_set_application_name (PACKAGE_NAME);

    if (!gtk_init_check (&argc, &argv))
    {
        if (G_LIKELY (error))
        {
            g_printerr ("%s: %s.\n", G_LOG_DOMAIN, error->message);
            g_printerr (_("Type '%s --help' for usage."), G_LOG_DOMAIN);
            g_printerr ("\n");
            g_error_free (error);
        }
        else
        {
            g_error ("Unable to open display.");
        }

        return EXIT_FAILURE;
    }
    
    blpm_debug_init (debug);
    
    bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
            
    if ( error )
    {
	xfce_dialog_show_error (NULL, 
				error, 
				"%s",
				_("Unable to get connection to the message bus session"));
	g_error ("%s: \n", error->message);
    }
    
    if ( quit )
    {
	if (!blpm_dbus_name_has_owner (dbus_g_connection_get_connection(bus), 
				       "org.blade.PowerManager") )
        {
            g_print (_("Xfce power manager is not running"));
	    g_print ("\n");
            return EXIT_SUCCESS;
        }
	else
	{
	    proxy = dbus_g_proxy_new_for_name (bus, 
			                       "org.blade.PowerManager",
					       "/org/blade/PowerManager",
					       "org.blade.Power.Manager");
	    if ( !proxy ) 
	    {
		g_critical ("Failed to get proxy");
		dbus_g_connection_unref(bus);
            	return EXIT_FAILURE;
	    }
	    blpm_manager_dbus_client_quit (proxy , &error);
	    g_object_unref (proxy);
	    
	    if ( error)
	    {
		g_critical ("Failed to send quit message %s:\n", error->message);
		g_error_free (error);
	    }
	}
	return EXIT_SUCCESS;
    }
    
    if ( config )
    {
	g_spawn_command_line_async ("blade-pm-settings", &error);
	
	if ( error )
	{
	    g_critical ("Failed to execute blade-pm-settings: %s", error->message);
	    g_error_free (error);
	    return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
    }
    
    if ( reload )
    {
	if (!blpm_dbus_name_has_owner (dbus_g_connection_get_connection (bus), "org.blade.PowerManager") &&
	    !blpm_dbus_name_has_owner (dbus_g_connection_get_connection (bus), "org.freedesktop.PowerManagement"))
	{
	    g_print ("Xfce power manager is not running\n");
	    blpm_start (bus, client_id, dump);
	}
	
	proxy = dbus_g_proxy_new_for_name (bus, 
			                   "org.blade.PowerManager",
					   "/org/blade/PowerManager",
					   "org.blade.Power.Manager");
	if ( !proxy ) 
	{
	    g_critical ("Failed to get proxy");
	    dbus_g_connection_unref (bus);
	    return EXIT_FAILURE;
	}
	    
	if ( !blpm_manager_dbus_client_restart (proxy, NULL) )
	{
	    g_critical ("Unable to send reload message");
	    g_object_unref (proxy);
	    dbus_g_connection_unref (bus);
	    return EXIT_SUCCESS;
	}
	return EXIT_SUCCESS;
    }
    
    if (dump)
    {
	if (blpm_dbus_name_has_owner (dbus_g_connection_get_connection (bus), 
				      "org.blade.PowerManager"))
	{
	    blpm_dump_remote (bus);
	    return EXIT_SUCCESS;
	}
    }
    
    if (blpm_dbus_name_has_owner (dbus_g_connection_get_connection (bus), "org.freedesktop.PowerManagement") )
    {
	g_print ("%s: %s\n", 
		 _("Xfce Power Manager"),
		 _("Another power manager is already running"));
		  
    }
    else if (blpm_dbus_name_has_owner (dbus_g_connection_get_connection(bus), 
				       "org.blade.PowerManager"))
    {
	g_print (_("Xfce power manager is already running"));
	g_print ("\n");
    	return EXIT_SUCCESS;
    }
    else
    {	
	blpm_start (bus, client_id, dump);
    }
    
    return EXIT_SUCCESS;
}
