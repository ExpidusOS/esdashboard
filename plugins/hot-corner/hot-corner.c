/*
 * hot-corner: Activates application when pointer is move to a corner
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hot-corner.h"

#include <libesdashboard/libesdashboard.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>

#include "hot-corner-settings.h"


/* Define this class in GObject system */
struct _EsdashboardHotCornerPrivate
{
	/* Instance related */
	EsdashboardApplication					*application;
	EsdashboardWindowTracker				*windowTracker;
	GdkWindow								*rootWindow;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat									*seat;
#else
	GdkDeviceManager						*deviceManager;
#endif

	guint									timeoutID;
	GDateTime								*enteredTime;
	gboolean								wasHandledRecently;

	EsdashboardHotCornerSettings			*settings;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(EsdashboardHotCorner,
								esdashboard_hot_corner,
								G_TYPE_OBJECT,
								0,
								G_ADD_PRIVATE_DYNAMIC(EsdashboardHotCorner))

/* Define this class in this plugin */
ESDASHBOARD_DEFINE_PLUGIN_TYPE(esdashboard_hot_corner);

/* IMPLEMENTATION: Enum ESDASHBOARD_TYPE_HOT_CORNER_ACTIVATION_CORNER */

GType esdashboard_hot_corner_activation_corner_get_type(void)
{
	static volatile gsize	g_define_type_id__volatile=0;

	if(g_once_init_enter(&g_define_type_id__volatile))
	{
		static const GEnumValue values[]=
		{
			{ ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_LEFT, "ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_LEFT", "top-left" },
			{ ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_RIGHT, "ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_RIGHT", "top-right" },
			{ ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_LEFT, "ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_LEFT", "bottom-left" },
			{ ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_RIGHT, "ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_RIGHT", "bottom-right" },
			{ 0, NULL, NULL }
		};

		GType	g_define_type_id=g_enum_register_static(g_intern_static_string("EsdashboardHotCornerActivationCorner"), values);
		g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
	}

	return(g_define_type_id__volatile);
}


/* IMPLEMENTATION: Private variables and methods */
#define POLL_POINTER_POSITION_INTERVAL			100

typedef struct _EsdashboardHotCornerBox		EsdashboardHotCornerBox;
struct _EsdashboardHotCornerBox
{
	gint		x1, y1;
	gint		x2, y2;
};

/* Timeout callback to check for activation or suspend via hot corner */
static gboolean _esdashboard_hot_corner_check_hot_corner(gpointer inUserData)
{
	EsdashboardHotCorner							*self;
	EsdashboardHotCornerPrivate						*priv;
	EsdashboardWindowTrackerWindow					*activeWindow;
	GdkDevice										*pointerDevice;
	gint											pointerX, pointerY;
	EsdashboardHotCornerSettingsActivationCorner	activationCorner;
	gint											activationRadius;
	gint64											activationDuration;
	gboolean										primaryMonitorOnly;
	EsdashboardWindowTrackerMonitor					*monitor;
	EsdashboardHotCornerBox							monitorRect;
	EsdashboardHotCornerBox							hotCornerRect;
	GDateTime										*currentTime;
	GTimeSpan										timeDiff;

	g_return_val_if_fail(ESDASHBOARD_IS_HOT_CORNER(inUserData), G_SOURCE_CONTINUE);

	self=ESDASHBOARD_HOT_CORNER(inUserData);
	priv=self->priv;

	/* Get all settings now which are used within this function */
	activationCorner=esdashboard_hot_corner_settings_get_activation_corner(priv->settings);
	activationRadius=esdashboard_hot_corner_settings_get_activation_radius(priv->settings);
	activationDuration=esdashboard_hot_corner_settings_get_activation_duration(priv->settings);
	primaryMonitorOnly=esdashboard_hot_corner_settings_get_primary_monitor_only(priv->settings);

	/* Do nothing if current window is fullscreen but not this application */
	activeWindow=esdashboard_window_tracker_get_active_window(priv->windowTracker);
	if(activeWindow)
	{
		EsdashboardWindowTrackerWindowState			activeWindowState;

		activeWindowState=esdashboard_window_tracker_window_get_state(activeWindow);
		if((activeWindowState & ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_FULLSCREEN) &&
			!esdashboard_window_tracker_window_is_stage(activeWindow))
		{
			return(G_SOURCE_CONTINUE);
		}
	}

	/* Get current position of pointer */
#if GTK_CHECK_VERSION(3, 20, 0)
	pointerDevice=gdk_seat_get_pointer(priv->seat);
#else
	pointerDevice=gdk_device_manager_get_client_pointer(priv->deviceManager);
#endif
	if(!pointerDevice)
	{
		g_critical("Could not get pointer to determine pointer position");
		return(G_SOURCE_CONTINUE);
	}

	gdk_window_get_device_position(priv->rootWindow, pointerDevice, &pointerX, &pointerY, NULL);

	/* Get monitor and its position and size at pointer position */
	monitor=esdashboard_window_tracker_get_monitor_by_position(priv->windowTracker, pointerX, pointerY);
	if(monitor)
	{
		gint										monitorWidth, monitorHeight;

		esdashboard_window_tracker_monitor_get_geometry(monitor,
														&monitorRect.x1,
														&monitorRect.y1,
														&monitorWidth,
														&monitorHeight);
		monitorRect.x2=monitorRect.x1+monitorWidth;
		monitorRect.y2=monitorRect.y1+monitorHeight;
	}
		else
		{
			/* Set position to 0,0 and size to screen size */
			monitorRect.x1=monitorRect.y1=0;
			esdashboard_window_tracker_get_screen_size(priv->windowTracker, &monitorRect.x2, &monitorRect.y2);
		}

	/* Check pointer in currently iterated monitor should be checked */
	if(primaryMonitorOnly &&
		monitor &&
		!esdashboard_window_tracker_monitor_is_primary(monitor))
	{
		return(G_SOURCE_CONTINUE);
	}

	/* Get rectangle where pointer must be inside to activate hot corner */
	switch(activationCorner)
	{
		case ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_RIGHT:
			hotCornerRect.x2=monitorRect.x2;
			hotCornerRect.x1=MAX(monitorRect.x2-activationRadius, monitorRect.x1);
			hotCornerRect.y1=monitorRect.y1;
			hotCornerRect.y2=MIN(monitorRect.y1+activationRadius, monitorRect.y2);
			break;

		case ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_LEFT:
			hotCornerRect.x1=monitorRect.x1;
			hotCornerRect.x2=MIN(monitorRect.x1+activationRadius, monitorRect.x2);
			hotCornerRect.y2=monitorRect.y2;
			hotCornerRect.y1=MAX(monitorRect.y2-activationRadius, monitorRect.y1);
			break;

		case ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_RIGHT:
			hotCornerRect.x2=monitorRect.x2;
			hotCornerRect.x1=MAX(monitorRect.x2-activationRadius, monitorRect.x1);
			hotCornerRect.y2=monitorRect.y2;
			hotCornerRect.y1=MAX(monitorRect.y2-activationRadius, monitorRect.y1);
			break;

		case ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_LEFT:
		default:
			hotCornerRect.x1=monitorRect.x1;
			hotCornerRect.x2=MIN(monitorRect.x1+activationRadius, monitorRect.x2);
			hotCornerRect.y1=monitorRect.y1;
			hotCornerRect.y2=MIN(monitorRect.y1+activationRadius, monitorRect.y2);
			break;
	}

	/* Check if pointer is in configured hot corner for a configured interval.
	 * If it is not reset entered time and return immediately without doing anything.
	 */
	if(pointerX<hotCornerRect.x1 || pointerX>=hotCornerRect.x2 ||
		pointerY<hotCornerRect.y1 || pointerY>=hotCornerRect.y2)
	{
		/* Reset entered time */
		if(priv->enteredTime)
		{
			g_date_time_unref(priv->enteredTime);
			priv->enteredTime=NULL;
		}

		return(G_SOURCE_CONTINUE);
	}

	/* If no entered time was registered yet we assume the pointer is in hot corner
	 * for the first time. So remember entered time for next polling interval.
	 */
	if(!priv->enteredTime)
	{
		/* Remember entered time */
		priv->enteredTime=g_date_time_new_now_local();

		/* Reset handled flag to get duration checked next time */
		priv->wasHandledRecently=FALSE;

		return(G_SOURCE_CONTINUE);
	}

	/* If handled flag is set then do nothing to avoid flapping between activation
	 * and suspending application once the activation duration was reached.
	 */
	if(priv->wasHandledRecently) return(G_SOURCE_CONTINUE);

	/* We know the time the pointer entered hot corner. Check if pointer have stayed
	 * in hot corner for the duration to activate/suspend application. If duration
	 * was not reached yet, just return immediately.
	 */
	currentTime=g_date_time_new_now_local();
	timeDiff=g_date_time_difference(currentTime, priv->enteredTime);
	g_date_time_unref(currentTime);

	if(timeDiff<(activationDuration*G_TIME_SPAN_MILLISECOND)) return(G_SOURCE_CONTINUE);

	/* Activation duration reached so activate application if suspended or suspend it
	 * if active currently.
	 */
	if(!esdashboard_application_is_suspended(priv->application))
	{
		esdashboard_application_suspend_or_quit(priv->application);
	}
		else
		{
			g_application_activate(G_APPLICATION(priv->application));
		}

	/* Set flag that activation was handled recently */
	priv->wasHandledRecently=TRUE;

	return(G_SOURCE_CONTINUE);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_hot_corner_dispose(GObject *inObject)
{
	EsdashboardHotCorner			*self=ESDASHBOARD_HOT_CORNER(inObject);
	EsdashboardHotCornerPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->enteredTime)
	{
		g_date_time_unref(priv->enteredTime);
		priv->enteredTime=NULL;
	}

	if(priv->windowTracker)
	{
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->timeoutID)
	{
		g_source_remove(priv->timeoutID);
		priv->timeoutID=0;
	}

	if(priv->settings)
	{
		g_object_unref(priv->settings);
		priv->settings=NULL;
	}

	if(priv->application)
	{
		priv->application=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_hot_corner_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_hot_corner_class_init(EsdashboardHotCornerClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_hot_corner_dispose;
}

/* Class finalization */
void esdashboard_hot_corner_class_finalize(EsdashboardHotCornerClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_hot_corner_init(EsdashboardHotCorner *self)
{
	EsdashboardHotCornerPrivate		*priv;
	GdkScreen						*screen;
	GdkDisplay						*display;

	self->priv=priv=esdashboard_hot_corner_get_instance_private(self);

	/* Set up default values */
	priv->application=esdashboard_application_get_default();
	priv->windowTracker=esdashboard_window_tracker_get_default();
	priv->rootWindow=NULL;
#if GTK_CHECK_VERSION(3, 20, 0)
	priv->seat=NULL;
#else
	priv->deviceManager=NULL;
#endif

	priv->timeoutID=0;
	priv->enteredTime=NULL;
	priv->wasHandledRecently=FALSE;

	/* Set up settings */
	priv->settings=esdashboard_hot_corner_settings_new();

	/* Get device manager for polling pointer position */
	if(esdashboard_application_is_daemonized(priv->application))
	{
		screen=gdk_screen_get_default();
		priv->rootWindow=gdk_screen_get_root_window(screen);
		if(priv->rootWindow)
		{
			display=gdk_window_get_display(priv->rootWindow);
#if GTK_CHECK_VERSION(3, 20, 0)
			priv->seat=gdk_display_get_default_seat(display);
#else
			priv->deviceManager=gdk_display_get_device_manager(display);
#endif
		}
			else
			{
				g_critical("Disabling hot-corner plugin because the root window to determine pointer position could not be found.");
			}

#if GTK_CHECK_VERSION(3, 20, 0)
		if(priv->seat)
#else
		if(priv->deviceManager)
#endif
		{
			/* Start polling pointer position */
			priv->timeoutID=g_timeout_add(POLL_POINTER_POSITION_INTERVAL,
											(GSourceFunc)_esdashboard_hot_corner_check_hot_corner,
											self);
		}
			else
			{
				g_critical("Disabling hot-corner plugin because the device manager to determine pointer position could not be found.");
			}
	}
		else
		{
			g_warning("Disabling hot-corner plugin because application is not running as daemon.");
		}
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
EsdashboardHotCorner* esdashboard_hot_corner_new(void)
{
	GObject		*hotCorner;

	hotCorner=g_object_new(ESDASHBOARD_TYPE_HOT_CORNER, NULL);
	if(!hotCorner) return(NULL);

	return(ESDASHBOARD_HOT_CORNER(hotCorner));
}
