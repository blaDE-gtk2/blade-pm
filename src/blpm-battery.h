/*
 * * Copyright (C) 2009-2011 Ali <aliov@xfce.org>
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

#ifndef __XFPM_BATTERY_H
#define __XFPM_BATTERY_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <upower.h>

#include "blpm-enum-glib.h"

G_BEGIN_DECLS

#define XFPM_TYPE_BATTERY        (blpm_battery_get_type () )
#define XFPM_BATTERY(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), XFPM_TYPE_BATTERY, XfpmBattery))
#define XFPM_IS_BATTERY(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFPM_TYPE_BATTERY))

typedef struct XfpmBatteryPrivate XfpmBatteryPrivate;

typedef struct
{
    GtkStatusIcon      	    parent;
    
    XfpmBatteryPrivate     *priv;
    
} XfpmBattery;

typedef struct
{
    GtkStatusIconClass 	    parent_class;
    
    void		    (*battery_charge_changed)	 (XfpmBattery *battery);
    
} XfpmBatteryClass;

GType        		    blpm_battery_get_type        (void) G_GNUC_CONST;

GtkStatusIcon              *blpm_battery_new             (void);

void			    blpm_battery_monitor_device  (XfpmBattery *battery,
							  const char *object_path,
							  UpDeviceKind device_type);

UpDeviceKind		    blpm_battery_get_device_type (XfpmBattery *battery);

XfpmBatteryCharge	    blpm_battery_get_charge      (XfpmBattery *battery);

const gchar 		   *blpm_battery_get_battery_name(XfpmBattery *battery);

gchar 			   *blpm_battery_get_time_left   (XfpmBattery *battery);

G_END_DECLS

#endif /* __XFPM_BATTERY_H */
