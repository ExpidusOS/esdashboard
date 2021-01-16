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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/x11/window-tracker-x11.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#ifdef CLUTTER_WINDOWING_GDK
#include <clutter/gdk/clutter-gdk.h>
#endif
#include <gdk/gdkx.h>
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include <libesdashboard/window-tracker.h>
#include <libesdashboard/x11/window-tracker-monitor-x11.h>
#include <libesdashboard/x11/window-tracker-window-x11.h>
#include <libesdashboard/x11/window-tracker-workspace-x11.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/application.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
static void _esdashboard_window_tracker_x11_window_tracker_iface_init(EsdashboardWindowTrackerInterface *iface);

struct _EsdashboardWindowTrackerX11Private
{
	/* Properties related */
	EsdashboardWindowTrackerWindowX11		*activeWindow;
	EsdashboardWindowTrackerWorkspaceX11	*activeWorkspace;
	EsdashboardWindowTrackerMonitorX11		*primaryMonitor;

	/* Instance related */
	GList									*windows;
	GList									*windowsStacked;
	GList									*workspaces;
	GList									*monitors;

	EsdashboardApplication					*application;
	gboolean								isAppSuspended;
	guint									suspendSignalID;

	WnckScreen								*screen;

	gboolean								supportsMultipleMonitors;
	GdkScreen								*gdkScreen;
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkDisplay								*gdkDisplay;
	gboolean								needScreenSizeUpdate;
	gint									screenWidth, screenHeight;
#endif
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardWindowTrackerX11,
						esdashboard_window_tracker_x11,
						G_TYPE_OBJECT,
						G_ADD_PRIVATE(EsdashboardWindowTrackerX11)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_WINDOW_TRACKER, _esdashboard_window_tracker_x11_window_tracker_iface_init))

/* Properties */
enum
{
	PROP_0,

	/* Overriden properties of interface: EsdashboardWindowTracker */
	PROP_ACTIVE_WINDOW,
	PROP_ACTIVE_WORKSPACE,
	PROP_PRIMARY_MONITOR,

	PROP_LAST
};

static GParamSpec* EsdashboardWindowTrackerX11Properties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Free workspace object */
static void _esdashboard_window_tracker_x11_free_workspace(EsdashboardWindowTrackerX11 *self,
															EsdashboardWindowTrackerWorkspaceX11 *inWorkspace)
{
	EsdashboardWindowTrackerX11Private		*priv;
	GList									*iter;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));

	priv=self->priv;

#if DEBUG
	/* There must be only one reference on that workspace object */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Freeing workspace %s@%p named '%s' with ref-count=%d",
						G_OBJECT_TYPE_NAME(inWorkspace), inWorkspace,
						esdashboard_window_tracker_workspace_get_name(ESDASHBOARD_WINDOW_TRACKER_WORKSPACE(inWorkspace)),
						G_OBJECT(inWorkspace)->ref_count);
	g_assert(G_OBJECT(inWorkspace)->ref_count==1);
#endif

	/* Find entry in workspace list and remove it if found */
	iter=g_list_find(priv->workspaces, inWorkspace);
	if(iter)
	{
		priv->workspaces=g_list_delete_link(priv->workspaces, iter);
	}

	/* Free workspace object */
	g_object_unref(inWorkspace);
}

/* Get workspace object for requested wnck workspace */
static EsdashboardWindowTrackerWorkspaceX11* _esdashboard_window_tracker_x11_get_workspace_for_wnck(EsdashboardWindowTrackerX11 *self,
																									WnckWorkspace *inWorkspace)
{
	EsdashboardWindowTrackerX11Private		*priv;
	GList									*iter;
	EsdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), NULL);

	priv=self->priv;

	/* Iterate through list of workspace object and check if an object for the
	 * request wnck workspace exist. If one is found then return this existing
	 * workspace object.
	 */
	for(iter=priv->workspaces; iter; iter=g_list_next(iter))
	{
		/* Get currently iterated workspace object */
		workspace=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(iter->data);
		if(!workspace) continue;

		/* Check if this workspace object wraps the requested wnck workspace */
		if(esdashboard_window_tracker_workspace_x11_get_workspace(workspace)==inWorkspace)
		{
			/* Return existing workspace object */
			return(workspace);
		}
	}

	/* If we get here, return NULL as we have not found a matching workspace
	 * object for the requested wnck workspace.
	 */
	return(NULL);
}

/* Create workspace object which must not exist yet */
static EsdashboardWindowTrackerWorkspaceX11* _esdashboard_window_tracker_x11_create_workspace_for_wnck(EsdashboardWindowTrackerX11 *self,
																										WnckWorkspace *inWorkspace)
{
	EsdashboardWindowTrackerX11Private		*priv;
	EsdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), NULL);

	priv=self->priv;

	/* Iterate through list of workspace object and check if an object for the
	 * request wnck workspace exist. If one is found then the existing workspace object.
	 */
	workspace=_esdashboard_window_tracker_x11_get_workspace_for_wnck(self, inWorkspace);
	if(workspace)
	{
		/* Return existing workspace object */
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"A workspace object %s@%p for wnck workspace %s@%p named '%s' exists already",
							G_OBJECT_TYPE_NAME(workspace), workspace,
							G_OBJECT_TYPE_NAME(inWorkspace), inWorkspace, wnck_workspace_get_name(inWorkspace));

		return(workspace);
	}

	/* Create workspace object */
	workspace=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(g_object_new(ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE_X11,
														"workspace", inWorkspace,
														NULL));
	if(!workspace)
	{
		g_critical("Could not create workspace object of type %s for workspace '%s'",
					g_type_name(ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE_X11),
					wnck_workspace_get_name(inWorkspace));
		return(NULL);
	}

	/* Add new workspace object to list of workspace objects */
	priv->workspaces=g_list_prepend(priv->workspaces, workspace);

	/* Return new workspace object */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Created workspace object %s@%p for wnck workspace %s@%p named '%s'",
						G_OBJECT_TYPE_NAME(workspace), workspace,
						G_OBJECT_TYPE_NAME(inWorkspace), inWorkspace, wnck_workspace_get_name(inWorkspace));
	return(workspace);
}

/* Free window object */
static void _esdashboard_window_tracker_x11_free_window(EsdashboardWindowTrackerX11 *self,
														EsdashboardWindowTrackerWindowX11 *inWindow)
{
	EsdashboardWindowTrackerX11Private		*priv;
	GList									*iter;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	priv=self->priv;

#if DEBUG
	/* There must be only one reference on that window object */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Freeing window %s@%p named '%s' with ref-count=%d",
						G_OBJECT_TYPE_NAME(inWindow), inWindow,
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(inWindow)),
						G_OBJECT(inWindow)->ref_count);
	g_assert(G_OBJECT(inWindow)->ref_count==1);
#endif

	/* Find entry in window lists and remove it if found */
	iter=g_list_find(priv->windows, inWindow);
	if(iter)
	{
		priv->windows=g_list_delete_link(priv->windows, iter);
	}

	iter=g_list_find(priv->windowsStacked, inWindow);
	if(iter)
	{
		priv->windowsStacked=g_list_delete_link(priv->windowsStacked, iter);
	}

	/* Free window object */
	g_object_unref(inWindow);
}

/* Get window object for requested wnck window */
static EsdashboardWindowTrackerWindowX11* _esdashboard_window_tracker_x11_get_window_for_wnck(EsdashboardWindowTrackerX11 *self,
																								WnckWindow *inWindow)
{
	EsdashboardWindowTrackerX11Private		*priv;
	GList									*iter;
	EsdashboardWindowTrackerWindowX11		*window;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	priv=self->priv;

	/* Iterate through list of window object and check if an object for the
	 * request wnck window exist. If one is found then return this existing
	 * window object.
	 */
	for(iter=priv->windows; iter; iter=g_list_next(iter))
	{
		/* Get currently iterated window object */
		if(!ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(iter->data)) continue;

		window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(iter->data);
		if(!window) continue;

		/* Check if this window object wraps the requested wnck window */
		if(esdashboard_window_tracker_window_x11_get_window(window)==inWindow)
		{
			/* Return existing window object */
			return(window);
		}
	}

	/* If we get here, return NULL as we have not found a matching window
	 * object for the requested wnck window.
	 */
	return(NULL);
}

/* Build correctly ordered list of windows in stacked order. The list will not
 * take a reference at the window object and must not be unreffed if list is
 * freed.
 */
static void _esdashboard_window_tracker_x11_build_stacked_windows_list(EsdashboardWindowTrackerX11 *self)
{
	EsdashboardWindowTrackerX11Private		*priv;
	GList									*wnckWindowsStacked;
	GList									*newWindowsStacked;
	GList									*iter;
	WnckWindow								*wnckWindow;
	EsdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));

	priv=self->priv;

	/* Get list of stacked windows from wnck */
	wnckWindowsStacked=wnck_screen_get_windows_stacked(priv->screen);

	/* Build new list of stacked windows containing window objects */
	newWindowsStacked=NULL;
	for(iter=wnckWindowsStacked; iter; iter=g_list_next(iter))
	{
		/* Get wnck window iterated */
		wnckWindow=WNCK_WINDOW(iter->data);
		if(!wnckWindow) continue;

		/* Lookup window object from wnck window iterated */
		window=_esdashboard_window_tracker_x11_get_window_for_wnck(self, wnckWindow);
		if(window)
		{
			newWindowsStacked=g_list_prepend(newWindowsStacked, window);
		}
	}
	newWindowsStacked=g_list_reverse(newWindowsStacked);

	/* Release old stacked windows list */
	g_list_free(priv->windowsStacked);
	priv->windowsStacked=newWindowsStacked;
}

/* Create window object which must not exist yet */
static EsdashboardWindowTrackerWindowX11* _esdashboard_window_tracker_x11_create_window_for_wnck(EsdashboardWindowTrackerX11 *self,
																									WnckWindow *inWindow)
{
	EsdashboardWindowTrackerX11Private		*priv;
	EsdashboardWindowTrackerWindowX11		*window;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	priv=self->priv;

	/* Iterate through list of window object and check if an object for the
	 * request wnck window exist. If one is found then the existing window object.
	 */
	window=_esdashboard_window_tracker_x11_get_window_for_wnck(self, inWindow);
	if(window)
	{
		/* Return existing window object */
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"A window object %s@%p for wnck window %s@%p named '%s' exists already",
							G_OBJECT_TYPE_NAME(window), window,
							G_OBJECT_TYPE_NAME(inWindow), inWindow, wnck_window_get_name(inWindow));

		return(window);
	}

	/* Create window object */
	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(g_object_new(ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11,
																"window", inWindow,
																NULL));
	if(!window)
	{
		g_critical("Could not create window object of type %s for window '%s'",
					g_type_name(ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
					wnck_window_get_name(inWindow));
		return(NULL);
	}

	/* Add new window object to list of window objects */
	priv->windows=g_list_prepend(priv->windows, window);

	/* Assume window stacking changed to get correctly ordered list of windows */
	_esdashboard_window_tracker_x11_build_stacked_windows_list(self);

	/* Return new window object */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Created window object %s@%p for wnck window %s@%p named '%s'",
						G_OBJECT_TYPE_NAME(window), window,
						G_OBJECT_TYPE_NAME(inWindow), inWindow, wnck_window_get_name(inWindow));
	return(window);
}

/* Position and/or size of window has changed */
static void _esdashboard_window_tracker_x11_on_window_geometry_changed(EsdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' changed position and/or size",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window)));
	g_signal_emit_by_name(self, "window-geometry-changed", window);
}

/* Action items of window has changed */
static void _esdashboard_window_tracker_x11_on_window_actions_changed(EsdashboardWindowTrackerX11 *self,
																		EsdashboardWindowTrackerWindowAction inOldActions,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerWindowX11		*window;
	EsdashboardWindowTrackerWindowAction	newActions;
	EsdashboardWindowTrackerWindowAction	changedActions;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Debugging information */
	newActions=esdashboard_window_tracker_window_get_state(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window));
	changedActions=inOldActions ^ newActions;
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' changed actions from %u to %u with mask %u",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window)),
						inOldActions, newActions, changedActions);

	/* Emit signal */
	g_signal_emit_by_name(self, "window-actions-changed", window);
}

/* State of window has changed */
static void _esdashboard_window_tracker_x11_on_window_state_changed(EsdashboardWindowTrackerX11 *self,
																	EsdashboardWindowTrackerWindowState inOldState,
																	gpointer inUserData)
{
	EsdashboardWindowTrackerWindowX11		*window;
	EsdashboardWindowTrackerWindowState		newState;
	EsdashboardWindowTrackerWindowState		changedStates;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Debugging information */
	newState=esdashboard_window_tracker_window_get_state(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window));
	changedStates=inOldState ^ newState;
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' changed state from %u to %u with mask %u",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window)),
						inOldState, newState, changedStates);

	/* Emit signal */
	g_signal_emit_by_name(self, "window-state-changed", window);
}

/* Icon of window has changed */
static void _esdashboard_window_tracker_x11_on_window_icon_changed(EsdashboardWindowTrackerX11 *self,
																	gpointer inUserData)
{
	EsdashboardWindowTrackerWindowX11	*window;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' changed its icon",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window)));
	g_signal_emit_by_name(self, "window-icon-changed", window);
}

/* Name of window has changed */
static void _esdashboard_window_tracker_x11_on_window_name_changed(EsdashboardWindowTrackerX11 *self,
																	gpointer inUserData)
{
	EsdashboardWindowTrackerWindowX11	*window;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window changed its name to '%s'",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window)));
	g_signal_emit_by_name(self, "window-name-changed", window);
}

/* A window has moved to another monitor */
static void _esdashboard_window_tracker_x11_on_window_monitor_changed(EsdashboardWindowTrackerX11 *self,
																		EsdashboardWindowTrackerMonitor *inOldMonitor,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerWindowX11		*window;
	EsdashboardWindowTrackerMonitor			*newMonitor;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(!inOldMonitor || ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inOldMonitor));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Get monitor window resides on. */
	newMonitor=esdashboard_window_tracker_window_get_monitor(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window));

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' moved from monitor %d (%s) to %d (%s)",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window)),
						inOldMonitor ? esdashboard_window_tracker_monitor_get_number(inOldMonitor) : -1,
						(inOldMonitor && esdashboard_window_tracker_monitor_is_primary(inOldMonitor)) ? "primary" : "non-primary",
						newMonitor ? esdashboard_window_tracker_monitor_get_number(newMonitor) : -1,
						(newMonitor && esdashboard_window_tracker_monitor_is_primary(newMonitor)) ? "primary" : "non-primary");
	g_signal_emit_by_name(self, "window-monitor-changed", window, inOldMonitor, newMonitor);
}

/* A window has moved to another workspace */
static void _esdashboard_window_tracker_x11_on_window_workspace_changed(EsdashboardWindowTrackerX11 *self,
																		EsdashboardWindowTrackerWorkspace *inOldWorkspace,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerWindowX11		*window;
	EsdashboardWindowTrackerWorkspace		*newWorkspace;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(!inOldWorkspace || ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inOldWorkspace));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Get workspace window resides on. */
	newWorkspace=esdashboard_window_tracker_window_get_workspace(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window));

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' moved to workspace %d (%s)",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window)),
						newWorkspace ? esdashboard_window_tracker_workspace_get_number(newWorkspace) : -1,
						newWorkspace ? esdashboard_window_tracker_workspace_get_name(newWorkspace) : "<nil>");
	g_signal_emit_by_name(self, "window-workspace-changed", window, newWorkspace);
}

/* A window was activated */
static void _esdashboard_window_tracker_x11_on_active_window_changed(EsdashboardWindowTrackerX11 *self,
																		WnckWindow *inPreviousWindow,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;
	WnckScreen								*screen;
	EsdashboardWindowTrackerWindowX11		*oldActiveWindow;
	EsdashboardWindowTrackerWindowX11		*newActiveWindow;
	WnckWindow								*activeWindow;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(inPreviousWindow==NULL || WNCK_IS_WINDOW(inPreviousWindow));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;
	screen=WNCK_SCREEN(inUserData);

	/* Get and remember new active window */
	oldActiveWindow=priv->activeWindow;

	newActiveWindow=NULL;
	activeWindow=wnck_screen_get_active_window(screen);
	if(activeWindow)
	{
		newActiveWindow=_esdashboard_window_tracker_x11_get_window_for_wnck(self, activeWindow);
		if(!newActiveWindow)
		{
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"No window object of type %s found for new active wnck window %s@%p named '%s'",
								g_type_name(ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
								G_OBJECT_TYPE_NAME(activeWindow), activeWindow, wnck_window_get_name(activeWindow));

			return;
		}
	}

	priv->activeWindow=newActiveWindow;

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Active window changed from '%s' to '%s'",
						oldActiveWindow ? esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(oldActiveWindow)) : "<nil>",
						newActiveWindow ? esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(newActiveWindow)) : "<nil>");
	g_signal_emit_by_name(self, "active-window-changed", oldActiveWindow, priv->activeWindow);
}

/* A window was closed */
static void _esdashboard_window_tracker_x11_on_window_closed(EsdashboardWindowTrackerX11 *self,
																WnckWindow *inWindow,
																gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;
	EsdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;

	/* Should not happen but if closed window is the last active known one, then
	 * reset to NULL
	 */
	if(esdashboard_window_tracker_window_x11_get_window(priv->activeWindow)==inWindow)
	{
		priv->activeWindow=NULL;
	}

	/* Get window object for closed wnck window */
	window=_esdashboard_window_tracker_x11_get_window_for_wnck(self, inWindow);
	if(!window)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"No window object of type %s found for wnck window %s@%p named '%s'",
							g_type_name(ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
							G_OBJECT_TYPE_NAME(inWindow), inWindow, wnck_window_get_name(inWindow));

		return;
	}

	/* Remove all signal handlers for closed window */
	g_signal_handlers_disconnect_by_data(window, self);

	/* Emit signals */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' closed",
						wnck_window_get_name(inWindow));
	g_signal_emit_by_name(self, "window-closed", window);

	/* Remove window from window list */
	_esdashboard_window_tracker_x11_free_window(self, window);
}

/* A new window was opened */
static void _esdashboard_window_tracker_x11_on_window_opened(EsdashboardWindowTrackerX11 *self,
																WnckWindow *inWindow,
																gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;
	EsdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;

	/* Create window object */
	window=_esdashboard_window_tracker_x11_create_window_for_wnck(self, inWindow);
	if(!window) return;

	/* Connect signals on newly opened window */
	g_signal_connect_swapped(window, "actions-changed", G_CALLBACK(_esdashboard_window_tracker_x11_on_window_actions_changed), self);
	g_signal_connect_swapped(window, "state-changed", G_CALLBACK(_esdashboard_window_tracker_x11_on_window_state_changed), self);
	g_signal_connect_swapped(window, "icon-changed", G_CALLBACK(_esdashboard_window_tracker_x11_on_window_icon_changed), self);
	g_signal_connect_swapped(window, "name-changed", G_CALLBACK(_esdashboard_window_tracker_x11_on_window_name_changed), self);
	g_signal_connect_swapped(window, "monitor-changed", G_CALLBACK(_esdashboard_window_tracker_x11_on_window_monitor_changed), self);
	g_signal_connect_swapped(window, "workspace-changed", G_CALLBACK(_esdashboard_window_tracker_x11_on_window_workspace_changed), self);
	g_signal_connect_swapped(window, "geometry-changed", G_CALLBACK(_esdashboard_window_tracker_x11_on_window_geometry_changed), self);

	/* Block signal handler for 'geometry-changed' at window if application is suspended */
	if(priv->isAppSuspended)
	{
		g_signal_handlers_block_by_func(window, _esdashboard_window_tracker_x11_on_window_geometry_changed, self);
	}

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' created",
						wnck_window_get_name(inWindow));
	g_signal_emit_by_name(self, "window-opened", window);
}

/* Window stacking has changed */
static void _esdashboard_window_tracker_x11_on_window_stacking_changed(EsdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));

	/* Before emitting the signal, build a correctly ordered list of windows */
	_esdashboard_window_tracker_x11_build_stacked_windows_list(self);

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS, "Window stacking has changed");
	g_signal_emit_by_name(self, "window-stacking-changed");
}

/* A workspace changed its name */
static void _esdashboard_window_tracker_x11_on_workspace_name_changed(EsdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inUserData));

	workspace=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inUserData);

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Workspace #%d changed name to '%s'",
						esdashboard_window_tracker_workspace_get_number(ESDASHBOARD_WINDOW_TRACKER_WORKSPACE(workspace)),
						esdashboard_window_tracker_workspace_get_name(ESDASHBOARD_WINDOW_TRACKER_WORKSPACE(workspace)));
	g_signal_emit_by_name(self, "workspace-name-changed", workspace);

}

/* A workspace was activated */
static void _esdashboard_window_tracker_x11_on_active_workspace_changed(EsdashboardWindowTrackerX11 *self,
																		WnckWorkspace *inPreviousWorkspace,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;
	WnckScreen								*screen;
	EsdashboardWindowTrackerWorkspaceX11	*oldActiveWorkspace;
	EsdashboardWindowTrackerWorkspaceX11	*newActiveWorkspace;
	WnckWorkspace							*activeWorkspace;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(inPreviousWorkspace==NULL || WNCK_IS_WORKSPACE(inPreviousWorkspace));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;
	screen=WNCK_SCREEN(inUserData);

	/* Get and remember new active workspace */
	oldActiveWorkspace=priv->activeWorkspace;

	newActiveWorkspace=NULL;
	activeWorkspace=wnck_screen_get_active_workspace(screen);
	if(activeWorkspace)
	{
		newActiveWorkspace=_esdashboard_window_tracker_x11_get_workspace_for_wnck(self, activeWorkspace);
		if(!newActiveWorkspace)
		{
			ESDASHBOARD_DEBUG(self, WINDOWS,
						"No workspace object of type %s found for new active wnck workspace %s@%p named '%s'",
						g_type_name(ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE_X11),
						G_OBJECT_TYPE_NAME(activeWorkspace), activeWorkspace, wnck_workspace_get_name(activeWorkspace));

			return;
		}
	}

	priv->activeWorkspace=newActiveWorkspace;

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Active workspace changed from #%d (%s) to #%d (%s)",
						oldActiveWorkspace ? wnck_workspace_get_number(inPreviousWorkspace) : -1,
						oldActiveWorkspace ? wnck_workspace_get_name(inPreviousWorkspace) : "<nil>",
						priv->activeWorkspace ? wnck_workspace_get_number(activeWorkspace) : -1,
						priv->activeWorkspace ? wnck_workspace_get_name(activeWorkspace) : "<nil>");
	g_signal_emit_by_name(self, "active-workspace-changed", oldActiveWorkspace, priv->activeWorkspace);
}

/* A workspace was destroyed */
static void _esdashboard_window_tracker_x11_on_workspace_destroyed(EsdashboardWindowTrackerX11 *self,
																	WnckWorkspace *inWorkspace,
																	gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;
	EsdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WORKSPACE(inWorkspace));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;

	/* Should not happen but if destroyed workspace is the last active known one,
	 * then reset to NULL
	 */
	if(esdashboard_window_tracker_workspace_x11_get_workspace(priv->activeWorkspace)==inWorkspace)
	{
		priv->activeWorkspace=NULL;
	}

	/* Get workspace object for wnck workspace */
	workspace=_esdashboard_window_tracker_x11_get_workspace_for_wnck(self, inWorkspace);
	if(!workspace)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"No workspace object of type %s found for wnck workspace %s@%p named '%s'",
							g_type_name(ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
							G_OBJECT_TYPE_NAME(inWorkspace), inWorkspace, wnck_workspace_get_name(inWorkspace));

		return;
	}

	/* Remove all signal handlers for closed window */
	g_signal_handlers_disconnect_by_data(workspace, self);

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Workspace #%d (%s) destroyed",
						wnck_workspace_get_number(inWorkspace),
						wnck_workspace_get_name(inWorkspace));
	g_signal_emit_by_name(self, "workspace-removed", workspace);

	/* Remove workspace from workspace list */
	_esdashboard_window_tracker_x11_free_workspace(self, workspace);
}

/* A new workspace was created */
static void _esdashboard_window_tracker_x11_on_workspace_created(EsdashboardWindowTrackerX11 *self,
																	WnckWorkspace *inWorkspace,
																	gpointer inUserData)
{
	EsdashboardWindowTrackerWorkspaceX11		*workspace;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WORKSPACE(inWorkspace));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	/* Create workspace object */
	workspace=_esdashboard_window_tracker_x11_create_workspace_for_wnck(self, inWorkspace);
	if(!workspace) return;

	/* Connect signals on newly created workspace */
	g_signal_connect_swapped(workspace, "name-changed", G_CALLBACK(_esdashboard_window_tracker_x11_on_workspace_name_changed), self);

	/* Emit signal */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"New workspace #%d (%s) created",
						wnck_workspace_get_number(inWorkspace),
						wnck_workspace_get_name(inWorkspace));
	g_signal_emit_by_name(self, "workspace-added", workspace);
}

/* Primary monitor has changed */
static void _esdashboard_window_tracker_x11_on_primary_monitor_changed(EsdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;
	EsdashboardWindowTrackerMonitorX11		*monitor;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inUserData));

	priv=self->priv;
	monitor=ESDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inUserData);

	/* If monitor emitting this signal is the (new) primary one
	 * then update primary monitor value of this instance.
	 */
	if(esdashboard_window_tracker_monitor_is_primary(ESDASHBOARD_WINDOW_TRACKER_MONITOR(monitor)) &&
		priv->primaryMonitor!=monitor)
	{
		EsdashboardWindowTrackerMonitorX11	*oldMonitor;

		/* Remember old monitor for signal emission */
		oldMonitor=priv->primaryMonitor;

		/* Set value */
		priv->primaryMonitor=monitor;

		/* Emit signal */
		g_signal_emit_by_name(self, "primary-monitor-changed", oldMonitor, priv->primaryMonitor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowTrackerX11Properties[PROP_PRIMARY_MONITOR]);

		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Primary monitor changed from %d to %d",
							oldMonitor ? esdashboard_window_tracker_monitor_get_number(ESDASHBOARD_WINDOW_TRACKER_MONITOR(oldMonitor)) : -1,
							esdashboard_window_tracker_monitor_get_number(ESDASHBOARD_WINDOW_TRACKER_MONITOR(monitor)));
	}
}

/* A monitor has changed its position and/or size */
static void _esdashboard_window_tracker_x11_on_monitor_geometry_changed(EsdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerMonitorX11			*monitor;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inUserData));

	monitor=ESDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inUserData);

	/* A monitor changed its position and/or size so re-emit the signal */
	g_signal_emit_by_name(self, "monitor-geometry-changed", monitor);
}

/* Create a monitor object */
static EsdashboardWindowTrackerMonitorX11* _esdashboard_window_tracker_x11_monitor_new(EsdashboardWindowTrackerX11 *self,
																						guint inMonitorIndex)
{
	EsdashboardWindowTrackerX11Private		*priv;
	EsdashboardWindowTrackerMonitorX11		*monitor;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(inMonitorIndex>=g_list_length(self->priv->monitors), NULL);

	priv=self->priv;

	/* Create monitor object */
	monitor=ESDASHBOARD_WINDOW_TRACKER_MONITOR_X11(g_object_new(ESDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_X11,
															"monitor-index", inMonitorIndex,
															NULL));
	priv->monitors=g_list_append(priv->monitors, monitor);

	/* Connect signals */
	g_signal_connect_swapped(monitor,
								"primary-changed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_primary_monitor_changed),
								self);
	g_signal_connect_swapped(monitor,
								"geometry-changed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_monitor_geometry_changed),
								self);

	/* Emit signal */
	g_signal_emit_by_name(self, "monitor-added", monitor);
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Monitor %d added",
						inMonitorIndex);

	/* If we newly added monitor is the primary one then emit signal. We could not
	 * have done it yet because the signals were connect to new monitor object
	 * after its creation.
	 */
	if(esdashboard_window_tracker_monitor_is_primary(ESDASHBOARD_WINDOW_TRACKER_MONITOR(monitor)))
	{
		_esdashboard_window_tracker_x11_on_primary_monitor_changed(self, monitor);
	}

	/* Return newly created monitor */
	return(monitor);
}

/* Free a monitor object */
static void _esdashboard_window_tracker_x11_monitor_free(EsdashboardWindowTrackerX11 *self,
															EsdashboardWindowTrackerMonitorX11 *inMonitor)
{
	EsdashboardWindowTrackerX11Private		*priv;
	GList									*iter;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inMonitor));

	priv=self->priv;

	/* Find monitor to free */
	iter=g_list_find(priv->monitors,  inMonitor);
	if(!iter)
	{
		g_critical("Cannot release unknown monitor %d",
					esdashboard_window_tracker_monitor_get_number(ESDASHBOARD_WINDOW_TRACKER_MONITOR(inMonitor)));
		return;
	}

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_data(inMonitor, self);

	/* Emit signal */
	g_signal_emit_by_name(self, "monitor-removed", inMonitor);
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Monitor %d removed",
						esdashboard_window_tracker_monitor_get_number(ESDASHBOARD_WINDOW_TRACKER_MONITOR(inMonitor)));

	/* Remove monitor object from list */
	priv->monitors=g_list_delete_link(priv->monitors, iter);

	/* Unref monitor object. Usually this is the last reference released
	 * and the object will be destroyed.
	 */
	g_object_unref(inMonitor);
}

#ifdef HAVE_XINERAMA
/* Number of monitors, primary monitor or size of any monitor changed */
static void _esdashboard_window_tracker_x11_on_monitors_changed(EsdashboardWindowTrackerX11 *self,
																gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;
	GdkScreen								*screen;
	gint									currentMonitorCount;
	gint									newMonitorCount;
	gint									i;
	EsdashboardWindowTrackerMonitorX11		*monitor;
	GList									*iter;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(GDK_IS_SCREEN(inUserData));

	priv=self->priv;
	screen=GDK_SCREEN(inUserData);

	/* Get current monitor states */
	currentMonitorCount=g_list_length(priv->monitors);

	/* Get new monitor state */
#if GTK_CHECK_VERSION(3, 22, 0)
	newMonitorCount=gdk_display_get_n_monitors(gdk_screen_get_display(screen));
#else
	newMonitorCount=gdk_screen_get_n_monitors(screen);
#endif
	if(newMonitorCount!=currentMonitorCount)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Number of monitors changed from %d to %d",
							currentMonitorCount,
							newMonitorCount);
	}

	/* There is no need to check if size of any monitor has changed because
	 * EsdashboardWindowTrackerMonitor instances should also be connected to
	 * this signal and will raise a signal if their size changed. This instance
	 * is connected to this "monitor-has-changed-signal' and will re-emit them.
	 * The same is with primary monitor.
	 */

	/* If number of monitors has increased create newly added monitors */
	if(newMonitorCount>currentMonitorCount)
	{
		for(i=currentMonitorCount; i<newMonitorCount; i++)
		{
			/* Create monitor object */
			_esdashboard_window_tracker_x11_monitor_new(self, i);
		}
	}

	/* If number of monitors has decreased remove all monitors beyond
	 * the new number of monitors.
	 */
	if(newMonitorCount<currentMonitorCount)
	{
		for(i=currentMonitorCount; i>newMonitorCount; i--)
		{
			/* Get monitor object */
			iter=g_list_last(priv->monitors);
			if(!iter) continue;

			monitor=ESDASHBOARD_WINDOW_TRACKER_MONITOR_X11(iter->data);

			/* Free monitor object */
			_esdashboard_window_tracker_x11_monitor_free(self, monitor);
		}
	}

#if GTK_CHECK_VERSION(3, 22, 0)
	/* Set flag to recalculate screen size which must have changed as monitors
	 * were added or removed.
	 */
	priv->needScreenSizeUpdate=TRUE;
#endif
}
#endif

/* Total size of screen changed */
static void _esdashboard_window_tracker_x11_on_screen_size_changed(EsdashboardWindowTrackerX11 *self,
																	gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;
	gint									w, h;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));

	priv=self->priv;

	/* Get new total size of screen */
#if GTK_CHECK_VERSION(3, 22, 0)
	priv->needScreenSizeUpdate=TRUE;
#endif
	esdashboard_window_tracker_get_screen_size(ESDASHBOARD_WINDOW_TRACKER(self), &w, &h);

	/* Emit signal to tell that screen size has changed */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Screen size changed to %dx%d",
						w,
						h);
	g_signal_emit_by_name(self, "screen-size-changed");
}

/* Window manager has changed */
static void _esdashboard_window_tracker_x11_on_window_manager_changed(EsdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));

	priv=self->priv;

	/* Emit signal to tell that window manager has changed */
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Window manager changed to %s",
						wnck_screen_get_window_manager_name(priv->screen));
	g_signal_emit_by_name(self, "window-manager-changed");
}

/* Suspension state of application changed */
static void _esdashboard_window_tracker_x11_on_application_suspended_changed(EsdashboardWindowTrackerX11 *self,
																				GParamSpec *inSpec,
																				gpointer inUserData)
{
	EsdashboardWindowTrackerX11Private		*priv;
	EsdashboardApplication					*app;
	GList									*iter;
	EsdashboardWindowTrackerWindow			*window;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;
	app=ESDASHBOARD_APPLICATION(inUserData);

	/* Get application suspend state */
	priv->isAppSuspended=esdashboard_application_is_suspended(app);

	/* Iterate through all windows and connect handler to signal 'geometry-changed'
	 * if application was resumed or disconnect signal handler if it was suspended.
	 */
	for(iter=esdashboard_window_tracker_get_windows(ESDASHBOARD_WINDOW_TRACKER(self)); iter; iter=g_list_next(iter))
	{
		/* Get window */
		window=ESDASHBOARD_WINDOW_TRACKER_WINDOW(iter->data);
		if(!window) continue;

		/* If application was suspended disconnect signal handlers ... */
		if(priv->isAppSuspended)
		{
			g_signal_handlers_block_by_func(window, _esdashboard_window_tracker_x11_on_window_geometry_changed, self);
		}
			/* ... otherwise if application was resumed reconnect signals handlers
			 * and emit 'geometry-changed' signal to reflect latest changes of
			 * position and size of window.
			 */
			else
			{
				/* Reconnect signal handler */
				g_signal_handlers_unblock_by_func(window, _esdashboard_window_tracker_x11_on_window_geometry_changed, self);

				/* Call signal handler to reflect latest changes */
				_esdashboard_window_tracker_x11_on_window_geometry_changed(self, window);
			}
	}
}


/* IMPLEMENTATION: Interface EsdashboardWindowTracker */

/* Get list of all windows (if wanted in stack order) */
static GList* _esdashboard_window_tracker_x11_window_tracker_get_windows(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return list of window objects created */
	return(priv->windows);
}

/* Get list of all windows in stack order */
static GList* _esdashboard_window_tracker_x11_window_tracker_get_windows_stacked(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return list of window in stack order */
	return(priv->windowsStacked);
}

/* Get active window */
static EsdashboardWindowTrackerWindow* _esdashboard_window_tracker_x11_window_tracker_get_active_window(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	return(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->activeWindow));
}

/* Get number of workspaces */
static gint _esdashboard_window_tracker_x11_window_tracker_get_workspaces_count(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), 0);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return number of workspaces */
	return(wnck_screen_get_workspace_count(priv->screen));
}

/* Get list of workspaces */
static GList* _esdashboard_window_tracker_x11_window_tracker_get_workspaces(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return list of workspaces */
	return(priv->workspaces);
}

/* Get active workspace */
static EsdashboardWindowTrackerWorkspace* _esdashboard_window_tracker_x11_window_tracker_get_active_workspace(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	return(ESDASHBOARD_WINDOW_TRACKER_WORKSPACE(priv->activeWorkspace));
}

/* Get workspace by number */
static EsdashboardWindowTrackerWorkspace* _esdashboard_window_tracker_x11_window_tracker_get_workspace_by_number(EsdashboardWindowTracker *inWindowTracker,
																													gint inNumber)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;
	WnckWorkspace							*wnckWorkspace;
	EsdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	g_return_val_if_fail(inNumber>=0 && inNumber<wnck_screen_get_workspace_count(priv->screen), NULL);

	/* Get wnck workspace by number */
	wnckWorkspace=wnck_screen_get_workspace(priv->screen, inNumber);

	/* Get workspace object for wnck workspace */
	workspace=_esdashboard_window_tracker_x11_get_workspace_for_wnck(self, wnckWorkspace);
	if(!workspace)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"No workspace object of type %s found for wnck workspace %s@%p named '%s'",
							g_type_name(ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
							G_OBJECT_TYPE_NAME(wnckWorkspace), wnckWorkspace, wnck_workspace_get_name(wnckWorkspace));

		return(NULL);
	}

	/* Return workspace object */
	return(ESDASHBOARD_WINDOW_TRACKER_WORKSPACE(workspace));
}

/* Determine if multiple monitors are supported */
static gboolean _esdashboard_window_tracker_x11_window_tracker_supports_multiple_monitors(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), FALSE);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	return(priv->supportsMultipleMonitors);
}

/* Get number of monitors */
static gint _esdashboard_window_tracker_x11_window_tracker_get_monitors_count(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), 0);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return number of monitors */
	return(g_list_length(priv->monitors));
}

/* Get list of monitors */
static GList* _esdashboard_window_tracker_x11_window_tracker_get_monitors(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return list of workspaces */
	return(priv->monitors);
}

/* Get primary monitor */
static EsdashboardWindowTrackerMonitor* _esdashboard_window_tracker_x11_window_tracker_get_primary_monitor(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	return(ESDASHBOARD_WINDOW_TRACKER_MONITOR(priv->primaryMonitor));
}

/* Get monitor by number */
static EsdashboardWindowTrackerMonitor* _esdashboard_window_tracker_x11_window_tracker_get_monitor_by_number(EsdashboardWindowTracker *inWindowTracker,
																												gint inNumber)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	g_return_val_if_fail(inNumber>=0, NULL);
	g_return_val_if_fail(((guint)inNumber)<g_list_length(priv->monitors), NULL);

	/* Return monitor at index */
	return(ESDASHBOARD_WINDOW_TRACKER_MONITOR(g_list_nth_data(priv->monitors, inNumber)));
}

/* Get monitor at requested position */
static EsdashboardWindowTrackerMonitor* _esdashboard_window_tracker_x11_window_tracker_get_monitor_by_position(EsdashboardWindowTracker *inWindowTracker,
																												gint inX,
																												gint inY)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;
	GList									*iter;
	EsdashboardWindowTrackerMonitorX11		*monitor;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Iterate through monitors and return the one containing the requested position */
	for(iter=priv->monitors; iter; iter=g_list_next(iter))
	{
		/* Get monitor at iterator */
		monitor=ESDASHBOARD_WINDOW_TRACKER_MONITOR_X11(iter->data);
		if(!monitor) continue;

		/* Check if this monitor contains the requested position. If it does
		 * then return it.
		 */
		if(esdashboard_window_tracker_monitor_contains(ESDASHBOARD_WINDOW_TRACKER_MONITOR(monitor), inX, inY))
		{
			return(ESDASHBOARD_WINDOW_TRACKER_MONITOR(monitor));
		}
	}

	/* If we get here none of the monitors contains the requested position,
	 * so return NULL here.
	 */
	return(NULL);
}

/* Get size of screen */
static void _esdashboard_window_tracker_x11_window_tracker_get_screen_size(EsdashboardWindowTracker *inWindowTracker,
																			gint *outWidth,
																			gint *outHeight)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;
	gint									width, height;
#if GTK_CHECK_VERSION(3, 22, 0)
	gint									i;
	gint									numberMonitors;
	GdkMonitor								*monitor;
	GdkRectangle							monitorRect;
	gint									left, top, right, bottom;
	gboolean								forceUpdate;
#endif

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker));

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

#if GTK_CHECK_VERSION(3, 22, 0)
	/* Only recalculate screen size if flag is set */
	if(priv->needScreenSizeUpdate)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS, "Screen size needs to be recalculated");

		/* Get width and height of screen by iterating through all connected monitors
		 * and find the most top left and most right bottom point among all these
		 * monitors. Then calculate size of screen by these points.
		 */
		forceUpdate=TRUE;
		left=top=right=bottom=0;
		numberMonitors=gdk_display_get_n_monitors(priv->gdkDisplay);
		for(i=0; i<numberMonitors; i++)
		{
			monitor=gdk_display_get_monitor(priv->gdkDisplay, i);
			gdk_monitor_get_geometry(monitor, &monitorRect);

			if(forceUpdate || monitorRect.x<left) left=monitorRect.x;
			if(forceUpdate || monitorRect.y<top) top=monitorRect.y;
			if(forceUpdate || (monitorRect.x+monitorRect.width)>right) right=monitorRect.x+monitorRect.width;
			if(forceUpdate || (monitorRect.y+monitorRect.height)>bottom) bottom=monitorRect.y+monitorRect.height;

			/* The first monitor in list was processed so do not enforcing updating
			 * coordinates from now on except they are most-specific points.
			 */
			forceUpdate=FALSE;

			/* Debug message */
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"Iterating monitor %d of %d [%d,%dx%d,%d] for screen size calculation",
								i, numberMonitors,
								monitorRect.x, monitorRect.y,
								monitorRect.width, monitorRect.height);
		}

		/* Calculate screen size */
		priv->screenWidth=right-left;
		priv->screenHeight=bottom-top;
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Screen size is %dx%d over all %d monitors covering area of [%d,%dx%d,%d]",
							priv->screenWidth, priv->screenHeight,
							numberMonitors,
							left, top,
							right, bottom);

		/* Reset flag to avoid recalculation of screen size again and again */
		priv->needScreenSizeUpdate=FALSE;
	}

	/* Get width and height of screen */
	width=priv->screenWidth;
	height=priv->screenHeight;
#else
	/* Get width and height of screen */
	width=gdk_screen_get_width(priv->gdkScreen);
	height=gdk_screen_get_height(priv->gdkScreen);
#endif

	/* Store result */
	if(outWidth) *outWidth=width;
	if(outHeight) *outHeight=height;
}

/* Get window manager name managing desktop environment */
static const gchar* _esdashboard_window_tracker_x11_window_tracker_get_window_manager_name(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Get window manager name from libwnck and return */
	return(wnck_screen_get_window_manager_name(priv->screen));

}

/* Get root (desktop) window */
static EsdashboardWindowTrackerWindow* _esdashboard_window_tracker_x11_window_tracker_get_root_window(EsdashboardWindowTracker *inWindowTracker)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerX11Private		*priv;
	gulong									backgroundWindowID;
	GList									*windows;
	EsdashboardWindowTrackerWindowX11		*window;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Find and return root window (the desktop) by known ID */
	backgroundWindowID=wnck_screen_get_background_pixmap(priv->screen);
	if(backgroundWindowID)
	{
		WnckWindow							*backgroundWindow;

		backgroundWindow=wnck_window_get(backgroundWindowID);
		if(backgroundWindow)
		{
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"Found desktop window %s@%p by known background pixmap ID",
								G_OBJECT_TYPE_NAME(backgroundWindow), backgroundWindow);

			/* Get or create window object for wnck background window */
			window=_esdashboard_window_tracker_x11_create_window_for_wnck(self, backgroundWindow);
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"Resolved desktop window %s@%p to window object %s@%p",
								G_OBJECT_TYPE_NAME(backgroundWindow), backgroundWindow,
								G_OBJECT_TYPE_NAME(window), window);

			/* Return window object found or created */
			return(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window));
		}
	}

	/* Either there was no known ID for the root window or the root window
	 * could not be found (happened a lot when running in daemon mode).
	 * So iterate through list of all known windows and lookup window of
	 * type 'desktop'.
	 */
	for(windows=wnck_screen_get_windows(priv->screen); windows; windows=g_list_next(windows))
	{
		WnckWindow					*wnckWindow;
		WnckWindowType				wnckWindowType;

		wnckWindow=(WnckWindow*)windows->data;
		wnckWindowType=wnck_window_get_window_type(wnckWindow);
		if(wnckWindowType==WNCK_WINDOW_DESKTOP)
		{
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"Desktop window %s@%p found while iterating through window list",
								G_OBJECT_TYPE_NAME(wnckWindow), wnckWindow);

			/* Get or create window object for wnck background window */
			window=_esdashboard_window_tracker_x11_create_window_for_wnck(self, wnckWindow);
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"Resolved desktop window %s@%p to window object %s@%p",
								G_OBJECT_TYPE_NAME(wnckWindow), wnckWindow,
								G_OBJECT_TYPE_NAME(window), window);

			/* Return window object found or created */
			return(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window));
		}
	}

	/* If we get here either desktop window does not exist or is not known
	 * in window list. So return NULL here.
	 */
	ESDASHBOARD_DEBUG(self, WINDOWS, "Desktop window could not be found");
	return(NULL);
}

/* Interface initialization
 * Set up default functions
 */
static void _esdashboard_window_tracker_x11_window_tracker_iface_init(EsdashboardWindowTrackerInterface *iface)
{
	iface->get_windows=_esdashboard_window_tracker_x11_window_tracker_get_windows;
	iface->get_windows_stacked=_esdashboard_window_tracker_x11_window_tracker_get_windows_stacked;
	iface->get_active_window=_esdashboard_window_tracker_x11_window_tracker_get_active_window;

	iface->get_workspaces_count=_esdashboard_window_tracker_x11_window_tracker_get_workspaces_count;
	iface->get_workspaces=_esdashboard_window_tracker_x11_window_tracker_get_workspaces;
	iface->get_active_workspace=_esdashboard_window_tracker_x11_window_tracker_get_active_workspace;
	iface->get_workspace_by_number=_esdashboard_window_tracker_x11_window_tracker_get_workspace_by_number;

	iface->supports_multiple_monitors=_esdashboard_window_tracker_x11_window_tracker_supports_multiple_monitors;
	iface->get_monitors_count=_esdashboard_window_tracker_x11_window_tracker_get_monitors_count;
	iface->get_monitors=_esdashboard_window_tracker_x11_window_tracker_get_monitors;
	iface->get_primary_monitor=_esdashboard_window_tracker_x11_window_tracker_get_primary_monitor;
	iface->get_monitor_by_number=_esdashboard_window_tracker_x11_window_tracker_get_monitor_by_number;
	iface->get_monitor_by_position=_esdashboard_window_tracker_x11_window_tracker_get_monitor_by_position;

	iface->get_screen_size=_esdashboard_window_tracker_x11_window_tracker_get_screen_size;

	iface->get_window_manager_name=_esdashboard_window_tracker_x11_window_tracker_get_window_manager_name;

	iface->get_root_window=_esdashboard_window_tracker_x11_window_tracker_get_root_window;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_window_tracker_x11_dispose_free_window(gpointer inData,
																gpointer inUserData)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inData));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inUserData));

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inUserData);
	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inData);

	/* Unreference window */
	_esdashboard_window_tracker_x11_free_window(self, window);
}

static void _esdashboard_window_tracker_x11_dispose_free_workspace(gpointer inData,
																	gpointer inUserData)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inData));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inUserData));

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inUserData);
	workspace=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inData);

	/* Unreference workspace */
	_esdashboard_window_tracker_x11_free_workspace(self, workspace);
}

static void _esdashboard_window_tracker_x11_dispose_free_monitor(gpointer inData,
																	gpointer inUserData)
{
	EsdashboardWindowTrackerX11				*self;
	EsdashboardWindowTrackerMonitorX11		*monitor;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inData));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(inUserData));

	self=ESDASHBOARD_WINDOW_TRACKER_X11(inUserData);
	monitor=ESDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inData);

	/* Unreference monitor */
	_esdashboard_window_tracker_x11_monitor_free(self, monitor);
}

static void _esdashboard_window_tracker_x11_dispose(GObject *inObject)
{
	EsdashboardWindowTrackerX11				*self=ESDASHBOARD_WINDOW_TRACKER_X11(inObject);
	EsdashboardWindowTrackerX11Private		*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->suspendSignalID)
	{
		if(priv->application)
		{
			g_signal_handler_disconnect(priv->application, priv->suspendSignalID);
			priv->application=NULL;
		}

		priv->suspendSignalID=0;
	}

	if(priv->activeWindow)
	{
		priv->activeWindow=NULL;
	}

	if(priv->windows)
	{
		g_list_foreach(priv->windows, _esdashboard_window_tracker_x11_dispose_free_window, self);
		g_list_free(priv->windows);
		priv->windows=NULL;
	}

	if(priv->windowsStacked)
	{
		g_list_free(priv->windowsStacked);
		priv->windowsStacked=NULL;
	}

	if(priv->activeWorkspace)
	{
		priv->activeWorkspace=NULL;
	}

	if(priv->workspaces)
	{
		g_list_foreach(priv->workspaces, _esdashboard_window_tracker_x11_dispose_free_workspace, self);
		g_list_free(priv->workspaces);
		priv->workspaces=NULL;
	}

	if(priv->primaryMonitor)
	{
		priv->primaryMonitor=NULL;
	}

	if(priv->monitors)
	{
		g_list_foreach(priv->monitors, _esdashboard_window_tracker_x11_dispose_free_monitor, self);
		g_list_free(priv->monitors);
		priv->monitors=NULL;
	}

	if(priv->gdkScreen)
	{
		g_signal_handlers_disconnect_by_data(priv->gdkScreen, self);
		priv->gdkScreen=NULL;
	}

#if GTK_CHECK_VERSION(3, 22, 0)
	if(priv->gdkDisplay)
	{
		g_signal_handlers_disconnect_by_data(priv->gdkDisplay, self);
		priv->gdkDisplay=NULL;
	}
#endif

	if(priv->screen)
	{
		g_signal_handlers_disconnect_by_data(priv->screen, self);
		priv->screen=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_window_tracker_x11_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_window_tracker_x11_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	switch(inPropID)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_window_tracker_x11_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	EsdashboardWindowTrackerX11			*self=ESDASHBOARD_WINDOW_TRACKER_X11(inObject);

	switch(inPropID)
	{
		case PROP_ACTIVE_WINDOW:
			g_value_set_object(outValue, self->priv->activeWindow);
			break;

		case PROP_ACTIVE_WORKSPACE:
			g_value_set_object(outValue, self->priv->activeWorkspace);
			break;

		case PROP_PRIMARY_MONITOR:
			g_value_set_object(outValue, self->priv->primaryMonitor);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_window_tracker_x11_class_init(EsdashboardWindowTrackerX11Class *klass)
{
	GObjectClass						*gobjectClass=G_OBJECT_CLASS(klass);
	EsdashboardWindowTracker			*trackerIface;
	GParamSpec							*paramSpec;

	/* Reference interface type to lookup properties etc. */
	trackerIface=g_type_default_interface_ref(ESDASHBOARD_TYPE_WINDOW_TRACKER);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_window_tracker_x11_dispose;
	gobjectClass->set_property=_esdashboard_window_tracker_x11_set_property;
	gobjectClass->get_property=_esdashboard_window_tracker_x11_get_property;

	/* Define properties */
	paramSpec=g_object_interface_find_property(trackerIface, "active-window");
	EsdashboardWindowTrackerX11Properties[PROP_ACTIVE_WINDOW]=
		g_param_spec_override("active-window", paramSpec);

	paramSpec=g_object_interface_find_property(trackerIface, "active-workspace");
	EsdashboardWindowTrackerX11Properties[PROP_ACTIVE_WORKSPACE]=
		g_param_spec_override("active-workspace", paramSpec);

	paramSpec=g_object_interface_find_property(trackerIface, "primary-monitor");
	EsdashboardWindowTrackerX11Properties[PROP_PRIMARY_MONITOR]=
		g_param_spec_override("primary-monitor", paramSpec);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardWindowTrackerX11Properties);

	/* Release allocated resources */
	g_type_default_interface_unref(trackerIface);
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_window_tracker_x11_init(EsdashboardWindowTrackerX11 *self)
{
	EsdashboardWindowTrackerX11Private		*priv;

	priv=self->priv=esdashboard_window_tracker_x11_get_instance_private(self);

	ESDASHBOARD_DEBUG(self, WINDOWS, "Initializing X11 window tracker");

	/* Set default values */
	priv->windows=NULL;
	priv->windowsStacked=NULL;
	priv->workspaces=NULL;
	priv->monitors=NULL;
	priv->screen=wnck_screen_get_default();
#if GTK_CHECK_VERSION(3, 22, 0)
	priv->gdkDisplay=gdk_display_get_default();
	priv->gdkScreen=gdk_display_get_default_screen(priv->gdkDisplay);
	priv->needScreenSizeUpdate=TRUE;
	priv->screenWidth=0;
	priv->screenHeight=0;
#else
	priv->gdkScreen=gdk_screen_get_default();
#endif
	priv->activeWindow=NULL;
	priv->activeWorkspace=NULL;
	priv->primaryMonitor=NULL;
	priv->supportsMultipleMonitors=FALSE;

	/* The very first call to libwnck should be setting the client type */
	wnck_set_client_type(WNCK_CLIENT_TYPE_PAGER);

	/* Connect signals to screen */
	g_signal_connect_swapped(priv->screen,
								"window-stacking-changed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_window_stacking_changed),
								self);

	g_signal_connect_swapped(priv->screen,
								"window-closed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_window_closed),
								self);
	g_signal_connect_swapped(priv->screen,
								"window-opened",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_window_opened),
								self);
	g_signal_connect_swapped(priv->screen,
								"active-window-changed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_active_window_changed),
								self);

	g_signal_connect_swapped(priv->screen,
								"workspace-destroyed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_workspace_destroyed),
								self);
	g_signal_connect_swapped(priv->screen,
								"workspace-created",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_workspace_created),
								self);
	g_signal_connect_swapped(priv->screen,
								"active-workspace-changed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_active_workspace_changed),
								self);

	g_signal_connect_swapped(priv->gdkScreen,
								"size-changed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_screen_size_changed),
								self);

	g_signal_connect_swapped(priv->screen,
								"window-manager-changed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_window_manager_changed),
								self);

#ifdef HAVE_XINERAMA
	/* Check if multiple monitors are supported */
	if(XineramaIsActive(GDK_SCREEN_XDISPLAY(priv->gdkScreen)))
	{
		EsdashboardWindowTrackerMonitorX11	*monitor;
		gint								numberMonitors;
		gint								i;

		/* Set flag that multiple monitors are supported */
		priv->supportsMultipleMonitors=TRUE;

		/* This signal must be called at least after the default signal handler - best at last.
		 * The reason is that all other signal handler should been processed because this handler
		 * could destroy monitor instances even the one of the primary monitor if it was not
		 * handled before. So give the other signal handlers a chance ;)
		 */
		g_signal_connect_data(priv->gdkScreen,
								"monitors-changed",
								G_CALLBACK(_esdashboard_window_tracker_x11_on_monitors_changed),
								self,
								NULL,
								G_CONNECT_AFTER | G_CONNECT_SWAPPED);

		/* Get monitors */
#if GTK_CHECK_VERSION(3, 22, 0)
		numberMonitors=gdk_display_get_n_monitors(priv->gdkDisplay);
#else
		numberMonitors=gdk_screen_get_n_monitors(priv->gdkScreen);
#endif
		for(i=0; i<numberMonitors; i++)
		{
			/* Create monitor object */
			monitor=_esdashboard_window_tracker_x11_monitor_new(self, i);

			/* Remember primary monitor */
			if(esdashboard_window_tracker_monitor_is_primary(ESDASHBOARD_WINDOW_TRACKER_MONITOR(monitor)))
			{
				priv->primaryMonitor=monitor;
			}
		}
	}
#endif

	/* Handle suspension signals from application */
	priv->application=esdashboard_application_get_default();
	priv->suspendSignalID=g_signal_connect_swapped(priv->application,
													"notify::is-suspended",
													G_CALLBACK(_esdashboard_window_tracker_x11_on_application_suspended_changed),
													self);
	priv->isAppSuspended=esdashboard_application_is_suspended(priv->application);
}


/* IMPLEMENTATION: Public API */

/* Get last timestamp for use in libwnck */
guint32 esdashboard_window_tracker_x11_get_time(void)
{
	const ClutterEvent		*currentClutterEvent;
	guint32					timestamp;
	GdkDisplay				*display;

	/* We don't use clutter_get_current_event_time as it can return
	 * a too old timestamp if there is no current event.
	 */
	currentClutterEvent=clutter_get_current_event();
	if(currentClutterEvent!=NULL) return(clutter_event_get_time(currentClutterEvent));

	/* Next we try timestamp of last GTK+ event */
	timestamp=gtk_get_current_event_time();
	if(timestamp>0) return(timestamp);

	/* Next we try to ask GDK for a timestamp */
	display=gdk_display_get_default();
	if(display)
	{
		timestamp=gdk_x11_display_get_user_time(display);
		if(timestamp>0) return(timestamp);
	}

#ifdef CLUTTER_WINDOWING_X11
	/* Next we try to get timestamp via Clutter X11 backend if available and used */
	if(clutter_check_windowing_backend(CLUTTER_WINDOWING_X11))
	{
		GdkWindow			*window;
		GdkEventMask		eventMask;
		GSList				*stages;
		GSList				*iter;
		ClutterStage		*stage;

		/* Next we try to retrieve timestamp of last X11 event in clutter */
		ESDASHBOARD_DEBUG(NULL, WINDOWS, "No timestamp for windows - trying timestamp of last X11 event in Clutter");
		timestamp=(guint32)clutter_x11_get_current_event_time();
		if(timestamp!=0)
		{
			ESDASHBOARD_DEBUG(NULL, WINDOWS,
								"Got timestamp %u of last X11 event in Clutter",
								timestamp);
			return(timestamp);
		}

		/* Last resort is to get X11 server time via stage windows, so iterate
		 * through stages, get their GDK-X11 window and try to retrieve timestamp.
		 */
		ESDASHBOARD_DEBUG(NULL, WINDOWS, "No timestamp for windows - trying last resort via X11 stage windows");
		if(!display)
		{
			ESDASHBOARD_DEBUG(NULL, WINDOWS, "No default X11 display found in GDK to get timestamp for windows");
			return(0);
		}

		timestamp=0;
		stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
		for(iter=stages; timestamp==0 && iter; iter=g_slist_next(iter))
		{
			/* Get stage */
			stage=CLUTTER_STAGE(iter->data);
			if(stage)
			{
				/* Get GDK window of stage */
				window=gdk_x11_window_lookup_for_display(display, clutter_x11_get_stage_window(stage));
				if(!window)
				{
					ESDASHBOARD_DEBUG(NULL, WINDOWS,
										"No GDK-X11 window found for stage %s@%p to get timestamp for windows",
										G_OBJECT_TYPE_NAME(stage),
										stage);
					continue;
				}

				/* Check if GDK window supports GDK_PROPERTY_CHANGE_MASK event
				 * or application (or worst X server) will hang
				 */
				eventMask=gdk_window_get_events(window);
				if(!(eventMask & GDK_PROPERTY_CHANGE_MASK))
				{
					ESDASHBOARD_DEBUG(NULL, WINDOWS,
										"GDK-X11 window %p for stage %s@%p does not support GDK_PROPERTY_CHANGE_MASK to get timestamp for windows",
										window,
										G_OBJECT_TYPE_NAME(stage),
										stage);
					continue;
				}

				timestamp=gdk_x11_get_server_time(window);
			}
		}
		g_slist_free(stages);
	}
#endif

#ifdef CLUTTER_WINDOWING_GDK
	/* Next we try to get timestamp via Clutter GDK backend if available and used */
	if(clutter_check_windowing_backend(CLUTTER_WINDOWING_GDK))
	{
		GdkWindow			*window;
		GdkEventMask		eventMask;
		GSList				*stages;
		GSList				*iter;
		ClutterStage		*stage;

		/* Iterate through stages, get their GDK-X11 window and try to retrieve timestamp */
		timestamp=0;
		stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
		for(iter=stages; timestamp==0 && iter; iter=g_slist_next(iter))
		{
			/* Get stage */
			stage=CLUTTER_STAGE(iter->data);
			if(stage)
			{
				/* Get GDK window of stage */
				window=clutter_gdk_get_stage_window(stage);
				if(!window)
				{
					ESDASHBOARD_DEBUG(NULL, WINDOWS,
										"No GDK-X11 window found for stage %s@%p to get timestamp for windows",
										G_OBJECT_TYPE_NAME(stage),
										stage);
					continue;
				}

				/* Check if GDK window supports GDK_PROPERTY_CHANGE_MASK event
				 * or application (or worst X server) will hang
				 */
				eventMask=gdk_window_get_events(window);
				if(!(eventMask & GDK_PROPERTY_CHANGE_MASK))
				{
					ESDASHBOARD_DEBUG(NULL, WINDOWS,
										"GDK-X11 window %p for stage %s@%p does not support GDK_PROPERTY_CHANGE_MASK to get timestamp for windows",
										window,
										G_OBJECT_TYPE_NAME(stage),
										stage);
					continue;
				}

				timestamp=gdk_x11_get_server_time(window);
			}
		}
		g_slist_free(stages);
	}
#endif

	/* Return timestamp of last resort */
	ESDASHBOARD_DEBUG(NULL, WINDOWS,
						"Last resort timestamp for windows %s (%u)",
						timestamp ? "found" : "not found",
						timestamp);
	return(timestamp);
}

/* Find and return EsdashboardWindowTrackerWindow object for mapped wnck window */
EsdashboardWindowTrackerWindow* esdashboard_window_tracker_x11_get_window_for_wnck(EsdashboardWindowTrackerX11 *self,
																					WnckWindow *inWindow)
{
	EsdashboardWindowTrackerWindowX11		*window;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	/* Lookup window object for requested wnck window and return it */
	window=_esdashboard_window_tracker_x11_get_window_for_wnck(self, inWindow);
	return(ESDASHBOARD_WINDOW_TRACKER_WINDOW(window));
}

/* Find and return EsdashboardWindowTrackerWorkspace object for mapped wnck workspace */
EsdashboardWindowTrackerWorkspace* esdashboard_window_tracker_x11_get_workspace_for_wnck(EsdashboardWindowTrackerX11 *self,
																							WnckWorkspace *inWorkspace)
{
	EsdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), NULL);

	/* Lookup workspace object for requested wnck workspace and return it */
	workspace=_esdashboard_window_tracker_x11_get_workspace_for_wnck(self, inWorkspace);
	return(ESDASHBOARD_WINDOW_TRACKER_WORKSPACE(workspace));
}
