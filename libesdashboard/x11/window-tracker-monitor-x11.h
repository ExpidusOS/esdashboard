/*
 * window-tracker-monitor: A monitor object tracked by window tracker.
 *                         It provides information about position and
 *                         size of monitor within screen and also a flag
 *                         if this monitor is the primary one.
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

#ifndef __LIBESDASHBOARD_WINDOW_TRACKER_MONITOR_X11__
#define __LIBESDASHBOARD_WINDOW_TRACKER_MONITOR_X11__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_X11					(esdashboard_window_tracker_monitor_x11_get_type())
#define ESDASHBOARD_WINDOW_TRACKER_MONITOR_X11(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_X11, EsdashboardWindowTrackerMonitorX11))
#define ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_X11))
#define ESDASHBOARD_WINDOW_TRACKER_MONITOR_X11_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_X11, EsdashboardWindowTrackerMonitorX11Class))
#define ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_X11))
#define ESDASHBOARD_WINDOW_TRACKER_MONITOR_X11_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_X11, EsdashboardWindowTrackerMonitorX11Class))

typedef struct _EsdashboardWindowTrackerMonitorX11					EsdashboardWindowTrackerMonitorX11;
typedef struct _EsdashboardWindowTrackerMonitorX11Class				EsdashboardWindowTrackerMonitorX11Class;
typedef struct _EsdashboardWindowTrackerMonitorX11Private			EsdashboardWindowTrackerMonitorX11Private;

struct _EsdashboardWindowTrackerMonitorX11
{
	/*< private >*/
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	EsdashboardWindowTrackerMonitorX11Private		*priv;
};

struct _EsdashboardWindowTrackerMonitorX11Class
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*primary_changed)(EsdashboardWindowTrackerMonitorX11 *self);
	void (*geometry_changed)(EsdashboardWindowTrackerMonitorX11 *self);
};

/* Public API */
GType esdashboard_window_tracker_monitor_x11_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif	/* __LIBESDASHBOARD_WINDOW_TRACKER_MONITOR_X11__ */
