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

#ifndef __XFPM_COMMON_H
#define __XFPM_COMMON_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

GdkPixbuf      *blpm_icon_load			(const gchar *icon_name,
						 gint size);

const gchar    *blpm_bool_to_string     	(gboolean value) G_GNUC_PURE;

gboolean        blpm_string_to_bool     	(const gchar *string) G_GNUC_PURE;

GtkBuilder     *blpm_builder_new_from_string   	(const gchar *file,
						 GError **error);

gboolean   	blpm_lock_screen  		(void);

void       	blpm_preferences		(void);

void        blpm_preferences_device_id (const gchar* object_path);

void            blpm_quit                       (void);

void       	blpm_about			(GtkWidget *widget, 
						 gpointer data);

gboolean	blpm_is_multihead_connected	(void);

G_END_DECLS

#endif /* XFPM_COMMON_H */
