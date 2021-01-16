/*
 * window-tracker-workspace: A workspace tracked by window tracker.
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

#ifndef __LIBESDASHBOARD_WINDOW_TRACKER_WORKSPACE__
#define __LIBESDASHBOARD_WINDOW_TRACKER_WORKSPACE__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE				(esdashboard_window_tracker_workspace_get_type())
#define ESDASHBOARD_WINDOW_TRACKER_WORKSPACE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE, EsdashboardWindowTrackerWorkspace))
#define ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE))
#define ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE, EsdashboardWindowTrackerWorkspaceInterface))

typedef struct _EsdashboardWindowTrackerWorkspace				EsdashboardWindowTrackerWorkspace;
typedef struct _EsdashboardWindowTrackerWorkspaceInterface		EsdashboardWindowTrackerWorkspaceInterface;

struct _EsdashboardWindowTrackerWorkspaceInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface						parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*is_equal)(EsdashboardWindowTrackerWorkspace *inLeft, EsdashboardWindowTrackerWorkspace *inRight);

	gint (*get_number)(EsdashboardWindowTrackerWorkspace *self);
	const gchar* (*get_name)(EsdashboardWindowTrackerWorkspace *self);

	void (*get_size)(EsdashboardWindowTrackerWorkspace *self, gint *outWidth, gint *outHeight);

	gboolean (*is_active)(EsdashboardWindowTrackerWorkspace *self);
	void (*activate)(EsdashboardWindowTrackerWorkspace *self);

	/* Signals */
	void (*name_changed)(EsdashboardWindowTrackerWorkspace *self);
};

/* Public API */
GType esdashboard_window_tracker_workspace_get_type(void) G_GNUC_CONST;

gboolean esdashboard_window_tracker_workspace_is_equal(EsdashboardWindowTrackerWorkspace *inLeft,
														EsdashboardWindowTrackerWorkspace *inRight);

gint esdashboard_window_tracker_workspace_get_number(EsdashboardWindowTrackerWorkspace *self);
const gchar* esdashboard_window_tracker_workspace_get_name(EsdashboardWindowTrackerWorkspace *self);

void esdashboard_window_tracker_workspace_get_size(EsdashboardWindowTrackerWorkspace *self,
													gint *outWidth,
													gint *outHeight);

gboolean esdashboard_window_tracker_workspace_is_active(EsdashboardWindowTrackerWorkspace *self);
void esdashboard_window_tracker_workspace_activate(EsdashboardWindowTrackerWorkspace *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_WINDOW_TRACKER_WORKSPACE__ */
