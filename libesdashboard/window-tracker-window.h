/*
 * window-tracker-window: A window tracked by window tracker.
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

#ifndef __LIBESDASHBOARD_WINDOW_TRACKER_WINDOW__
#define __LIBESDASHBOARD_WINDOW_TRACKER_WINDOW__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>
#include <glib-object.h>
#include <gdk/gdk.h>

#include <libesdashboard/window-tracker-workspace.h>
#include <libesdashboard/window-tracker-monitor.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * EsdashboardWindowTrackerWindowState:
 * @ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN: The window is not visible on its
 *                                                  #EsdashboardWindowTrackerWorkspace,
 *                                                  e.g. when minimized.
 * @ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED: The window is minimized.
 * @ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MAXIMIZED: The window is maximized.
 * @ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_FULLSCREEN: The window is fullscreen.
 * @ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER: The window should not be included on pagers.
 * @ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST: The window should not be included on tasklists.
 * @ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED: The window is on all workspaces.
 * @ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT: The window requires a response from the user.
 *
 * Type used as a bitmask to describe the state of a #EsdashboardWindowTrackerWindow.
 */
typedef enum /*< flags,prefix=ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE >*/
{
	ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN=1 << 0,
	ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED=1 << 1,
	ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MAXIMIZED=1 << 2,
	ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_FULLSCREEN=1 << 3,
	ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER=1 << 4,
	ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST=1 << 5,
	ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED=1 << 6,
	ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT=1 << 7,
} EsdashboardWindowTrackerWindowState;

/**
 * EsdashboardWindowTrackerWindowAction:
 * @ESDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION_CLOSE: The window may be closed.
 *
 * Type used as a bitmask to describe the actions that can be done for a #EsdashboardWindowTrackerWindow.
 */
typedef enum /*< flags,prefix=ESDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION >*/
{
	ESDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION_CLOSE=1 << 0,
} EsdashboardWindowTrackerWindowAction;


/* Object declaration */
#define ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW				(esdashboard_window_tracker_window_get_type())
#define ESDASHBOARD_WINDOW_TRACKER_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, EsdashboardWindowTrackerWindow))
#define ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW))
#define ESDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, EsdashboardWindowTrackerWindowInterface))

typedef struct _EsdashboardWindowTrackerWindow				EsdashboardWindowTrackerWindow;
typedef struct _EsdashboardWindowTrackerWindowInterface		EsdashboardWindowTrackerWindowInterface;

struct _EsdashboardWindowTrackerWindowInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface						parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*is_equal)(EsdashboardWindowTrackerWindow *inLeft, EsdashboardWindowTrackerWindow *inRight);

	gboolean (*is_visible)(EsdashboardWindowTrackerWindow *self);
	void (*show)(EsdashboardWindowTrackerWindow *self);
	void (*hide)(EsdashboardWindowTrackerWindow *self);

	EsdashboardWindowTrackerWindow* (*get_parent)(EsdashboardWindowTrackerWindow *self);

	EsdashboardWindowTrackerWindowState (*get_state)(EsdashboardWindowTrackerWindow *self);
	EsdashboardWindowTrackerWindowAction (*get_actions)(EsdashboardWindowTrackerWindow *self);

	const gchar* (*get_name)(EsdashboardWindowTrackerWindow *self);

	GdkPixbuf* (*get_icon)(EsdashboardWindowTrackerWindow *self);
	const gchar* (*get_icon_name)(EsdashboardWindowTrackerWindow *self);

	EsdashboardWindowTrackerWorkspace* (*get_workspace)(EsdashboardWindowTrackerWindow *self);
	gboolean (*is_on_workspace)(EsdashboardWindowTrackerWindow *self, EsdashboardWindowTrackerWorkspace *inWorkspace);

	EsdashboardWindowTrackerMonitor* (*get_monitor)(EsdashboardWindowTrackerWindow *self);
	gboolean (*is_on_monitor)(EsdashboardWindowTrackerWindow *self, EsdashboardWindowTrackerMonitor *inMonitor);

	void (*get_geometry)(EsdashboardWindowTrackerWindow *self, gint *outX, gint *outY, gint *outWidth, gint *outHeight);
	void (*set_geometry)(EsdashboardWindowTrackerWindow *self, gint inX, gint inY, gint inWidth, gint inHeight);
	void (*move)(EsdashboardWindowTrackerWindow *self, gint inX, gint inY);
	void (*resize)(EsdashboardWindowTrackerWindow *self, gint inWidth, gint inHeight);
	void (*move_to_workspace)(EsdashboardWindowTrackerWindow *self, EsdashboardWindowTrackerWorkspace *inWorkspace);
	void (*activate)(EsdashboardWindowTrackerWindow *self);
	void (*close)(EsdashboardWindowTrackerWindow *self);

	gint (*get_pid)(EsdashboardWindowTrackerWindow *self);
	gchar** (*get_instance_names)(EsdashboardWindowTrackerWindow *self);

	ClutterContent* (*get_content)(EsdashboardWindowTrackerWindow *self);

	/* Signals */
	void (*name_changed)(EsdashboardWindowTrackerWindow *self);
	void (*state_changed)(EsdashboardWindowTrackerWindow *self,
							EsdashboardWindowTrackerWindowState inOldStates);
	void (*actions_changed)(EsdashboardWindowTrackerWindow *self,
							EsdashboardWindowTrackerWindowAction inOldActions);
	void (*icon_changed)(EsdashboardWindowTrackerWindow *self);
	void (*workspace_changed)(EsdashboardWindowTrackerWindow *self,
								EsdashboardWindowTrackerWorkspace *inOldWorkspace);
	void (*monitor_changed)(EsdashboardWindowTrackerWindow *self,
							EsdashboardWindowTrackerMonitor *inOldMonitor);
	void (*geometry_changed)(EsdashboardWindowTrackerWindow *self);
	void (*closed)(EsdashboardWindowTrackerWindow *self);
};

/* Public API */
GType esdashboard_window_tracker_window_get_type(void) G_GNUC_CONST;

gboolean esdashboard_window_tracker_window_is_equal(EsdashboardWindowTrackerWindow *inLeft,
													EsdashboardWindowTrackerWindow *inRight);

gboolean esdashboard_window_tracker_window_is_visible(EsdashboardWindowTrackerWindow *self);
gboolean esdashboard_window_tracker_window_is_visible_on_workspace(EsdashboardWindowTrackerWindow *self,
																	EsdashboardWindowTrackerWorkspace *inWorkspace);
gboolean esdashboard_window_tracker_window_is_visible_on_monitor(EsdashboardWindowTrackerWindow *self,
																	EsdashboardWindowTrackerMonitor *inMonitor);
void esdashboard_window_tracker_window_show(EsdashboardWindowTrackerWindow *self);
void esdashboard_window_tracker_window_hide(EsdashboardWindowTrackerWindow *self);

EsdashboardWindowTrackerWindow* esdashboard_window_tracker_window_get_parent(EsdashboardWindowTrackerWindow *self);

EsdashboardWindowTrackerWindowState esdashboard_window_tracker_window_get_state(EsdashboardWindowTrackerWindow *self);
EsdashboardWindowTrackerWindowAction esdashboard_window_tracker_window_get_actions(EsdashboardWindowTrackerWindow *self);

const gchar* esdashboard_window_tracker_window_get_name(EsdashboardWindowTrackerWindow *self);

GdkPixbuf* esdashboard_window_tracker_window_get_icon(EsdashboardWindowTrackerWindow *self);
const gchar* esdashboard_window_tracker_window_get_icon_name(EsdashboardWindowTrackerWindow *self);

EsdashboardWindowTrackerWorkspace* esdashboard_window_tracker_window_get_workspace(EsdashboardWindowTrackerWindow *self);
gboolean esdashboard_window_tracker_window_is_on_workspace(EsdashboardWindowTrackerWindow *self,
															EsdashboardWindowTrackerWorkspace *inWorkspace);
void esdashboard_window_tracker_window_move_to_workspace(EsdashboardWindowTrackerWindow *self,
															EsdashboardWindowTrackerWorkspace *inWorkspace);

EsdashboardWindowTrackerMonitor* esdashboard_window_tracker_window_get_monitor(EsdashboardWindowTrackerWindow *self);
gboolean esdashboard_window_tracker_window_is_on_monitor(EsdashboardWindowTrackerWindow *self,
															EsdashboardWindowTrackerMonitor *inMonitor);

void esdashboard_window_tracker_window_get_geometry(EsdashboardWindowTrackerWindow *self,
															gint *outX,
															gint *outY,
															gint *outWidth,
															gint *outHeight);
void esdashboard_window_tracker_window_set_geometry(EsdashboardWindowTrackerWindow *self,
															gint inX,
															gint inY,
															gint inWidth,
															gint inHeight);
void esdashboard_window_tracker_window_move(EsdashboardWindowTrackerWindow *self,
											gint inX,
											gint inY);
void esdashboard_window_tracker_window_resize(EsdashboardWindowTrackerWindow *self,
												gint inWidth,
												gint inHeight);

void esdashboard_window_tracker_window_activate(EsdashboardWindowTrackerWindow *self);
void esdashboard_window_tracker_window_close(EsdashboardWindowTrackerWindow *self);

gboolean esdashboard_window_tracker_window_is_stage(EsdashboardWindowTrackerWindow *self);
ClutterStage* esdashboard_window_tracker_window_get_stage(EsdashboardWindowTrackerWindow *self);
void esdashboard_window_tracker_window_show_stage(EsdashboardWindowTrackerWindow *self);
void esdashboard_window_tracker_window_hide_stage(EsdashboardWindowTrackerWindow *self);

gint esdashboard_window_tracker_window_get_pid(EsdashboardWindowTrackerWindow *self);
gchar** esdashboard_window_tracker_window_get_instance_names(EsdashboardWindowTrackerWindow *self);

ClutterContent* esdashboard_window_tracker_window_get_content(EsdashboardWindowTrackerWindow *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_WINDOW_TRACKER_WINDOW__ */
