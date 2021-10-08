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

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <gtk/gtk.h>

#include <libbladeutil/libbladeutil.h>

#include <libnotify/notify.h>

#include "blpm-common.h"
#include "blpm-notify.h"
#include "blpm-dbus-monitor.h"

static void blpm_notify_finalize   (GObject *object);

static NotifyNotification * blpm_notify_new_notification_internal (const gchar *title, 
								   const gchar *message, 
								   const gchar *icon_name, 
								   guint timeout, 
								   XfpmNotifyUrgency urgency, 
								   GtkStatusIcon *icon) G_GNUC_MALLOC;

#define XFPM_NOTIFY_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE((o), XFPM_TYPE_NOTIFY, XfpmNotifyPrivate))

struct XfpmNotifyPrivate
{
    XfpmDBusMonitor    *monitor;
    
    NotifyNotification *notification;
    NotifyNotification *critical;
    
    gulong		critical_id;
    gulong		notify_id;
    
    gboolean	        supports_actions;
    gboolean		supports_sync; /* For x-canonical-private-synchronous */
};

enum
{
    PROP_0,
    PROP_ACTIONS,
    PROP_SYNC
};

G_DEFINE_TYPE(XfpmNotify, blpm_notify, G_TYPE_OBJECT)

static void
blpm_notify_get_server_caps (XfpmNotify *notify)
{
    GList *caps = NULL;
    notify->priv->supports_actions = FALSE;
    notify->priv->supports_sync    = FALSE;
    
    caps = notify_get_server_caps ();
    
    if (caps != NULL) 
    {
	if (g_list_find_custom (caps, "x-canonical-private-synchronous", (GCompareFunc) g_strcmp0) != NULL)
	    notify->priv->supports_sync = TRUE;
    
	if (g_list_find_custom (caps, "actions", (GCompareFunc) g_strcmp0) != NULL)
	    notify->priv->supports_actions = TRUE;

	g_list_foreach(caps, (GFunc)g_free, NULL);
	g_list_free(caps);
    }
}

static void
blpm_notify_check_server (XfpmDBusMonitor *monitor, 
			  gchar *service_name, 
			  gboolean connected,
			  gboolean on_session,
			  XfpmNotify *notify)
{
    if ( !g_strcmp0 (service_name, "org.freedesktop.Notifications") && on_session && connected )
	blpm_notify_get_server_caps (notify);
}

static void blpm_notify_get_property (GObject *object,
				      guint prop_id,
				      GValue *value,
				      GParamSpec *pspec)
{
    XfpmNotify *notify;
    
    notify = XFPM_NOTIFY (object);
    
    switch (prop_id)
    {
	case PROP_ACTIONS:
	    g_value_set_boolean (value, notify->priv->supports_actions);
	    break;
	case PROP_SYNC:
	    g_value_set_boolean (value, notify->priv->supports_sync);
	    break;
	default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
blpm_notify_class_init (XfpmNotifyClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = blpm_notify_finalize;
    object_class->get_property = blpm_notify_get_property;

    g_object_class_install_property (object_class,
                                     PROP_ACTIONS,
                                     g_param_spec_boolean ("actions",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READABLE));

    g_object_class_install_property (object_class,
                                     PROP_SYNC,
                                     g_param_spec_boolean ("sync",
                                                           NULL, NULL,
                                                           FALSE,
                                                           G_PARAM_READABLE));

    g_type_class_add_private (klass, sizeof (XfpmNotifyPrivate));
}

static void
blpm_notify_init (XfpmNotify *notify)
{
    notify->priv = XFPM_NOTIFY_GET_PRIVATE (notify);
    
    notify->priv->notification = NULL;
    notify->priv->critical = NULL;
    
    notify->priv->critical_id = 0;
    notify->priv->notify_id   = 0;
    
    notify->priv->monitor = blpm_dbus_monitor_new ();
    blpm_dbus_monitor_add_service (notify->priv->monitor, DBUS_BUS_SESSION, "org.freedesktop.Notifications");
    g_signal_connect (notify->priv->monitor, "service-connection-changed",
		      G_CALLBACK (blpm_notify_check_server), notify);
    
    blpm_notify_get_server_caps (notify);
}

static void
blpm_notify_finalize (GObject *object)
{
    XfpmNotify *notify;

    notify = XFPM_NOTIFY (object);
    
    blpm_notify_close_normal (notify);
    blpm_notify_close_critical (notify);
    
    G_OBJECT_CLASS (blpm_notify_parent_class)->finalize(object);
}

static void
blpm_notify_set_notification_icon (NotifyNotification *n, const gchar *icon_name )
{
    GdkPixbuf *pix = blpm_icon_load (icon_name, 48);
    
    if ( pix )
    {
	notify_notification_set_icon_from_pixbuf (n,
						  pix);
	g_object_unref ( G_OBJECT(pix));
    }
    
}

static NotifyNotification *
blpm_notify_new_notification_internal (const gchar *title, const gchar *message,
				       const gchar *icon_name, guint timeout,
				       XfpmNotifyUrgency urgency, GtkStatusIcon *icon)
{
    NotifyNotification *n;
    
#ifdef NOTIFY_CHECK_VERSION
#if NOTIFY_CHECK_VERSION (0, 7, 0) 
    n = notify_notification_new (title, message, NULL);
#else
    n = notify_notification_new (title, message, NULL, NULL);
#endif
#else
    n = notify_notification_new (title, message, NULL, NULL);
#endif

    
    if ( icon_name )
    	blpm_notify_set_notification_icon (n, icon_name);

#ifdef NOTIFY_CHECK_VERSION
#if !NOTIFY_CHECK_VERSION (0, 7, 0) 
    if ( icon )
    	notify_notification_attach_to_status_icon (n, icon);
#endif
#else
     if ( icon )
     	notify_notification_attach_to_status_icon (n, icon);
#endif
	
    notify_notification_set_urgency (n, (NotifyUrgency)urgency);
    
    if ( timeout != 0)
	notify_notification_set_timeout (n, timeout);
    
    return n;
}

static void
blpm_notify_closed_cb (NotifyNotification *n, XfpmNotify *notify)
{
    notify->priv->notification = NULL;
    g_object_unref (G_OBJECT (n));
}

static void
blpm_notify_close_critical_cb (NotifyNotification *n, XfpmNotify *notify)
{
    notify->priv->critical = NULL;
    g_object_unref (G_OBJECT (n));
}

static gboolean
blpm_notify_show (NotifyNotification *n)
{
    notify_notification_show (n, NULL);
    return FALSE;
}

static void
blpm_notify_close_notification (XfpmNotify *notify )
{
    if (notify->priv->notify_id != 0)
    {
	g_source_remove (notify->priv->notify_id);
	notify->priv->notify_id = 0;
    }
    
    if ( notify->priv->notification )
    {
    	if (!notify_notification_close (notify->priv->notification, NULL))
	    g_warning ("Failed to close notification\n");
	
	g_object_unref (G_OBJECT(notify->priv->notification) );
	notify->priv->notification  = NULL;
    }
}

XfpmNotify *
blpm_notify_new (void)
{
    static gpointer blpm_notify_object = NULL;
    
    if ( blpm_notify_object != NULL )
    {
	g_object_ref (blpm_notify_object);
    }
    else
    {
	blpm_notify_object = g_object_new (XFPM_TYPE_NOTIFY, NULL);
	g_object_add_weak_pointer (blpm_notify_object, &blpm_notify_object);
    }
    return XFPM_NOTIFY (blpm_notify_object);
}

void blpm_notify_show_notification (XfpmNotify *notify, const gchar *title,
				    const gchar *text,  const gchar *icon_name,
				    gint timeout, gboolean simple,
				    XfpmNotifyUrgency urgency, GtkStatusIcon *icon)
{
    NotifyNotification *n;
    
    if ( !simple )
        blpm_notify_close_notification (notify);
    
    n = blpm_notify_new_notification_internal (title, 
				               text, icon_name, 
					       timeout, urgency, 
					       icon);
					       
    blpm_notify_present_notification (notify, n, simple);
}

NotifyNotification *blpm_notify_new_notification (XfpmNotify *notify,
						  const gchar *title,
						  const gchar *text,
						  const gchar *icon_name,
						  guint timeout,
						  XfpmNotifyUrgency urgency,
						  GtkStatusIcon *icon)
{
    NotifyNotification *n = blpm_notify_new_notification_internal (title, 
							           text, icon_name, 
								   timeout, urgency, 
								   icon);
    return n;
}

void blpm_notify_add_action_to_notification (XfpmNotify *notify, NotifyNotification *n,
					    const gchar *id, const gchar *action_label,
					    NotifyActionCallback callback, gpointer data)
{
    g_return_if_fail (XFPM_IS_NOTIFY(notify));
    
    notify_notification_add_action (n, id, action_label,
				   (NotifyActionCallback)callback,
				    data, NULL);
    
}

void blpm_notify_present_notification (XfpmNotify *notify, NotifyNotification *n, gboolean simple)
{
    g_return_if_fail (XFPM_IS_NOTIFY(notify));
    
    if ( !simple )
        blpm_notify_close_notification (notify);
    
    if ( !simple )
    {
	g_signal_connect (G_OBJECT(n),"closed",
			G_CALLBACK(blpm_notify_closed_cb), notify);
	notify->priv->notification = n;
    }
    
    notify->priv->notify_id = g_idle_add ((GSourceFunc) blpm_notify_show, n);
}

void blpm_notify_critical (XfpmNotify *notify, NotifyNotification *n)
{
    g_return_if_fail (XFPM_IS_NOTIFY (notify));

    blpm_notify_close_critical (notify);
    
    notify->priv->critical = n;
    
    g_signal_connect (G_OBJECT (n), "closed", 
		      G_CALLBACK (blpm_notify_close_critical_cb), notify);
		      
    notify->priv->critical_id = g_idle_add ((GSourceFunc) blpm_notify_show, n);
}

void blpm_notify_close_critical (XfpmNotify *notify)
{
    g_return_if_fail (XFPM_IS_NOTIFY (notify));
    
    
    if (notify->priv->critical_id != 0)
    {
	g_source_remove (notify->priv->critical_id);
	notify->priv->critical_id = 0;
    }
    
    if ( notify->priv->critical )
    {
    	if (!notify_notification_close (notify->priv->critical, NULL))
	    g_warning ("Failed to close notification\n");
	
	g_object_unref (G_OBJECT(notify->priv->critical) );
	notify->priv->critical  = NULL;
    }
}

void blpm_notify_close_normal  (XfpmNotify *notify)
{
    g_return_if_fail (XFPM_IS_NOTIFY (notify));
    
    blpm_notify_close_notification (notify);
}
