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
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <gtk/gtk.h>
#include <libbladeutil/libbladeutil.h>

#include "blpm-backlight.h"
#include "egg-idletime.h"
#include "blpm-notify.h"
#include "blpm-blconf.h"
#include "blpm-power.h"
#include "blpm-config.h"
#include "blpm-button.h"
#include "blpm-brightness.h"
#include "blpm-debug.h"
#include "blpm-icons.h"

static void blpm_backlight_finalize     (GObject *object);

static void blpm_backlight_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);

static void blpm_backlight_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);

#define ALARM_DISABLED 9

#define XFPM_BACKLIGHT_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), XFPM_TYPE_BACKLIGHT, XfpmBacklightPrivate))

struct XfpmBacklightPrivate
{
    XfpmBrightness *brightness;
    XfpmPower      *power;
    EggIdletime    *idle;
    XfpmBlconf     *conf;
    XfpmButton     *button;
    XfpmNotify     *notify;
    
    NotifyNotification *n;
    
    gboolean	    has_hw;
    gboolean	    on_battery;
    
    gint32          last_level;
    gint32 	    max_level;
    
    gint            brightness_switch;
    gint            brightness_switch_save;
    gboolean        brightness_switch_initialized;

    gboolean        dimmed;
    gboolean	    block;
};

enum
{
    PROP_0,
    PROP_BRIGHTNESS_SWITCH,
    PROP_BRIGHTNESS_SWITCH_SAVE,
    N_PROPERTIES
};

G_DEFINE_TYPE (XfpmBacklight, blpm_backlight, G_TYPE_OBJECT)


static void
blpm_backlight_dim_brightness (XfpmBacklight *backlight)
{
    gboolean ret;
    
    if (blpm_power_is_in_presentation_mode (backlight->priv->power) == FALSE )
    {
	gint32 dim_level;
	
	g_object_get (G_OBJECT (backlight->priv->conf),
		      backlight->priv->on_battery ? BRIGHTNESS_LEVEL_ON_BATTERY : BRIGHTNESS_LEVEL_ON_AC, &dim_level,
		      NULL);
	
	ret = blpm_brightness_get_level (backlight->priv->brightness, &backlight->priv->last_level);
	
	if ( !ret )
	{
	    g_warning ("Unable to get current brightness level");
	    return;
	}
	
	dim_level = dim_level * backlight->priv->max_level / 100;
	
	/**
	 * Only reduce if the current level is brighter than
	 * the configured dim_level
	 **/
	if (backlight->priv->last_level > dim_level)
	{
	    XFPM_DEBUG ("Current brightness level before dimming : %d, new %d", backlight->priv->last_level, dim_level);
	    backlight->priv->dimmed = blpm_brightness_set_level (backlight->priv->brightness, dim_level);
	}
    }
}

static gboolean
blpm_backlight_destroy_popup (gpointer data)
{
    XfpmBacklight *backlight;
    
    backlight = XFPM_BACKLIGHT (data);
    
    if ( backlight->priv->n )
    {
	g_object_unref (backlight->priv->n);
	backlight->priv->n = NULL;
    }
    
    return FALSE;
}

static void
blpm_backlight_show_notification (XfpmBacklight *backlight, gfloat value)
{
    gchar *summary;

    /* create the notification on demand */
    if ( backlight->priv->n == NULL )
    {
	backlight->priv->n = blpm_notify_new_notification (backlight->priv->notify,
							   "",
							   "",
							   "blpm-brightness-lcd",
							   0,
							   XFPM_NOTIFY_NORMAL,
							   NULL);
    }

    /* generate a human-readable summary for the notification */
    summary = g_strdup_printf (_("Brightness: %.0f percent"), value);
    notify_notification_update (backlight->priv->n, summary, NULL, NULL);
    g_free (summary);
    
    /* add the brightness value to the notification */
    notify_notification_set_hint_int32 (backlight->priv->n, "value", value);
    
    /* show the notification */
    notify_notification_show (backlight->priv->n, NULL);
}

static void
blpm_backlight_show (XfpmBacklight *backlight, gint level)
{
    gfloat value;
    
    XFPM_DEBUG ("Level %u", level);
    
    value = (gfloat) 100 * level / backlight->priv->max_level;
    blpm_backlight_show_notification (backlight, value);
}


static void
blpm_backlight_alarm_timeout_cb (EggIdletime *idle, guint id, XfpmBacklight *backlight)
{
    backlight->priv->block = FALSE;
    
    if ( id == TIMEOUT_BRIGHTNESS_ON_AC && !backlight->priv->on_battery)
	blpm_backlight_dim_brightness (backlight);
    else if ( id == TIMEOUT_BRIGHTNESS_ON_BATTERY && backlight->priv->on_battery)
	blpm_backlight_dim_brightness (backlight);
}

static void
blpm_backlight_reset_cb (EggIdletime *idle, XfpmBacklight *backlight)
{
    if ( backlight->priv->dimmed)
    {
	if ( !backlight->priv->block)
	{
	    XFPM_DEBUG ("Alarm reset, setting level to %d", backlight->priv->last_level);
	    blpm_brightness_set_level (backlight->priv->brightness, backlight->priv->last_level);
	}
	backlight->priv->dimmed = FALSE;
    }
}

static void
blpm_backlight_button_pressed_cb (XfpmButton *button, XfpmButtonKey type, XfpmBacklight *backlight)
{
    gint32 level;
    gboolean ret = TRUE;
    
    gboolean handle_brightness_keys, show_popup;
    
    g_object_get (G_OBJECT (backlight->priv->conf),
                  HANDLE_BRIGHTNESS_KEYS, &handle_brightness_keys,
                  SHOW_BRIGHTNESS_POPUP, &show_popup,
                  NULL);
    
    if ( type != BUTTON_MON_BRIGHTNESS_UP && type != BUTTON_MON_BRIGHTNESS_DOWN )
	return; /* sanity check, can this ever happen? */

    backlight->priv->block = TRUE;
    if ( !handle_brightness_keys )
        ret = blpm_brightness_get_level (backlight->priv->brightness, &level);
    else
    {
	if ( type == BUTTON_MON_BRIGHTNESS_UP )
	    ret = blpm_brightness_up (backlight->priv->brightness, &level);
	else
	    ret = blpm_brightness_down (backlight->priv->brightness, &level);
    }
    if ( ret && show_popup)
	blpm_backlight_show (backlight, level);
}

static void
blpm_backlight_brightness_on_ac_settings_changed (XfpmBacklight *backlight)
{
    guint timeout_on_ac;
    
    g_object_get (G_OBJECT (backlight->priv->conf),
		  BRIGHTNESS_ON_AC, &timeout_on_ac,
		  NULL);
		  
    XFPM_DEBUG ("Alarm on ac timeout changed %u", timeout_on_ac);
    
    if ( timeout_on_ac == ALARM_DISABLED )
    {
	egg_idletime_alarm_remove (backlight->priv->idle, TIMEOUT_BRIGHTNESS_ON_AC );
    }
    else
    {
	egg_idletime_alarm_set (backlight->priv->idle, TIMEOUT_BRIGHTNESS_ON_AC, timeout_on_ac * 1000);
    }
}

static void
blpm_backlight_brightness_on_battery_settings_changed (XfpmBacklight *backlight)
{
    guint timeout_on_battery ;
    
    g_object_get (G_OBJECT (backlight->priv->conf),
		  BRIGHTNESS_ON_BATTERY, &timeout_on_battery,
		  NULL);
    
    XFPM_DEBUG ("Alarm on battery timeout changed %u", timeout_on_battery);
    
    if ( timeout_on_battery == ALARM_DISABLED )
    {
	egg_idletime_alarm_remove (backlight->priv->idle, TIMEOUT_BRIGHTNESS_ON_BATTERY );
    }
    else
    {
	egg_idletime_alarm_set (backlight->priv->idle, TIMEOUT_BRIGHTNESS_ON_BATTERY, timeout_on_battery * 1000);
    } 
}


static void
blpm_backlight_set_timeouts (XfpmBacklight *backlight)
{
    blpm_backlight_brightness_on_ac_settings_changed (backlight);
    blpm_backlight_brightness_on_battery_settings_changed (backlight);
}

static void
blpm_backlight_on_battery_changed_cb (XfpmPower *power, gboolean on_battery, XfpmBacklight *backlight)
{
    backlight->priv->on_battery = on_battery;
}

static void
blpm_backlight_class_init (XfpmBacklightClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->get_property = blpm_backlight_get_property;
    object_class->set_property = blpm_backlight_set_property;
    object_class->finalize = blpm_backlight_finalize;

    g_object_class_install_property (object_class,
                                     PROP_BRIGHTNESS_SWITCH,
                                     g_param_spec_int (BRIGHTNESS_SWITCH,
                                                       NULL, NULL,
                                                       -1,
                                                       1,
                                                       -1,
                                                       G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_BRIGHTNESS_SWITCH_SAVE,
                                     g_param_spec_int (BRIGHTNESS_SWITCH_SAVE,
                                                       NULL, NULL,
                                                       -1,
                                                       1,
                                                       -1,
                                                       G_PARAM_READWRITE));

    g_type_class_add_private (klass, sizeof (XfpmBacklightPrivate));
}

static void
blpm_backlight_init (XfpmBacklight *backlight)
{
    backlight->priv = XFPM_BACKLIGHT_GET_PRIVATE (backlight);
    
    backlight->priv->brightness = blpm_brightness_new ();
    backlight->priv->has_hw     = blpm_brightness_setup (backlight->priv->brightness);
    
    backlight->priv->notify = NULL;
    backlight->priv->idle   = NULL;
    backlight->priv->conf   = NULL;
    backlight->priv->button = NULL;
    backlight->priv->power    = NULL;
    backlight->priv->dimmed = FALSE;
    backlight->priv->block = FALSE;
    backlight->priv->brightness_switch_initialized = FALSE;
    
    if ( !backlight->priv->has_hw )
    {
	g_object_unref (backlight->priv->brightness);
	backlight->priv->brightness = NULL;
    }
    else
    {
	gboolean ret, handle_keys;

	backlight->priv->idle   = egg_idletime_new ();
	backlight->priv->conf   = blpm_blconf_new ();
	backlight->priv->button = blpm_button_new ();
	backlight->priv->power    = blpm_power_get ();
	backlight->priv->notify = blpm_notify_new ();
	backlight->priv->max_level = blpm_brightness_get_max_level (backlight->priv->brightness);
	backlight->priv->brightness_switch = -1;

	blconf_g_property_bind (blpm_blconf_get_channel(backlight->priv->conf),
							PROPERTIES_PREFIX BRIGHTNESS_SWITCH, G_TYPE_INT,
							G_OBJECT(backlight), BRIGHTNESS_SWITCH);

	ret = blpm_brightness_get_switch (backlight->priv->brightness,
									  &backlight->priv->brightness_switch);

	if (ret)
	g_object_set (G_OBJECT (backlight),
				  BRIGHTNESS_SWITCH,
				  backlight->priv->brightness_switch,
				  NULL);
	backlight->priv->brightness_switch_initialized = TRUE;

	/*
	 * If power manager has crashed last time, the brightness switch
	 * saved value will be set to the original value. In that case, we
	 * will use this saved value instead of the one found at the
	 * current startup so the setting is restored properly.
	 */
	backlight->priv->brightness_switch_save =
		blconf_channel_get_int (blpm_blconf_get_channel(backlight->priv->conf),
								PROPERTIES_PREFIX BRIGHTNESS_SWITCH_SAVE,
								-1);

	if (backlight->priv->brightness_switch_save == -1)
	{
	if (!blconf_channel_set_int (blpm_blconf_get_channel(backlight->priv->conf),
								 PROPERTIES_PREFIX BRIGHTNESS_SWITCH_SAVE,
								 backlight->priv->brightness_switch))
	g_critical ("Cannot set value for property %s\n", BRIGHTNESS_SWITCH_SAVE);

	backlight->priv->brightness_switch_save = backlight->priv->brightness_switch;
	}
	else
	{
	g_warning ("It seems the kernel brightness switch handling value was "
			   "not restored properly on exit last time, blade-pm "
			   "will try to restore it this time.");
	}

    /* check whether to change the brightness switch */
	handle_keys = blconf_channel_get_bool (blpm_blconf_get_channel(backlight->priv->conf),
										   PROPERTIES_PREFIX HANDLE_BRIGHTNESS_KEYS,
										   TRUE);
	backlight->priv->brightness_switch = handle_keys ? 0 : 1;
	g_object_set (G_OBJECT (backlight),
				  BRIGHTNESS_SWITCH,
				  backlight->priv->brightness_switch,
				  NULL);

	g_signal_connect (backlight->priv->idle, "alarm-expired",
                          G_CALLBACK (blpm_backlight_alarm_timeout_cb), backlight);
        
        g_signal_connect (backlight->priv->idle, "reset",
                          G_CALLBACK(blpm_backlight_reset_cb), backlight);
			  
	g_signal_connect (backlight->priv->button, "button-pressed",
		          G_CALLBACK (blpm_backlight_button_pressed_cb), backlight);
			  
	g_signal_connect_swapped (backlight->priv->conf, "notify::" BRIGHTNESS_ON_AC,
				  G_CALLBACK (blpm_backlight_brightness_on_ac_settings_changed), backlight);
	
	g_signal_connect_swapped (backlight->priv->conf, "notify::" BRIGHTNESS_ON_BATTERY,
				  G_CALLBACK (blpm_backlight_brightness_on_battery_settings_changed), backlight);
				
	g_signal_connect (backlight->priv->power, "on-battery-changed",
			  G_CALLBACK (blpm_backlight_on_battery_changed_cb), backlight);
	g_object_get (G_OBJECT (backlight->priv->power),
		      "on-battery", &backlight->priv->on_battery,
		      NULL);
	blpm_brightness_get_level (backlight->priv->brightness, &backlight->priv->last_level);
	blpm_backlight_set_timeouts (backlight);
    }
}

static void
blpm_backlight_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
    XfpmBacklight *backlight = XFPM_BACKLIGHT (object);

    switch (prop_id)
    {
    case PROP_BRIGHTNESS_SWITCH:
        g_value_set_int (value, backlight->priv->brightness_switch);
        break;
    case PROP_BRIGHTNESS_SWITCH_SAVE:
        g_value_set_int (value, backlight->priv->brightness_switch_save);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
blpm_backlight_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
    XfpmBacklight *backlight = XFPM_BACKLIGHT (object);
    gboolean ret;

    switch (prop_id)
    {
	case PROP_BRIGHTNESS_SWITCH:
        backlight->priv->brightness_switch = g_value_get_int (value);
        if (!backlight->priv->brightness_switch_initialized)
            break;
        ret = blpm_brightness_set_switch (backlight->priv->brightness,
                                          backlight->priv->brightness_switch);
        if (!ret)
            g_warning ("Unable to set the kernel brightness switch parameter to %d.",
                       backlight->priv->brightness_switch);
        else
            g_message ("Set kernel brightness switch to %d",
                       backlight->priv->brightness_switch);

	    break;
	case PROP_BRIGHTNESS_SWITCH_SAVE:
        backlight->priv->brightness_switch_save = g_value_get_int (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
blpm_backlight_finalize (GObject *object)
{
    XfpmBacklight *backlight;

    backlight = XFPM_BACKLIGHT (object);

    blpm_backlight_destroy_popup (backlight);

    if ( backlight->priv->idle )
	g_object_unref (backlight->priv->idle);

    if ( backlight->priv->conf )
    {
    /* restore video module brightness switch setting */
    if ( backlight->priv->brightness_switch_save != -1 )
    {
    gboolean ret =
        blpm_brightness_set_switch (backlight->priv->brightness,
                                    backlight->priv->brightness_switch_save);
    /* unset the blconf saved value after the restore */
    if (!blconf_channel_set_int (blpm_blconf_get_channel(backlight->priv->conf),
                                 PROPERTIES_PREFIX BRIGHTNESS_SWITCH_SAVE, -1))
    g_critical ("Cannot set value for property %s\n", BRIGHTNESS_SWITCH_SAVE);

    if (ret)
    {
    backlight->priv->brightness_switch = backlight->priv->brightness_switch_save;
    g_message ("Restored brightness switch value to: %d", backlight->priv->brightness_switch);
    }
    else
    g_warning ("Unable to restore the kernel brightness switch parameter to its original value, "
               "still resetting the saved value.");
    }

    g_object_unref (backlight->priv->conf);
    }

    if ( backlight->priv->brightness )
	g_object_unref (backlight->priv->brightness);

    if ( backlight->priv->button )
	g_object_unref (backlight->priv->button);

    if ( backlight->priv->power )
	g_object_unref (backlight->priv->power);

    if ( backlight->priv->notify)
	g_object_unref (backlight->priv->notify);

    G_OBJECT_CLASS (blpm_backlight_parent_class)->finalize (object);
}

XfpmBacklight *
blpm_backlight_new (void)
{
    XfpmBacklight *backlight = NULL;
    backlight = g_object_new (XFPM_TYPE_BACKLIGHT, NULL);
    return backlight;
}

gboolean blpm_backlight_has_hw (XfpmBacklight *backlight)
{
    return backlight->priv->has_hw;
}
