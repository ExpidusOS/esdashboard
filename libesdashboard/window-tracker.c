/*
 * window-tracker: Tracks windows, workspaces, monitors and
 *                 listens for changes. It also bundles libwnck into one
 *                 class.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/window-tracker.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/window-tracker-backend.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
G_DEFINE_INTERFACE(EsdashboardWindowTracker,
					esdashboard_window_tracker,
					G_TYPE_OBJECT)


/* Signals */
enum
{
	SIGNAL_WINDOW_STACKING_CHANGED,

	SIGNAL_ACTIVE_WINDOW_CHANGED,
	SIGNAL_WINDOW_OPENED,
	SIGNAL_WINDOW_CLOSED,
	SIGNAL_WINDOW_GEOMETRY_CHANGED,
	SIGNAL_WINDOW_ACTIONS_CHANGED,
	SIGNAL_WINDOW_STATE_CHANGED,
	SIGNAL_WINDOW_ICON_CHANGED,
	SIGNAL_WINDOW_NAME_CHANGED,
	SIGNAL_WINDOW_WORKSPACE_CHANGED,
	SIGNAL_WINDOW_MONITOR_CHANGED,

	SIGNAL_ACTIVE_WORKSPACE_CHANGED,
	SIGNAL_WORKSPACE_ADDED,
	SIGNAL_WORKSPACE_REMOVED,
	SIGNAL_WORKSPACE_NAME_CHANGED,

	SIGNAL_PRIMARY_MONITOR_CHANGED,
	SIGNAL_MONITOR_ADDED,
	SIGNAL_MONITOR_REMOVED,
	SIGNAL_MONITOR_GEOMETRY_CHANGED,
	SIGNAL_SCREEN_SIZE_CHANGED,

	SIGNAL_WINDOW_MANAGER_CHANGED,

	SIGNAL_LAST
};

static guint EsdashboardWindowTrackerSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, vfunc)  \
	g_warning("Object of type %s does not implement required virtual function EsdashboardWindowTracker::%s",\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

/* Default signal handler for signal "window_closed" */
static void _esdashboard_window_tracker_real_window_closed(EsdashboardWindowTracker *self,
															EsdashboardWindowTrackerWindow *inWindow)
{
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	/* By default (if not overidden) emit "closed" signal at window */
	g_signal_emit_by_name(inWindow, "closed");
}


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void esdashboard_window_tracker_default_init(EsdashboardWindowTrackerInterface *iface)
{
	static gboolean		initialized=FALSE;
	GParamSpec			*property;

	/* The following virtual functions should be overriden if default
	 * implementation does not fit.
	 */
	iface->window_closed=_esdashboard_window_tracker_real_window_closed;

	/* Define properties, signals and actions */
	if(!initialized)
	{
		/* Define properties */
		property=g_param_spec_object("active-window",
										"Active window",
										"The current active window",
										ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
										G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		property=g_param_spec_object("active-workspace",
										"Active workspace",
										"The current active workspace",
										ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE,
										G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		property=g_param_spec_object("primary-monitor",
										"Primary monitor",
										"The current primary monitor",
										ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
										G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		/* Define signals */
		/**
		 * EsdashboardWindowTracker::window-tacking-changed:
		 * @self: The window tracker
		 *
		 * The ::window-tacking-changed signal is emitted whenever the stacking
		 * order of the windows at the desktop environment has changed.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_STACKING_CHANGED]=
			g_signal_new("window-stacking-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_stacking_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		/**
		 * EsdashboardWindowTracker::active-window-changed:
		 * @self: The window tracker
		 * @inPreviousActiveWindow: The #EsdashboardWindowTrackerWindow which
		 *    was the active window before this change
		 * @inCurrentActiveWindow: The #EsdashboardWindowTrackerWindow which is
		 *    the active window now
		 *
		 * The ::active-window-changed is emitted when the active window has
		 * changed.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_ACTIVE_WINDOW_CHANGED]=
			g_signal_new("active-window-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, active_window_changed),
							NULL,
							NULL,
							_esdashboard_marshal_VOID__OBJECT_OBJECT,
							G_TYPE_NONE,
							2,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * EsdashboardWindowTracker::window-opened:
		 * @self: The window tracker
		 * @inWindow: The #EsdashboardWindowTrackerWindow opened
		 *
		 * The ::window-opened signal is emitted whenever a new window was opened
		 * at the desktop environment.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_OPENED]=
			g_signal_new("window-opened",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_opened),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * EsdashboardWindowTracker::window-closed:
		 * @self: The window tracker
		 * @inWindow: The #EsdashboardWindowTrackerWindow closed
		 *
		 * The ::window-closed signal is emitted when a window was closed and is
		 * not available anymore.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_CLOSED]=
			g_signal_new("window-closed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_closed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * EsdashboardWindowTracker::window-geometry-changed:
		 * @self: The window tracker
		 * @inWindow: The #EsdashboardWindowTrackerWindow which changed its size or position
		 *
		 * The ::window-geometry-changed signal is emitted when the size of a
		 * window or its position at screen of the desktop environment has changed.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_GEOMETRY_CHANGED]=
			g_signal_new("window-geometry-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_geometry_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * EsdashboardWindowTracker::window-actions-changed:
		 * @self: The window tracker
		 * @inWindow: The #EsdashboardWindowTrackerWindow changed the availability
		 *    of actions
		 *
		 * The ::window-actions-changed signal is emitted whenever the availability
		 * of actions of a window changes.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_ACTIONS_CHANGED]=
			g_signal_new("window-actions-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_actions_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * EsdashboardWindowTracker::window-state-changed:
		 * @self: The window tracker
		 * @inWindow: The #EsdashboardWindowTrackerWindow changed its state
		 *
		 * The ::window-state-changed signal is emitted whenever a window
		 * changes its state. This can happen when @inWindow is (un)minimized,
		 * (un)maximized, (un)pinned, (un)set fullscreen etc. See
		 * #EsdashboardWindowTrackerWindowState for the complete list of states
		 * that might have changed.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_STATE_CHANGED]=
			g_signal_new("window-state-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_state_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * EsdashboardWindowTracker::window-icon-changed:
		 * @self: The window tracker
		 * @inWindow: The #EsdashboardWindowTrackerWindow changed its icon
		 *
		 * The ::window-icon-changed signal is emitted whenever a window
		 * changes its icon
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_ICON_CHANGED]=
			g_signal_new("window-icon-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_icon_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * EsdashboardWindowTracker::window-name-changed:
		 * @self: The window tracker
		 * @inWindow: The #EsdashboardWindowTrackerWindow changed its name
		 *
		 * The ::window-name-changed signal is emitted whenever a window
		 * changes its name, known as window title.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_NAME_CHANGED]=
			g_signal_new("window-name-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_name_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * EsdashboardWindowTracker::window-workspace-changed:
		 * @self: The window tracker
		 * @inWindow: The #EsdashboardWindowTrackerWindow moved to another workspace
		 * @inWorkspace: The #EsdashboardWindowTrackerWorkspace where the window
		 *    @inWindow was moved to
		 *
		 * The ::window-workspace-changed signal is emitted whenever a window
		 * moves to another workspace.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_WORKSPACE_CHANGED]=
			g_signal_new("window-workspace-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_workspace_changed),
							NULL,
							NULL,
							_esdashboard_marshal_VOID__OBJECT_OBJECT,
							G_TYPE_NONE,
							2,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * EsdashboardWindowTracker::window-monitor-changed:
		 * @self: The window tracker
		 * @inWindow: The #EsdashboardWindowTrackerWindow moved to another monitor
		 * @inPreviousMonitor: The #EsdashboardWindowTrackerMonitor where the window
		 *    @inWindow was located at before this change
		 * @inCurrentMonitor: The #EsdashboardWindowTrackerMonitor where the window
		 *    @inWindow is located at now
		 *
		 * The ::window-monitor-changed signal is emitted whenever a window
		 * moves to another monitor.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_MONITOR_CHANGED]=
			g_signal_new("window-monitor-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_monitor_changed),
							NULL,
							NULL,
							_esdashboard_marshal_VOID__OBJECT_OBJECT_OBJECT,
							G_TYPE_NONE,
							3,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * EsdashboardWindowTracker::active-workspace-changed:
		 * @self: The window tracker
		 * @inPreviousActiveWorkspace: The #EsdashboardWindowTrackerWorkspace which
		 *    was the active workspace before this change
		 * @inCurrentActiveWorkspace: The #EsdashboardWindowTrackerWorkspace which
		 *    is the active workspace now
		 *
		 * The ::active-workspace-changed signal is emitted when the active workspace
		 * has changed.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_ACTIVE_WORKSPACE_CHANGED]=
			g_signal_new("active-workspace-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, active_workspace_changed),
							NULL,
							NULL,
							_esdashboard_marshal_VOID__OBJECT_OBJECT,
							G_TYPE_NONE,
							2,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * EsdashboardWindowTracker::workspace-added:
		 * @self: The window tracker
		 * @inWorkspace: The #EsdashboardWindowTrackerWorkspace added
		 *
		 * The ::workspace-added signal is emitted whenever a new workspace was added.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_ADDED]=
			g_signal_new("workspace-added",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, workspace_added),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * EsdashboardWindowTracker::workspace-removed:
		 * @self: The window tracker
		 * @inWorkspace: The #EsdashboardWindowTrackerWorkspace removed
		 *
		 * The ::workspace-removed signal is emitted whenever a workspace was removed.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_REMOVED]=
			g_signal_new("workspace-removed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, workspace_removed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * EsdashboardWindowTracker::workspace-name-changed:
		 * @self: The window tracker
		 * @inWorkspace: The #EsdashboardWindowTrackerWorkspace changed its name
		 *
		 * The ::workspace-name-changed signal is emitted whenever a workspace
		 * changes its name.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_NAME_CHANGED]=
			g_signal_new("workspace-name-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, workspace_name_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * EsdashboardWindowTracker::primary-monitor-changed:
		 * @self: The window tracker
		 * @inPreviousPrimaryMonitor: The #EsdashboardWindowTrackerMonitor which
		 *    was the primary monitor before this change
		 * @inCurrentPrimaryMonitor: The #EsdashboardWindowTrackerMonitor which
		 *    is the new primary monitor now
		 *
		 * The ::primary-monitor-changed signal is emitted when another monitor
		 * was configured to be the primary monitor.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_PRIMARY_MONITOR_CHANGED]=
			g_signal_new("primary-monitor-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, primary_monitor_changed),
							NULL,
							NULL,
							_esdashboard_marshal_VOID__OBJECT_OBJECT,
							G_TYPE_NONE,
							2,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * EsdashboardWindowTracker::monitor-added:
		 * @self: The window tracker
		 * @inMonitor: The #EsdashboardWindowTrackerMonitor added
		 *
		 * The ::monitor-added signal is emitted whenever a new monitor was added.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_MONITOR_ADDED]=
			g_signal_new("monitor-added",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, monitor_added),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * EsdashboardWindowTracker::monitor-removed:
		 * @self: The window tracker
		 * @inMonitor: The #EsdashboardWindowTrackerMonitor removed
		 *
		 * The ::monitor-removed signal is emitted whenever a monitor was removed.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_MONITOR_REMOVED]=
			g_signal_new("monitor-removed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, monitor_removed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * EsdashboardWindowTracker::monitor-geometry-changed:
		 * @self: The window tracker
		 * @inMonitor: The #EsdashboardWindowTrackerMonitor which changed its size or position
		 *
		 * The ::monitor-geometry-changed signal is emitted when the size of a
		 * monitor or its position at screen of the desktop environment has changed.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_MONITOR_GEOMETRY_CHANGED]=
			g_signal_new("monitor-geometry-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, monitor_geometry_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * EsdashboardWindowTracker::screen-size-changed:
		 * @self: The window tracker
		 *
		 * The ::screen-size-changed signal is emitted when the screen size of
		 * the desktop environment has been changed, e.g. one monitor changed its
		 * resolution.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_SCREEN_SIZE_CHANGED]=
			g_signal_new("screen-size-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, screen_size_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		/**
		 * EsdashboardWindowTracker::window-manager-changed:
		 * @self: The window tracker
		 *
		 * The ::window-manager-changed signal is emitted when the window manager
		 * of the desktop environment has been replaced with a new one.
		 */
		EsdashboardWindowTrackerSignals[SIGNAL_WINDOW_MANAGER_CHANGED]=
			g_signal_new("window-manager-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(EsdashboardWindowTrackerInterface, window_manager_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		/* Set flag that base initialization was done for this interface */
		initialized=TRUE;
	}
}


/* IMPLEMENTATION: Public API */

/**
 * esdashboard_window_tracker_get_default:
 *
 * Retrieves the singleton instance of #EsdashboardWindowTracker. If not needed
 * anymore the caller must unreference the returned object instance.
 *
 * This function is the logical equivalent of:
 *
 * |[<!-- language="C" -->
 *   EsdashboardWindowTrackerBackend *backend;
 *   EsdashboardWindowTracker        *tracker;
 *
 *   backend=esdashboard_window_tracker_backend_get_default();
 *   tracker=esdashboard_window_tracker_backend_get_window_tracker(backend);
 *   g_object_unref(backend);
 * ]|
 *
 * Return value: (transfer full): The instance of #EsdashboardWindowTracker.
 */
EsdashboardWindowTracker* esdashboard_window_tracker_get_default(void)
{
	EsdashboardWindowTrackerBackend		*backend;
	EsdashboardWindowTracker			*windowTracker;

	/* Get default window tracker backend */
	backend=esdashboard_window_tracker_backend_get_default();
	if(!backend)
	{
		g_critical("Could not get default window tracker backend");
		return(NULL);
	}

	/* Get window tracker object instance of backend */
	windowTracker=esdashboard_window_tracker_backend_get_window_tracker(backend);

	/* Release allocated resources */
	if(backend) g_object_unref(backend);

	/* Return window tracker object instance */
	return(windowTracker);
}

/**
 * esdashboard_window_tracker_get_windows:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the list of #EsdashboardWindowTrackerWindow tracked by @self.
 * The list is ordered: the first element in the list is the first
 * #EsdashboardWindowTrackerWindow, etc..
 *
 * Return value: (element-type EsdashboardWindowTrackerWindow) (transfer none):
 *   The list of #EsdashboardWindowTrackerWindow. The list should not be modified
 *   nor freed, as it is owned by Esdashboard.
 */
GList* esdashboard_window_tracker_get_windows(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_windows)
	{
		return(iface->get_windows(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_windows");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_windows_stacked:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the list of #EsdashboardWindowTrackerWindow tracked by @self in
 * stacked order from bottom to top. The list is ordered: the first element in
 * the list is the most bottom #EsdashboardWindowTrackerWindow, etc..
 *
 * Return value: (element-type EsdashboardWindowTrackerWindow) (transfer none):
 *   The list of #EsdashboardWindowTrackerWindow in stacked order. The list should
 *   not be modified nor freed, as it is owned by Esdashboard.
 */
GList* esdashboard_window_tracker_get_windows_stacked(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_windows_stacked)
	{
		return(iface->get_windows_stacked(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_windows_stacked");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_active_window:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the currently active #EsdashboardWindowTrackerWindow.
 *
 * Return value: (transfer none): The #EsdashboardWindowTrackerWindow currently
 *   active or %NULL if not determinable. The returned object is owned by Esdashboard
 *   and it should not be referenced or unreferenced.
 */
EsdashboardWindowTrackerWindow* esdashboard_window_tracker_get_active_window(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_active_window)
	{
		return(iface->get_active_window(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_active_window");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_workspaces_count:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the number of #EsdashboardWindowTrackerWorkspace tracked by @self.
 *
 * Return value: The number of #EsdashboardWindowTrackerWorkspace
 */
gint esdashboard_window_tracker_get_workspaces_count(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), 0);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_workspaces_count)
	{
		return(iface->get_workspaces_count(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_workspaces_count");
	return(0);
}

/**
 * esdashboard_window_tracker_get_workspaces:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the list of #EsdashboardWindowTrackerWorkspace tracked by @self.
 * The list is ordered: the first element in the list is the first
 * #EsdashboardWindowTrackerWorkspace, etc..
 *
 * Return value: (element-type EsdashboardWindowTrackerWorkspace) (transfer none):
 *   The list of #EsdashboardWindowTrackerWorkspace. The list should not be modified
 *   nor freed, as it is owned by Esdashboard.
 */
GList* esdashboard_window_tracker_get_workspaces(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_workspaces)
	{
		return(iface->get_workspaces(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_workspaces");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_active_workspace:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the currently active #EsdashboardWindowTrackerWorkspace.
 *
 * Return value: (transfer none): The #EsdashboardWindowTrackerWorkspace currently
 *   active or %NULL if not determinable. The returned object is owned by Esdashboard
 *   and it should not be referenced or unreferenced.
 */
EsdashboardWindowTrackerWorkspace* esdashboard_window_tracker_get_active_workspace(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_active_workspace)
	{
		return(iface->get_active_workspace(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_active_workspace");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_workspace_by_number:
 * @self: A #EsdashboardWindowTracker
 * @inNumber: The workspace index, starting from 0
 *
 * Retrieves the #EsdashboardWindowTrackerWorkspace at index @inNumber.
 *
 * Return value: (transfer none): The #EsdashboardWindowTrackerWorkspace at the
 *   index or %NULL if no such workspace exists. The returned object is owned by
 *   Esdashboard and it should not be referenced or unreferenced.
 */
EsdashboardWindowTrackerWorkspace* esdashboard_window_tracker_get_workspace_by_number(EsdashboardWindowTracker *self,
																						gint inNumber)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(inNumber>=0, NULL);
	g_return_val_if_fail(inNumber<esdashboard_window_tracker_get_workspaces_count(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_workspace_by_number)
	{
		return(iface->get_workspace_by_number(self, inNumber));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_workspace_by_number");
	return(NULL);
}

/**
 * esdashboard_window_tracker_supports_multiple_monitors:
 * @self: A #EsdashboardWindowTracker
 *
 * Determines if window tracker at @self supports multiple monitors.
 *
 * If multiple monitors are supported, %TRUE will be returned and the number
 * of monitors can be determined by calling esdashboard_window_tracker_get_monitors_count().
 * Also each monitor can be accessed esdashboard_window_tracker_get_monitor_by_number()
 * and other monitor related functions.
 *
 * If multiple monitors are not supported or the desktop environment cannot provide
 * this kind of information, %FALSE will be returned.
 *
 * Return value: %TRUE if @self supports multiple monitors, otherwise %FALSE.
 */
gboolean esdashboard_window_tracker_supports_multiple_monitors(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), FALSE);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->supports_multiple_monitors)
	{
		return(iface->supports_multiple_monitors(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "supports_multiple_monitors");
	return(FALSE);
}

/**
 * esdashboard_window_tracker_get_monitors_count:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the number of #EsdashboardWindowTrackerMonitor tracked by @self.
 *
 * Return value: The number of #EsdashboardWindowTrackerMonitor
 */
gint esdashboard_window_tracker_get_monitors_count(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), 0);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_monitors_count)
	{
		return(iface->get_monitors_count(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_monitors_count");
	return(0);
}

/**
 * esdashboard_window_tracker_get_monitors:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the list of #EsdashboardWindowTrackerMonitor tracked by @self.
 * The list is ordered: the first element in the list is the first #EsdashboardWindowTrackerMonitor,
 * etc..
 *
 * Return value: (element-type EsdashboardWindowTrackerMonitor) (transfer none):
 *   The list of #EsdashboardWindowTrackerMonitor. The list should not be modified
 *   nor freed, as it is owned by Esdashboard.
 */
GList* esdashboard_window_tracker_get_monitors(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_monitors)
	{
		return(iface->get_monitors(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_monitors");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_primary_monitor:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the primary #EsdashboardWindowTrackerMonitor the user configured
 * at its desktop environment.
 *
 * Return value: (transfer none): The #EsdashboardWindowTrackerMonitor configured
 *   as primary or %NULL if no primary monitor exists. The returned object is
 *   owned by Esdashboard and it should not be referenced or unreferenced.
 */
EsdashboardWindowTrackerMonitor* esdashboard_window_tracker_get_primary_monitor(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_primary_monitor)
	{
		return(iface->get_primary_monitor(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_primary_monitor");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_monitor_by_number:
 * @self: A #EsdashboardWindowTracker
 * @inNumber: The monitor index, starting from 0
 *
 * Retrieves the #EsdashboardWindowTrackerMonitor at index @inNumber.
 *
 * Return value: (transfer none): The #EsdashboardWindowTrackerMonitor at the
 *   index or %NULL if no such monitor exists. The returned object is owned by
 *   Esdashboard and it should not be referenced or unreferenced.
 */
EsdashboardWindowTrackerMonitor* esdashboard_window_tracker_get_monitor_by_number(EsdashboardWindowTracker *self,
																					gint inNumber)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(inNumber>=0, NULL);
	g_return_val_if_fail(inNumber<esdashboard_window_tracker_get_monitors_count(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_monitor_by_number)
	{
		return(iface->get_monitor_by_number(self, inNumber));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_monitor_by_number");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_monitor_by_position:
 * @self: A #EsdashboardWindowTracker
 * @inX: The X coordinate of position at screen
 * @inY: The Y coordinate of position at screen
 *
 * Retrieves the monitor containing the position at @inX,@inY at screen.
 *
 * Return value: (transfer none): The #EsdashboardWindowTrackerMonitor for the
 *   requested position or %NULL if no monitor could be found containing the
 *   position. The returned object is owned by Esdashboard and it should not be
 *   referenced or unreferenced.
 */
EsdashboardWindowTrackerMonitor* esdashboard_window_tracker_get_monitor_by_position(EsdashboardWindowTracker *self,
																						gint inX,
																						gint inY)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_monitor_by_position)
	{
		return(iface->get_monitor_by_position(self, inX, inY));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_monitor_by_position");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_screen_size:
 * @self: A #EsdashboardWindowTracker
 * @outWidth: (out): Return location for width of screen.
 * @outHeight: (out): Return location for height of screen.
 *
 * Retrieves width and height of screen of the desktop environment. The screen
 * contains all connected monitors so it the total size of the desktop environment.
 */
void esdashboard_window_tracker_get_screen_size(EsdashboardWindowTracker *self, gint *outWidth, gint *outHeight)
{
	EsdashboardWindowTrackerInterface		*iface;
	gint									width, height;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_screen_size)
	{
		/* Get screen size */
		iface->get_screen_size(self, &width, &height);

		/* Store result where possible */
		if(outWidth) *outWidth=width;
		if(outHeight) *outHeight=height;

		return;
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_screen_width");
}

/**
 * esdashboard_window_tracker_get_window_manager_name:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the name of window manager managing the desktop environment, i.e.
 * windows, workspaces etc.
 *
 * Return value: A string with name of the window manager
 */
const gchar* esdashboard_window_tracker_get_window_manager_name(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_window_manager_name)
	{
		return(iface->get_window_manager_name(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_window_manager_name");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_root_window:
 * @self: A #EsdashboardWindowTracker
 *
 * Retrieves the root window of the desktop environment. The root window is
 * usually the desktop seen at the background of the desktop environment.
 *
 * Return value: (transfer none): The #EsdashboardWindowTrackerWindow representing
 *   the root window or %NULL if not available. The returned object is owned by
 *   Esdashboard and it should not be referenced or unreferenced.
 */
EsdashboardWindowTrackerWindow* esdashboard_window_tracker_get_root_window(EsdashboardWindowTracker *self)
{
	EsdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_root_window)
	{
		return(iface->get_root_window(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_root_window");
	return(NULL);
}

/**
 * esdashboard_window_tracker_get_stage_window:
 * @self: A #EsdashboardWindowTracker
 * @inStage: A #ClutterStage
 *
 * Retrieves the window created for the requested stage @inStage.
 *
 * This function is the logical equivalent of:
 *
 * |[<!-- language="C" -->
 *   EsdashboardWindowTrackerBackend *backend;
 *   EsdashboardWindowTrackerWindow  *stageWindow;
 *
 *   backend=esdashboard_window_tracker_backend_get_default();
 *   stageWindow=esdashboard_window_tracker_backend_get_window_for_stage(backend, inStage);
 *   g_object_unref(backend);
 * ]|
 *
 * Return value: (transfer none): The #EsdashboardWindowTrackerWindow representing
 *   the window of requested stage or %NULL if not available. The returned object
 *   is owned by Esdashboard and it should not be referenced or unreferenced.
 */
EsdashboardWindowTrackerWindow* esdashboard_window_tracker_get_stage_window(EsdashboardWindowTracker *self,
																			ClutterStage *inStage)
{
	EsdashboardWindowTrackerBackend		*backend;
	EsdashboardWindowTrackerWindow		*stageWindow;

	/* Get default window tracker backend */
	backend=esdashboard_window_tracker_backend_get_default();
	if(!backend)
	{
		g_critical("Could not get default window tracker backend");
		return(NULL);
	}

	/* Get window for requested stage from backend */
	stageWindow=esdashboard_window_tracker_backend_get_window_for_stage(backend, inStage);

	/* Release allocated resources */
	if(backend) g_object_unref(backend);

	/* Return window object instance */
	return(stageWindow);
}
