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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>

#include <libbladeutil/libbladeutil.h>

#include "blpm-common.h"

const gchar *blpm_bool_to_string (gboolean value)
{
    if ( value == TRUE ) return "TRUE";
    else 		 return "FALSE";
}

gboolean blpm_string_to_bool (const gchar *string)
{
    if ( !g_strcmp0 (string, "TRUE") ) return TRUE;
    else if ( !g_strcmp0 (string, "FALSE") ) return FALSE;
    
    return FALSE;
}

GtkBuilder *blpm_builder_new_from_string (const gchar *ui, GError **error)
{
    GtkBuilder *builder;

    builder = gtk_builder_new ();
    
    gtk_builder_add_from_string (GTK_BUILDER (builder),
                                 ui,
                                 -1,
                                 error);
    
    return builder;
}

gboolean
blpm_lock_screen (void)
{
    gboolean ret = g_spawn_command_line_async ("xflock4", NULL);
    
    if ( !ret )
    {
        ret = g_spawn_command_line_async ("gnome-screensaver-command -l", NULL);
    }
    
    if ( !ret )
    {
        /* this should be the default*/
        ret = g_spawn_command_line_async ("xdg-screensaver lock", NULL);
    }
    
    if ( !ret )
    {
        ret = g_spawn_command_line_async ("xscreensaver-command -lock", NULL);
    }
    
    if ( !ret )
    {
        g_critical ("Connot lock screen\n");
    }

    return ret;
}

void       
blpm_preferences (void) 
{
    g_spawn_command_line_async ("blade-pm-settings", NULL);
}

void
blpm_preferences_device_id (const gchar* object_path)
{
    gchar *string = g_strdup_printf("blade-pm-settings -d %s", object_path);

    if (string)
	g_spawn_command_line_async (string, NULL);

    g_free (string);
}

void
blpm_quit (void)
{
    g_spawn_command_line_async ("blade-pm -q", NULL);
}

void       
blpm_about (GtkWidget *widget, gpointer data)
{
    gchar *package = (gchar *)data;
    
    const gchar* authors[3] = 
    {
	"Ali Abdallah <aliov@xfce.org>", 
	 NULL
    };
							    
    static const gchar *documenters[] =
    {
	"Ali Abdallah <aliov@xfce.org>",
	NULL,
    };

    gtk_show_about_dialog (NULL,
		     "authors", authors,
		     "copyright", "Copyright \302\251 2008-2011 Ali Abdallah",
		     "destroy-with-parent", TRUE,
		     "documenters", documenters,
		     "license", XFCE_LICENSE_GPL,
		     "program-name", package,
		     "translator-credits", _("translator-credits"),
		     "version", PACKAGE_VERSION,
		     "website", "http://goodies.xfce.org/projects/applications/blade-pm",
		     NULL);
						 
}

gboolean blpm_is_multihead_connected (void)
{
    GdkDisplay *dpy;
    GdkScreen *screen;
    gint nscreen;
    gint nmonitor;
    
    dpy = gdk_display_get_default ();
    
    nscreen = gdk_display_get_n_screens (dpy);
    
    if ( nscreen == 1 )
    {
	screen = gdk_display_get_screen (dpy, 0);
	if ( screen )
	{
	    nmonitor = gdk_screen_get_n_monitors (screen);
	    if ( nmonitor > 1 )
	    {
		g_debug ("Multiple monitor connected");
		return TRUE; 
	    }
	    else
		return FALSE;
	}
    }
    else if ( nscreen > 1 )
    {
	g_debug ("Multiple screen connected");
	return TRUE;
    }
    
    return FALSE;
}

GdkPixbuf *blpm_icon_load (const gchar *icon_name, gint size)
{
    GdkPixbuf *pix = NULL;
    GError *error = NULL;
    
    pix = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), 
				    icon_name, 
				    size,
				    GTK_ICON_LOOKUP_USE_BUILTIN,
				    &error);
				    
    if ( error )
    {
	g_warning ("Unable to load icon : %s : %s", icon_name, error->message);
	g_error_free (error);
    }
    
    return pix;
}

