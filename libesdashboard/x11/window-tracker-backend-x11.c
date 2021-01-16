/*
 * window-tracker-backend: Window tracker backend providing special functions
 *                         for different windowing and clutter backends.
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

#include <libesdashboard/x11/window-tracker-backend-x11.h>

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include <libesdashboard/x11/window-tracker-x11.h>
#include <libesdashboard/x11/window-tracker-window-x11.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
static void _esdashboard_window_tracker_backend_x11_window_tracker_backend_iface_init(EsdashboardWindowTrackerBackendInterface *iface);

struct _EsdashboardWindowTrackerBackendX11Private
{
	/* Instance related */
	EsdashboardWindowTrackerX11		*windowTracker;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardWindowTrackerBackendX11,
						esdashboard_window_tracker_backend_x11,
						G_TYPE_OBJECT,
						G_ADD_PRIVATE(EsdashboardWindowTrackerBackendX11)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND, _esdashboard_window_tracker_backend_x11_window_tracker_backend_iface_init))

/* IMPLEMENTATION: Private variables and methods */

/* State of stage window changed */
static void _esdashboard_window_tracker_backend_x11_on_stage_state_changed(WnckWindow *inWindow,
																			WnckWindowState inChangedMask,
																			WnckWindowState inNewValue,
																			gpointer inUserData)
{
	EsdashboardWindowTrackerWindowX11			*stageWindow;

	g_return_if_fail(WNCK_IS_WINDOW(inWindow));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	stageWindow=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Set 'skip-tasklist' if changed */
	if((inChangedMask & WNCK_WINDOW_STATE_SKIP_TASKLIST) &&
		!(inNewValue & WNCK_WINDOW_STATE_SKIP_TASKLIST))
	{
		wnck_window_set_skip_tasklist(WNCK_WINDOW(inWindow), TRUE);
		ESDASHBOARD_DEBUG(inWindow, WINDOWS,
							"State 'skip-tasklist' for stage window %p (wnck-window=%p) needs reset",
							stageWindow,
							inWindow);
	}

	/* Set 'skip-pager' if changed */
	if((inChangedMask & WNCK_WINDOW_STATE_SKIP_PAGER) &&
		!(inNewValue & WNCK_WINDOW_STATE_SKIP_PAGER))
	{
		wnck_window_set_skip_pager(WNCK_WINDOW(inWindow), TRUE);
		ESDASHBOARD_DEBUG(inWindow, WINDOWS,
							"State 'skip-pager' for stage window %p (wnck-window=%p) needs reset",
							stageWindow,
							inWindow);
	}

	/* Set 'make-above' if changed */
	if((inChangedMask & WNCK_WINDOW_STATE_ABOVE) &&
		!(inNewValue & WNCK_WINDOW_STATE_ABOVE))
	{
		wnck_window_make_above(WNCK_WINDOW(inWindow));
		ESDASHBOARD_DEBUG(inWindow, WINDOWS,
							"State 'make-above' for stage window %p (wnck-window=%p) needs reset",
							stageWindow,
							inWindow);
	}
}

/* The active window changed. Reselect stage window as active one if it is visible */
static void _esdashboard_window_tracker_backend_x11_on_stage_active_window_changed(WnckScreen *inScreen,
																					WnckWindow *inPreviousWindow,
																					gpointer inUserData)
{
	EsdashboardWindowTrackerWindowX11			*stageWindow;
	EsdashboardWindowTrackerWindowState			stageWindowState;
	WnckWindow									*stageWnckWindow;
	WnckWindow									*activeWindow;
	gboolean									reselect;

	g_return_if_fail(WNCK_IS_SCREEN(inScreen));
	g_return_if_fail(inPreviousWindow==NULL || WNCK_IS_WINDOW(inPreviousWindow));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	stageWindow=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);
	reselect=FALSE;

	/* Get wnck window of stage window */
	stageWnckWindow=esdashboard_window_tracker_window_x11_get_window(stageWindow);
	if(!stageWnckWindow)
	{
		g_critical("Could not get real stage window to handle signal 'active-window-changed'");
		return;
	}

	/* Get stage of stage window */
	stageWindowState=esdashboard_window_tracker_window_get_state(ESDASHBOARD_WINDOW_TRACKER_WINDOW(stageWindow));

	/* Reactive stage window if not hidden */
	activeWindow=wnck_screen_get_active_window(inScreen);

	if(inPreviousWindow && inPreviousWindow==stageWnckWindow) reselect=TRUE;
	if(!activeWindow || activeWindow!=stageWnckWindow) reselect=TRUE;
	if(!(stageWindowState & (ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED | ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN))) reselect=TRUE;

	if(reselect)
	{
		wnck_window_activate_transient(stageWnckWindow, esdashboard_window_tracker_x11_get_time());
		ESDASHBOARD_DEBUG(stageWindow, WINDOWS,
							"Active window changed from %p (%s) to %p (%s) but stage window %p (wnck-window=%p) is visible and should be active one",
							inPreviousWindow, inPreviousWindow ? wnck_window_get_name(inPreviousWindow) : "<nil>",
							activeWindow, activeWindow ? wnck_window_get_name(activeWindow) : "<nil>",
							stageWindow,
							stageWnckWindow);
	}
}

/* Size of screen has changed so resize stage window */
static void _esdashboard_window_tracker_backend_x11_on_stage_screen_size_changed(EsdashboardWindowTracker *inWindowTracker,
																					gint inWidth,
																					gint inHeight,
																					gpointer inUserData)
{
#ifdef HAVE_XINERAMA
	EsdashboardWindowTrackerWindowX11	*realStageWindow;
	WnckWindow							*stageWindow;
	GdkDisplay							*display;
	GdkScreen							*screen;
	XineramaScreenInfo					*monitors;
	int									monitorsCount;
	gint								top, bottom, left, right;
	gint								topIndex, bottomIndex, leftIndex, rightIndex;
	gint								i;
	Atom								atomFullscreenMonitors;
	XEvent								xEvent;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(inWindowTracker));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	realStageWindow=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	ESDASHBOARD_DEBUG(inWindowTracker, WINDOWS, "Set fullscreen across all monitors using Xinerama");

	/* Get wnck window for stage window object as it is needed a lot from this
	 * point on.
	 */
	stageWindow=esdashboard_window_tracker_window_x11_get_window(realStageWindow);

	/* If window manager does not support fullscreen across all monitors
	 * return here.
	 */
	if(!wnck_screen_net_wm_supports(wnck_window_get_screen(stageWindow), "_NET_WM_FULLSCREEN_MONITORS"))
	{
		g_warning("Keep window fullscreen on primary monitor because window manager does not support _NET_WM_FULLSCREEN_MONITORS.");
		return;
	}

	/* Get display */
	display=gdk_display_get_default();

	/* Get screen */
	screen=gdk_screen_get_default();

	/* Check if Xinerama is active on display. If not try to move and resize
	 * stage window to primary monitor.
	 */
	if(!XineramaIsActive(gdk_x11_display_get_xdisplay(display)))
	{
#if GTK_CHECK_VERSION(3, 22, 0)
		GdkMonitor			*primaryMonitor;
#else
		gint				primaryMonitor;
#endif
		GdkRectangle		geometry;

		/* Get position and size of primary monitor and try to move and resize
		 * stage window to its position and size. Even if it fails it should
		 * resize the stage to the size of current monitor this window is
		 * fullscreened to. Tested with xfwm4.
		 */
#if GTK_CHECK_VERSION(3, 22, 0)
		primaryMonitor=gdk_display_get_primary_monitor(gdk_screen_get_display(screen));
		gdk_monitor_get_geometry(primaryMonitor, &geometry);
#else
		primaryMonitor=gdk_screen_get_primary_monitor(screen);
		gdk_screen_get_monitor_geometry(screen, primaryMonitor, &geometry);
#endif
		wnck_window_set_geometry(stageWindow,
									WNCK_WINDOW_GRAVITY_STATIC,
									WNCK_WINDOW_CHANGE_X | WNCK_WINDOW_CHANGE_Y | WNCK_WINDOW_CHANGE_WIDTH | WNCK_WINDOW_CHANGE_HEIGHT,
									geometry.x, geometry.y, geometry.width, geometry.height);
		return;
	}

	/* Get monitors from Xinerama */
	monitors=XineramaQueryScreens(GDK_DISPLAY_XDISPLAY(display), &monitorsCount);
	if(monitorsCount<=0 || !monitors)
	{
		if(monitors) XFree(monitors);
		return;
	}

	/* Get monitor indices for each corner of screen */
	esdashboard_window_tracker_get_screen_size(inWindowTracker, &left, &top);
	bottom=0;
	right=0;
	topIndex=bottomIndex=leftIndex=rightIndex=0;
	for(i=0; i<monitorsCount; i++)
	{
		ESDASHBOARD_DEBUG(inWindowTracker, WINDOWS,
							"Checking edges at monitor %d with upper-left at %d,%d and lower-right at %d,%d [size: %dx%d]",
							i,
							monitors[i].x_org,
							monitors[i].y_org,
							monitors[i].x_org+monitors[i].width, monitors[i].y_org+monitors[i].height,
							monitors[i].width, monitors[i].height);

		if(left>monitors[i].x_org)
		{
			left=monitors[i].x_org;
			leftIndex=i;
		}

		if(right<(monitors[i].x_org+monitors[i].width))
		{
			right=(monitors[i].x_org+monitors[i].width);
			rightIndex=i;
		}

		if(top>monitors[i].y_org)
		{
			top=monitors[i].y_org;
			topIndex=i;
		}

		if(bottom<(monitors[i].y_org+monitors[i].height))
		{
			bottom=(monitors[i].y_org+monitors[i].height);
			bottomIndex=i;
		}
	}
	ESDASHBOARD_DEBUG(inWindowTracker, WINDOWS,
						"Found edge monitors: left=%d (monitor %d), right=%d (monitor %d), top=%d (monitor %d), bottom=%d (monitor %d)",
						left, leftIndex,
						right, rightIndex,
						top, topIndex,
						bottom, bottomIndex);

	/* Get X atom for fullscreen-across-all-monitors */
	atomFullscreenMonitors=XInternAtom(gdk_x11_display_get_xdisplay(display),
										"_NET_WM_FULLSCREEN_MONITORS",
										False);

	/* Send event to X to set window to fullscreen over all monitors */
	memset(&xEvent, 0, sizeof(xEvent));
	xEvent.type=ClientMessage;
	xEvent.xclient.window=wnck_window_get_xid(stageWindow);
	xEvent.xclient.display=GDK_DISPLAY_XDISPLAY(display);
	xEvent.xclient.message_type=atomFullscreenMonitors;
	xEvent.xclient.format=32;
	xEvent.xclient.data.l[0]=topIndex;
	xEvent.xclient.data.l[1]=bottomIndex;
	xEvent.xclient.data.l[2]=leftIndex;
	xEvent.xclient.data.l[3]=rightIndex;
	xEvent.xclient.data.l[4]=0;
	XSendEvent(GDK_DISPLAY_XDISPLAY(display),
				DefaultRootWindow(GDK_DISPLAY_XDISPLAY(display)),
				False,
				SubstructureRedirectMask | SubstructureNotifyMask,
				&xEvent);

	/* Release allocated resources */
	if(monitors) XFree(monitors);
#else
	EsdashboardWindowTrackerWindowX11	*realStageWindow;
	WnckWindow							*stageWindow;
	GdkScreen							*screen;
	gint								primaryMonitor;
	GdkRectangle						geometry;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(inWindowTracker));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	realStageWindow=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	ESDASHBOARD_DEBUG(inWindowTracker, WINDOWS, "No support for multiple monitor: Setting fullscreen on primary monitor");

	/* Get wnck window for stage window object as it is needed a lot from this
	 * point on.
	 */
	stageWindow=esdashboard_window_tracker_window_x11_get_window(realStageWindow);

	/* Get screen */
	screen=gdk_screen_get_default();

	/* Get position and size of primary monitor and try to move and resize
	 * stage window to its position and size. Even if it fails it should
	 * resize the stage to the size of current monitor this window is
	 * fullscreened to. Tested with xfwm4.
	 */
	primaryMonitor=gdk_screen_get_primary_monitor(screen);
	gdk_screen_get_monitor_geometry(screen, primaryMonitor, &geometry);
	wnck_window_set_geometry(stageWindow,
								WNCK_WINDOW_GRAVITY_STATIC,
								WNCK_WINDOW_CHANGE_X | WNCK_WINDOW_CHANGE_Y | WNCK_WINDOW_CHANGE_WIDTH | WNCK_WINDOW_CHANGE_HEIGHT,
								geometry.x, geometry.y, geometry.width, geometry.height);

	ESDASHBOARD_DEBUG(inWindowTracker, WINDOWS,
						"Moving stage window to %d,%d and resize to %dx%d",
						geometry.x, geometry.y,
						geometry.width, geometry.height);
#endif
}


/* IMPLEMENTATION: EsdashboardWindowTrackerBackend */

/* Get name of backend */
static const gchar* _esdashboard_window_tracker_backend_x11_window_tracker_backend_get_name(EsdashboardWindowTrackerBackend *inBackend)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND_X11(inBackend), NULL);

	/* Return name of backend */
	return("X11");
}

/* Get window tracker instance used by this backend */
static EsdashboardWindowTracker* _esdashboard_window_tracker_backend_x11_window_tracker_backend_get_window_tracker(EsdashboardWindowTrackerBackend *inBackend)
{
	EsdashboardWindowTrackerBackendX11				*self;
	EsdashboardWindowTrackerBackendX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND_X11(inBackend), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_BACKEND_X11(inBackend);
	priv=self->priv;

	/* Return window tracker instance used by this instance but do not take a
	 * reference on it as it will be done by the backend interface when
	 * esdashboard_window_tracker_backend_get_default() is called.
	 */
	return(ESDASHBOARD_WINDOW_TRACKER(priv->windowTracker));
}

/* Get window of stage */
static EsdashboardWindowTrackerWindow* _esdashboard_window_tracker_backend_x11_window_tracker_backend_get_window_for_stage(EsdashboardWindowTrackerBackend *inBackend,
																															ClutterStage *inStage)
{
	EsdashboardWindowTrackerBackendX11			*self;
	EsdashboardWindowTrackerBackendX11Private	*priv;
	Window										stageXWindow;
	WnckWindow									*wnckWindow;
	EsdashboardWindowTrackerWindow				*window;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND_X11(inBackend), NULL);
	g_return_val_if_fail(CLUTTER_IS_STAGE(inStage), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_BACKEND_X11(inBackend);
	priv=self->priv;

	/* Get stage X window and translate to needed window type */
	stageXWindow=clutter_x11_get_stage_window(inStage);
	wnckWindow=wnck_window_get(stageXWindow);

	/* Get or create window object for wnck background window */
	window=esdashboard_window_tracker_x11_get_window_for_wnck(priv->windowTracker, wnckWindow);
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Resolved stage wnck window %s@%p of stage %s@%p to window object %s@%p",
						G_OBJECT_TYPE_NAME(wnckWindow), wnckWindow,
						G_OBJECT_TYPE_NAME(inStage), inStage,
						G_OBJECT_TYPE_NAME(window), window);

	return(window);
}

/* Get associated stage of window */
static ClutterStage* _esdashboard_window_tracker_backend_x11_window_tracker_backend_get_stage_from_window(EsdashboardWindowTrackerBackend *inBackend,
																											EsdashboardWindowTrackerWindow *inStageWindow)
{
	EsdashboardWindowTrackerBackendX11			*self;
	EsdashboardWindowTrackerWindowX11			*stageWindow;
	WnckWindow									*stageWnckWindow;
	Window										stageXWindow;
	ClutterStage								*foundStage;
	GSList										*stages;
	GSList										*iter;
	Window										iterXWindow;
	ClutterStage								*stage;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND_X11(inBackend), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inStageWindow), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_BACKEND_X11(inBackend);
	stageWindow=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inStageWindow);

	/* Get wnck and X window of stage window */
	stageWnckWindow=esdashboard_window_tracker_window_x11_get_window(stageWindow);
	if(!stageWnckWindow)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Could not get wnck window for window %s@%p",
							G_OBJECT_TYPE_NAME(stageWindow),
							stageWindow);
		g_critical("Could not get real stage window to find stage");
		return(NULL);
	}

	stageXWindow=wnck_window_get_xid(stageWnckWindow);
	if(stageXWindow==None)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Could not get X server window from wnck window %s@%p for window %s@%p",
							G_OBJECT_TYPE_NAME(stageWnckWindow),
							stageWnckWindow,
							G_OBJECT_TYPE_NAME(stageWindow),
							stageWindow);
		g_critical("Could not get real stage window to find stage");
		return(NULL);
	}

	/* Iterate through stages and check if stage window matches requested one */
	foundStage=NULL;
	stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
	for(iter=stages; !foundStage && iter; iter=g_slist_next(iter))
	{
		stage=CLUTTER_STAGE(iter->data);
		if(stage)
		{
			iterXWindow=clutter_x11_get_stage_window(stage);
			if(iterXWindow==stageXWindow) foundStage=stage;
		}
	}
	g_slist_free(stages);

	/* Return stage found */
	return(foundStage);
}

/* Set up and show window for use as stage */
static void _esdashboard_window_tracker_backend_x11_window_tracker_backend_show_stage_window(EsdashboardWindowTrackerBackend *inBackend,
																								EsdashboardWindowTrackerWindow *inStageWindow)
{
	EsdashboardWindowTrackerBackendX11			*self;
	EsdashboardWindowTrackerBackendX11Private	*priv;
	EsdashboardWindowTrackerWindowX11			*stageWindow;
	WnckWindow									*stageWnckWindow;
	WnckScreen									*screen;
	guint										signalID;
	gulong										handlerID;
	gint										width, height;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND_X11(inBackend));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inStageWindow));

	self=ESDASHBOARD_WINDOW_TRACKER_BACKEND_X11(inBackend);
	priv=self->priv;
	stageWindow=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inStageWindow);

	/* Get wnck window of stage window */
	stageWnckWindow=esdashboard_window_tracker_window_x11_get_window(stageWindow);
	if(!stageWnckWindow)
	{
		g_critical("Could not get real stage window to show");
		return;
	}

	/* Window of stage should always be above all other windows, pinned to all
	 * workspaces, not be listed in window pager and set to fullscreen
	 */
	if(!wnck_window_is_skip_tasklist(stageWnckWindow)) wnck_window_set_skip_tasklist(stageWnckWindow, TRUE);
	if(!wnck_window_is_skip_pager(stageWnckWindow)) wnck_window_set_skip_pager(stageWnckWindow, TRUE);
	if(!wnck_window_is_above(stageWnckWindow)) wnck_window_make_above(stageWnckWindow);
	if(!wnck_window_is_pinned(stageWnckWindow)) wnck_window_pin(stageWnckWindow);

	/* Get screen of window */
	screen=wnck_window_get_screen(stageWnckWindow);

	/* Connect signals if not already connected */
	signalID=g_signal_lookup("state-changed", WNCK_TYPE_WINDOW);
	handlerID=g_signal_handler_find(stageWnckWindow,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
									signalID,
									0,
									NULL,
									G_CALLBACK(_esdashboard_window_tracker_backend_x11_on_stage_state_changed),
									stageWindow);
	if(!handlerID)
	{
		handlerID=g_signal_connect(stageWnckWindow,
									"state-changed",
									G_CALLBACK(_esdashboard_window_tracker_backend_x11_on_stage_state_changed),
									stageWindow);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Connecting signal handler %lu to 'state-changed' at window %p (wnck-window=%p)",
							handlerID,
							stageWindow,
							stageWnckWindow);
	}

	signalID=g_signal_lookup("active-window-changed", WNCK_TYPE_SCREEN);
	handlerID=g_signal_handler_find(screen,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
									signalID,
									0,
									NULL,
									G_CALLBACK(_esdashboard_window_tracker_backend_x11_on_stage_active_window_changed),
									stageWindow);
	if(!handlerID)
	{
		handlerID=g_signal_connect(screen,
									"active-window-changed",
									G_CALLBACK(_esdashboard_window_tracker_backend_x11_on_stage_active_window_changed),
									stageWindow);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Connecting signal handler %lu to 'active-window-changed' at screen %p of window %p (wnck-window=%p)",
							handlerID,
							screen,
							stageWindow,
							stageWnckWindow);
	}

	signalID=g_signal_lookup("screen-size-changed", ESDASHBOARD_TYPE_WINDOW_TRACKER);
	handlerID=g_signal_handler_find(priv->windowTracker,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
									signalID,
									0,
									NULL,
									G_CALLBACK(_esdashboard_window_tracker_backend_x11_on_stage_screen_size_changed),
									stageWindow);
	if(!handlerID)
	{
		handlerID=g_signal_connect(priv->windowTracker,
									"screen-size-changed",
									G_CALLBACK(_esdashboard_window_tracker_backend_x11_on_stage_screen_size_changed),
									stageWindow);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Connecting signal handler %lu to 'screen-size-changed' at window %p (wnck-window=%p)",
							handlerID,
							stageWindow,
							stageWnckWindow);
	}
	esdashboard_window_tracker_get_screen_size(ESDASHBOARD_WINDOW_TRACKER(priv->windowTracker), &width, &height);
	_esdashboard_window_tracker_backend_x11_on_stage_screen_size_changed(ESDASHBOARD_WINDOW_TRACKER(priv->windowTracker),
																			width,
																			height,
																			inStageWindow);

	/* Now the window is set up and we can show it */
	esdashboard_window_tracker_window_show(inStageWindow);
}

/* Unset up and hide stage window */
static void _esdashboard_window_tracker_backend_x11_window_tracker_backend_hide_stage_window(EsdashboardWindowTrackerBackend *inBackend,
																								EsdashboardWindowTrackerWindow *inStageWindow)
{
	EsdashboardWindowTrackerBackendX11			*self;
	EsdashboardWindowTrackerBackendX11Private	*priv;
	EsdashboardWindowTrackerWindowX11			*stageWindow;
	WnckWindow									*stageWnckWindow;
	WnckScreen									*screen;
	guint										signalID;
	gulong										handlerID;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND_X11(inBackend));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inStageWindow));

	self=ESDASHBOARD_WINDOW_TRACKER_BACKEND_X11(inBackend);
	priv=self->priv;
	stageWindow=ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inStageWindow);

	/* Get wnck window of stage window */
	stageWnckWindow=esdashboard_window_tracker_window_x11_get_window(stageWindow);
	if(!stageWnckWindow)
	{
		g_critical("Could not get real stage window to hide");
		return;
	}

	/* First hide window before removing signals etc. */
	esdashboard_window_tracker_window_hide(inStageWindow);

	/* Get screen of window */
	screen=wnck_window_get_screen(stageWnckWindow);

	/* Disconnect signals */
	signalID=g_signal_lookup("state-changed", WNCK_TYPE_WINDOW);
	handlerID=g_signal_handler_find(stageWnckWindow,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
									signalID,
									0,
									NULL,
									G_CALLBACK(_esdashboard_window_tracker_backend_x11_on_stage_state_changed),
									stageWindow);
	if(handlerID)
	{
		g_signal_handler_disconnect(stageWnckWindow, handlerID);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Disconnecting handler %lu for signal 'state-changed' at window %p (wnck-window=%p)",
							handlerID,
							stageWindow,
							stageWnckWindow);
	}

	signalID=g_signal_lookup("active-window-changed", WNCK_TYPE_SCREEN);
	handlerID=g_signal_handler_find(screen,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
									signalID,
									0,
									NULL,
									G_CALLBACK(_esdashboard_window_tracker_backend_x11_on_stage_active_window_changed),
									stageWindow);
	if(handlerID)
	{
		g_signal_handler_disconnect(screen, handlerID);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Disconnecting handler %lu for signal 'active-window-changed' at screen %p of window %p (wnck-window=%p)",
							handlerID,
							screen,
							stageWindow,
							stageWnckWindow);
	}

	signalID=g_signal_lookup("screen-size-changed", ESDASHBOARD_TYPE_WINDOW_TRACKER);
	handlerID=g_signal_handler_find(priv->windowTracker,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
									signalID,
									0,
									NULL,
									G_CALLBACK(_esdashboard_window_tracker_backend_x11_on_stage_screen_size_changed),
									stageWindow);
	if(handlerID)
	{
		g_signal_handler_disconnect(priv->windowTracker, handlerID);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Disconnecting handler %lu for signal 'screen-size-changed' at window %p (wnck-window=%p)",
							handlerID,
							stageWindow,
							stageWnckWindow);
	}
}

/* Interface initialization
 * Set up default functions
 */
static void _esdashboard_window_tracker_backend_x11_window_tracker_backend_iface_init(EsdashboardWindowTrackerBackendInterface *iface)
{
	iface->get_name=_esdashboard_window_tracker_backend_x11_window_tracker_backend_get_name;

	iface->get_window_tracker=_esdashboard_window_tracker_backend_x11_window_tracker_backend_get_window_tracker;

	iface->get_window_for_stage=_esdashboard_window_tracker_backend_x11_window_tracker_backend_get_window_for_stage;
	iface->get_stage_from_window=_esdashboard_window_tracker_backend_x11_window_tracker_backend_get_stage_from_window;
	iface->show_stage_window=_esdashboard_window_tracker_backend_x11_window_tracker_backend_show_stage_window;
	iface->hide_stage_window=_esdashboard_window_tracker_backend_x11_window_tracker_backend_hide_stage_window;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_window_tracker_backend_x11_dispose(GObject *inObject)
{
	EsdashboardWindowTrackerBackendX11				*self=ESDASHBOARD_WINDOW_TRACKER_BACKEND_X11(inObject);
	EsdashboardWindowTrackerBackendX11Private		*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->windowTracker)
	{
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_window_tracker_backend_x11_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_window_tracker_backend_x11_class_init(EsdashboardWindowTrackerBackendX11Class *klass)
{
	GObjectClass						*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_window_tracker_backend_x11_dispose;
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_window_tracker_backend_x11_init(EsdashboardWindowTrackerBackendX11 *self)
{
	EsdashboardWindowTrackerBackendX11Private		*priv;

	priv=self->priv=esdashboard_window_tracker_backend_x11_get_instance_private(self);

	ESDASHBOARD_DEBUG(self, WINDOWS, "Initializing X11 window tracker backend");

	/* Create window tracker instance */
	priv->windowTracker=g_object_new(ESDASHBOARD_TYPE_WINDOW_TRACKER_X11, NULL);
}


/* IMPLEMENTATION: Public API */

/**
 * esdashboard_window_tracker_backend_x11_new:
 *
 * Creates a new #EsdashboardWindowTrackerBackendX11 backend for use with
 * Clutter's X11 backend.
 *
 * Return value: The newly created #EsdashboardWindowTrackerBackend
 */
EsdashboardWindowTrackerBackend* esdashboard_window_tracker_backend_x11_new(void)
{
	return(g_object_new(ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_X11, NULL));
}
