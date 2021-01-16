/*
 * window-tracker-monitor: A monitor object tracked by window tracker.
 *                         It provides information about position and
 *                         size of monitor within screen and also a flag
 *                         if this monitor is the primary one.
 * 
 * Copyright 2012-2016 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBESDASHBOARD_WINDOW_TRACKER_MONITOR__
#define __LIBESDASHBOARD_WINDOW_TRACKER_MONITOR__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR					(esdashboard_window_tracker_monitor_get_type())
#define ESDASHBOARD_WINDOW_TRACKER_MONITOR(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR, EsdashboardWindowTrackerMonitor))
#define ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR))
#define ESDASHBOARD_WINDOW_TRACKER_MONITOR_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR, EsdashboardWindowTrackerMonitorInterface))

typedef struct _EsdashboardWindowTrackerMonitor					EsdashboardWindowTrackerMonitor;
typedef struct _EsdashboardWindowTrackerMonitorInterface		EsdashboardWindowTrackerMonitorInterface;

struct _EsdashboardWindowTrackerMonitorInterface
{
	/*< private >*/
	/* Parent class */
	GTypeInterface						parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*is_equal)(EsdashboardWindowTrackerMonitor *inLeft, EsdashboardWindowTrackerMonitor *inRight);

	gboolean (*is_primary)(EsdashboardWindowTrackerMonitor *self);
	gint (*get_number)(EsdashboardWindowTrackerMonitor *self);

	void (*get_geometry)(EsdashboardWindowTrackerMonitor *self, gint *outX, gint *outY, gint *outWidth, gint *outHeight);

	/* Signals */
	void (*primary_changed)(EsdashboardWindowTrackerMonitor *self);
	void (*geometry_changed)(EsdashboardWindowTrackerMonitor *self);
};

/* Public API */
GType esdashboard_window_tracker_monitor_get_type(void) G_GNUC_CONST;

gboolean esdashboard_window_tracker_monitor_is_equal(EsdashboardWindowTrackerMonitor *inLeft,
														EsdashboardWindowTrackerMonitor *inRight);

gint esdashboard_window_tracker_monitor_get_number(EsdashboardWindowTrackerMonitor *self);

gboolean esdashboard_window_tracker_monitor_is_primary(EsdashboardWindowTrackerMonitor *self);

void esdashboard_window_tracker_monitor_get_geometry(EsdashboardWindowTrackerMonitor *self,
														gint *outX,
														gint *outY,
														gint *outWidth,
														gint *outHeight);
gboolean esdashboard_window_tracker_monitor_contains(EsdashboardWindowTrackerMonitor *self,
														gint inX,
														gint inY);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_WINDOW_TRACKER_MONITOR__ */
