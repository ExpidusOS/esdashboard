/*
 * window-tracker: Tracks windows, workspaces, monitors and
 *                 listens for changes. It also bundles libwnck into one
 *                 class.
 *                 By wrapping libwnck objects we can use a virtual
 *                 stable API while the API in libwnck changes within versions.
 *                 We only need to use #ifdefs in window tracker object
 *                 and nowhere else in the code.
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

#ifndef __LIBESDASHBOARD_WINDOW_TRACKER_X11__
#define __LIBESDASHBOARD_WINDOW_TRACKER_X11__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libesdashboard/window-tracker-window.h>
#include <libesdashboard/window-tracker-workspace.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_WINDOW_TRACKER_X11				(esdashboard_window_tracker_x11_get_type())
#define ESDASHBOARD_WINDOW_TRACKER_X11(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_X11, EsdashboardWindowTrackerX11))
#define ESDASHBOARD_IS_WINDOW_TRACKER_X11(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_X11))
#define ESDASHBOARD_WINDOW_TRACKER_X11_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_WINDOW_TRACKER_X11, EsdashboardWindowTrackerX11Class))
#define ESDASHBOARD_IS_WINDOW_TRACKER_X11_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_WINDOW_TRACKER_X11))
#define ESDASHBOARD_WINDOW_TRACKER_X11_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_X11, EsdashboardWindowTrackerX11Class))

typedef struct _EsdashboardWindowTrackerX11				EsdashboardWindowTrackerX11;
typedef struct _EsdashboardWindowTrackerX11Class		EsdashboardWindowTrackerX11Class;
typedef struct _EsdashboardWindowTrackerX11Private		EsdashboardWindowTrackerX11Private;

struct _EsdashboardWindowTrackerX11
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	EsdashboardWindowTrackerX11Private		*priv;
};

struct _EsdashboardWindowTrackerX11Class
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_window_tracker_x11_get_type(void) G_GNUC_CONST;

guint32 esdashboard_window_tracker_x11_get_time(void);

EsdashboardWindowTrackerWindow* esdashboard_window_tracker_x11_get_window_for_wnck(EsdashboardWindowTrackerX11 *self,
																					WnckWindow *inWindow);
EsdashboardWindowTrackerWorkspace* esdashboard_window_tracker_x11_get_workspace_for_wnck(EsdashboardWindowTrackerX11 *self,
																							WnckWorkspace *inWorkspace);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_WINDOW_TRACKER_X11__ */
