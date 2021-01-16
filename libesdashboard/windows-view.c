/*
 * windows-view: A view showing visible windows
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

#include <libesdashboard/windows-view.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <libesdashboard/live-window.h>
#include <libesdashboard/scaled-table-layout.h>
#include <libesdashboard/stage.h>
#include <libesdashboard/application.h>
#include <libesdashboard/view.h>
#include <libesdashboard/drop-action.h>
#include <libesdashboard/quicklaunch.h>
#include <libesdashboard/application-button.h>
#include <libesdashboard/window-tracker.h>
#include <libesdashboard/image-content.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/enums.h>
#include <libesdashboard/stage-interface.h>
#include <libesdashboard/live-workspace.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
static void _esdashboard_windows_view_focusable_iface_init(EsdashboardFocusableInterface *iface);

struct _EsdashboardWindowsViewPrivate
{
	/* Properties related */
	EsdashboardWindowTrackerWorkspace	*workspace;
	gfloat								spacing;
	gboolean							preventUpscaling;
	gboolean							isScrollEventChangingWorkspace;

	/* Instance related */
	EsdashboardWindowTracker			*windowTracker;
	ClutterLayoutManager				*layout;
	gpointer							selectedItem;

	EsconfChannel						*esconfChannel;
	guint								esconfScrollEventChangingWorkspaceBindingID;
	EsdashboardStageInterface			*scrollEventChangingWorkspaceStage;
	guint								scrollEventChangingWorkspaceStageSignalID;

	gboolean							isWindowsNumberShown;

	gboolean							filterMonitorWindows;
	gboolean							filterWorkspaceWindows;
	EsdashboardStageInterface			*currentStage;
	EsdashboardWindowTrackerMonitor		*currentMonitor;
	guint								currentStageMonitorBindingID;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardWindowsView,
						esdashboard_windows_view,
						ESDASHBOARD_TYPE_VIEW,
						G_ADD_PRIVATE(EsdashboardWindowsView)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_FOCUSABLE, _esdashboard_windows_view_focusable_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_WORKSPACE,
	PROP_SPACING,
	PROP_PREVENT_UPSCALING,
	PROP_SCROLL_EVENT_CHANGES_WORKSPACE,
	PROP_FILTER_MONITOR_WINDOWS,
	PROP_FILTER_WORKSPACE_WINDOWS,

	PROP_LAST
};

static GParamSpec* EsdashboardWindowsViewProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	ACTION_WINDOW_CLOSE,
	ACTION_WINDOWS_SHOW_NUMBERS,
	ACTION_WINDOWS_HIDE_NUMBERS,
	ACTION_WINDOWS_ACTIVATE_WINDOW_ONE,
	ACTION_WINDOWS_ACTIVATE_WINDOW_TWO,
	ACTION_WINDOWS_ACTIVATE_WINDOW_THREE,
	ACTION_WINDOWS_ACTIVATE_WINDOW_FOUR,
	ACTION_WINDOWS_ACTIVATE_WINDOW_FIVE,
	ACTION_WINDOWS_ACTIVATE_WINDOW_SIX,
	ACTION_WINDOWS_ACTIVATE_WINDOW_SEVEN,
	ACTION_WINDOWS_ACTIVATE_WINDOW_EIGHT,
	ACTION_WINDOWS_ACTIVATE_WINDOW_NINE,
	ACTION_WINDOWS_ACTIVATE_WINDOW_TEN,

	SIGNAL_LAST
};

static guint EsdashboardWindowsViewSignals[SIGNAL_LAST]={ 0, };

/* Forward declaration */
static void _esdashboard_windows_view_recreate_window_actors(EsdashboardWindowsView *self);
static EsdashboardLiveWindow* _esdashboard_windows_view_create_actor(EsdashboardWindowsView *self, EsdashboardWindowTrackerWindow *inWindow);
static void _esdashboard_windows_view_set_active_workspace(EsdashboardWindowsView *self, EsdashboardWindowTrackerWorkspace *inWorkspace);

/* IMPLEMENTATION: Private variables and methods */
#define SCROLL_EVENT_CHANGES_WORKSPACE_ESCONF_PROP		"/components/windows-view/scroll-event-changes-workspace"

#define DEFAULT_VIEW_ICON								"view-fullscreen"
#define DEFAULT_DRAG_HANDLE_SIZE						32.0f

/* Stage interface has changed monitor */
static void _esdashboard_windows_view_update_on_stage_monitor_changed(EsdashboardWindowsView *self,
																		GParamSpec *inSpec,
																		gpointer inUserData)
{
	EsdashboardWindowsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));

	priv=self->priv;

	/* Get new reference to new monitor of stage */
	priv->currentMonitor=esdashboard_stage_interface_get_monitor(priv->currentStage);

	/* Recreate all window actors */
	_esdashboard_windows_view_recreate_window_actors(self);
}

/* Update reference to stage interface and monitor where this view is on */
static gboolean _esdashboard_windows_view_update_stage_and_monitor(EsdashboardWindowsView *self)
{
	EsdashboardWindowsViewPrivate		*priv;
	EsdashboardStageInterface			*newStage;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), FALSE);

	priv=self->priv;

	/* Iterate through parent actors until stage interface is reached */
	newStage=esdashboard_get_stage_of_actor(CLUTTER_ACTOR(self));

	/* If stage did not change return immediately */
	if(newStage==priv->currentStage) return(FALSE);

	/* Release old references to stage and monitor */
	priv->currentMonitor=NULL;

	if(priv->currentStage)
	{
		if(priv->currentStageMonitorBindingID)
		{
			g_signal_handler_disconnect(priv->currentStage, priv->currentStageMonitorBindingID);
			priv->currentStageMonitorBindingID=0;
		}

		priv->currentStage=NULL;
	}

	/* Get new references to new stage and monitor and connect signal to get notified
	 * if stage changes monitor.
	 */
	if(newStage)
	{
		priv->currentStage=newStage;
		priv->currentStageMonitorBindingID=g_signal_connect_swapped(priv->currentStage,
																	"notify::monitor",
																	G_CALLBACK(_esdashboard_windows_view_update_on_stage_monitor_changed),
																	self);

		/* Get new reference to current monitor of new stage */
		priv->currentMonitor=esdashboard_stage_interface_get_monitor(priv->currentStage);
	}

	/* Stage and monitor changed and references were renewed and setup */
	return(TRUE);
}

/* Check if window should be shown */
static gboolean _esdashboard_windows_view_is_visible_window(EsdashboardWindowsView *self,
																EsdashboardWindowTrackerWindow *inWindow)
{
	EsdashboardWindowsViewPrivate			*priv;
	EsdashboardWindowTrackerWindowState		state;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), FALSE);

	priv=self->priv;

	/* Get state of window */
	state=esdashboard_window_tracker_window_get_state(inWindow);

	/* Determine if windows should be shown depending on its state, size and position */
	if(state & ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER)
	{
		return(FALSE);
	}

	if(state & ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST)
	{
		return(FALSE);
	}

	if(esdashboard_window_tracker_window_is_stage(inWindow))
	{
		return(FALSE);
	}

	if(!priv->workspace)
	{
		return(FALSE);
	}

	if(!esdashboard_window_tracker_window_is_visible(inWindow) ||
		(priv->filterWorkspaceWindows && !esdashboard_window_tracker_window_is_on_workspace(inWindow, priv->workspace)))
	{
		return(FALSE);
	}

	if(priv->filterMonitorWindows &&
		esdashboard_window_tracker_supports_multiple_monitors(priv->windowTracker) &&
		(!priv->currentMonitor || !esdashboard_window_tracker_window_is_on_monitor(inWindow, priv->currentMonitor)))
	{
		return(FALSE);
	}

	/* If we get here the window should be shown */
	return(TRUE);
}

/* Find live window actor by window */
static EsdashboardLiveWindow* _esdashboard_windows_view_find_by_window(EsdashboardWindowsView *self,
																		EsdashboardWindowTrackerWindow *inWindow)
{
	EsdashboardLiveWindow		*liveWindow;
	ClutterActor				*child;
	ClutterActorIter			iter;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	/* Iterate through list of current actors and find the one for requested window */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(!ESDASHBOARD_IS_LIVE_WINDOW(child)) continue;

		liveWindow=ESDASHBOARD_LIVE_WINDOW(child);
		if(esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(liveWindow))==inWindow)
		{
			return(liveWindow);
		}
	}

	/* If we get here we did not find the window and we return NULL */
	return(NULL);
}

/* Update window number in close button of each window actor */
static void _esdashboard_windows_view_update_window_number_in_actors(EsdashboardWindowsView *self)
{
	EsdashboardWindowsViewPrivate		*priv;
	ClutterActor						*child;
	ClutterActorIter					iter;
	gint								index;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));

	priv=self->priv;

	/* Iterate through list of current actors and for the first ten actors
	 * change the close button to window number and the rest will still be
	 * close buttons.
	 */
	index=1;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only live window actors can be handled */
		if(!ESDASHBOARD_IS_LIVE_WINDOW(child)) continue;

		/* If this is one of the first ten window actors change close button
		 * to window number and set number.
		 */
		if(priv->isWindowsNumberShown && index<=10)
		{
			g_object_set(child, "window-number", index, NULL);
			index++;
		}
			else
			{
				g_object_set(child, "window-number", 0, NULL);
			}
	}
}

/* Recreate all window actors in this view */
static void _esdashboard_windows_view_recreate_window_actors(EsdashboardWindowsView *self)
{
	EsdashboardWindowsViewPrivate			*priv;
	GList									*windowsList;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));

	priv=self->priv;

	/* Remove weak reference at current selection and unset selection */
	if(priv->selectedItem)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
		priv->selectedItem=NULL;
	}

	/* Destroy all actors */
	esdashboard_actor_destroy_all_children(CLUTTER_ACTOR(self));

	/* Create live window actors for new workspace */
	if(priv->workspace!=NULL)
	{
		/* Get list of all windows open */
		windowsList=esdashboard_window_tracker_get_windows(priv->windowTracker);

		/* Iterate through list of window (from last to first), check if window
		 * is visible and create actor for it if it is.
		 */
		windowsList=g_list_last(windowsList);
		while(windowsList)
		{
			EsdashboardWindowTrackerWindow	*window=ESDASHBOARD_WINDOW_TRACKER_WINDOW(windowsList->data);
			EsdashboardLiveWindow			*liveWindow;

			/* Check if window is visible on this workspace */
			if(_esdashboard_windows_view_is_visible_window(self, window))
			{
				/* Create actor */
				liveWindow=_esdashboard_windows_view_create_actor(ESDASHBOARD_WINDOWS_VIEW(self), window);
				if(liveWindow)
				{
					clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(liveWindow));
					_esdashboard_windows_view_update_window_number_in_actors(self);
				}
			}

			/* Next window */
			windowsList=g_list_previous(windowsList);
		}
	}
}

/* Move window to monitor of this window view */
static void _esdashboard_windows_view_move_live_to_view(EsdashboardWindowsView *self,
														EsdashboardLiveWindow *inWindowActor)
{
	EsdashboardWindowsViewPrivate			*priv;
	EsdashboardWindowTrackerWindow			*window;
	EsdashboardWindowTrackerWorkspace		*sourceWorkspace;
	EsdashboardWindowTrackerWorkspace		*targetWorkspace;
	EsdashboardWindowTrackerMonitor			*sourceMonitor;
	EsdashboardWindowTrackerMonitor			*targetMonitor;
	gint									oldWindowX, oldWindowY, oldWindowWidth, oldWindowHeight;
	gint									newWindowX, newWindowY;
	gint									oldMonitorX, oldMonitorY, oldMonitorWidth, oldMonitorHeight;
	gint									newMonitorX, newMonitorY, newMonitorWidth, newMonitorHeight;
	gfloat									relativeX, relativeY;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(inWindowActor));

	priv=self->priv;

	/* Get window from window actor */
	window=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(inWindowActor));

	/* Get source and target workspace */
	sourceWorkspace=esdashboard_window_tracker_window_get_workspace(window);
	targetWorkspace=priv->workspace;

	/* Get source and target monitor */
	sourceMonitor=esdashboard_window_tracker_window_get_monitor(window);
	targetMonitor=priv->currentMonitor;

	ESDASHBOARD_DEBUG(self, ACTOR,
						"Moving window '%s' from %s-monitor %d to %s-monitor %d and from workspace '%s' (%d) to '%s' (%d)",
						esdashboard_window_tracker_window_get_name(window),
						esdashboard_window_tracker_monitor_is_primary(sourceMonitor) ? "primary" : "secondary",
						esdashboard_window_tracker_monitor_get_number(sourceMonitor),
						esdashboard_window_tracker_monitor_is_primary(targetMonitor) ? "primary" : "secondary",
						esdashboard_window_tracker_monitor_get_number(targetMonitor),
						esdashboard_window_tracker_workspace_get_name(sourceWorkspace),
						esdashboard_window_tracker_workspace_get_number(sourceWorkspace),
						esdashboard_window_tracker_workspace_get_name(targetWorkspace),
						esdashboard_window_tracker_workspace_get_number(targetWorkspace));

	/* Get position and size of window to move */
	esdashboard_window_tracker_window_get_geometry(window,
													&oldWindowX,
													&oldWindowY,
													&oldWindowWidth,
													&oldWindowHeight);

	/* Calculate source x and y coordinate relative to monitor size in percent */
	esdashboard_window_tracker_monitor_get_geometry(sourceMonitor,
													&oldMonitorX,
													&oldMonitorY,
													&oldMonitorWidth,
													&oldMonitorHeight);
	relativeX=((gfloat)(oldWindowX-oldMonitorX)) / ((gfloat)oldMonitorWidth);
	relativeY=((gfloat)(oldWindowY-oldMonitorY)) / ((gfloat)oldMonitorHeight);

	/* Calculate target x and y coordinate from relative size in percent to monitor size */
	esdashboard_window_tracker_monitor_get_geometry(targetMonitor,
													&newMonitorX,
													&newMonitorY,
													&newMonitorWidth,
													&newMonitorHeight);
	newWindowX=newMonitorX+((gint)(relativeX*newMonitorWidth));
	newWindowY=newMonitorY+((gint)(relativeY*newMonitorHeight));

	/* Move window to workspace if they are not the same */
	if(!esdashboard_window_tracker_workspace_is_equal(sourceWorkspace, targetWorkspace))
	{
		esdashboard_window_tracker_window_move_to_workspace(window, targetWorkspace);
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Moved window '%s' from workspace '%s' (%d) to '%s' (%d)",
							esdashboard_window_tracker_window_get_name(window),
							esdashboard_window_tracker_workspace_get_name(sourceWorkspace),
							esdashboard_window_tracker_workspace_get_number(sourceWorkspace),
							esdashboard_window_tracker_workspace_get_name(targetWorkspace),
							esdashboard_window_tracker_workspace_get_number(targetWorkspace));
	}

	/* Move window to new position */
	esdashboard_window_tracker_window_move(window, newWindowX, newWindowY);
	ESDASHBOARD_DEBUG(self, ACTOR,
						"Moved window '%s' from [%d,%d] at monitor [%d,%d x %d,%d] to [%d,%d] at monitor [%d,%d x %d,%d] (relative x=%.2f, y=%.2f)",
						esdashboard_window_tracker_window_get_name(window),
						oldWindowX, oldWindowY,
						oldMonitorX, oldMonitorY, oldMonitorWidth, oldMonitorHeight,
						newWindowX, newWindowY,
						newMonitorX, newMonitorY, newMonitorWidth, newMonitorHeight,
						relativeX, relativeY);
}

/* Drag of an actor to this view as drop target begins */
static gboolean _esdashboard_windows_view_on_drop_begin(EsdashboardWindowsView *self,
														EsdashboardDragAction *inDragAction,
														gpointer inUserData)
{
	ClutterActor					*dragSource;
	ClutterActor					*draggedActor;
	gboolean						canHandle;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData), FALSE);

	canHandle=FALSE;

	/* Get source where dragging started and actor being dragged */
	dragSource=esdashboard_drag_action_get_source(inDragAction);
	draggedActor=esdashboard_drag_action_get_actor(inDragAction);

	/* We can handle dragged actor if it is an application button and its source
	 * is quicklaunch.
	 */
	if(ESDASHBOARD_IS_QUICKLAUNCH(dragSource) &&
		ESDASHBOARD_IS_APPLICATION_BUTTON(draggedActor))
	{
		canHandle=TRUE;
	}

	/* We can handle dragged actor if it is a live window and its source
	 * is windows view.
	 */
	if(ESDASHBOARD_IS_WINDOWS_VIEW(dragSource) &&
		ESDASHBOARD_IS_LIVE_WINDOW(draggedActor))
	{
		canHandle=TRUE;
	}

	/* We can handle dragged actor if it is a live window and its source
	 * is a live workspace
	 */
	if(ESDASHBOARD_IS_LIVE_WORKSPACE(dragSource) &&
		ESDASHBOARD_IS_LIVE_WINDOW_SIMPLE(draggedActor))
	{
		canHandle=TRUE;
	}

	/* Return TRUE if we can handle dragged actor in this drop target
	 * otherwise FALSE
	 */
	return(canHandle);
}

/* Dragged actor was dropped on this drop target */
static void _esdashboard_windows_view_on_drop_drop(EsdashboardWindowsView *self,
													EsdashboardDragAction *inDragAction,
													gfloat inX,
													gfloat inY,
													gpointer inUserData)
{
	EsdashboardWindowsViewPrivate		*priv;
	ClutterActor						*dragSource;
	ClutterActor						*draggedActor;
	GAppLaunchContext					*context;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get source where dragging started and actor being dragged */
	dragSource=esdashboard_drag_action_get_source(inDragAction);
	draggedActor=esdashboard_drag_action_get_actor(inDragAction);

	/* Handle drop of an application button from quicklaunch */
	if(ESDASHBOARD_IS_QUICKLAUNCH(dragSource) &&
		ESDASHBOARD_IS_APPLICATION_BUTTON(draggedActor))
	{
		/* Launch application being dragged here */
		context=esdashboard_create_app_context(priv->workspace);
		esdashboard_application_button_execute(ESDASHBOARD_APPLICATION_BUTTON(draggedActor), context);
		g_object_unref(context);

		/* Drop action handled so return here */
		return;
	}

	/* Handle drop of an window from another windows view */
	if(ESDASHBOARD_IS_WINDOWS_VIEW(dragSource) &&
		ESDASHBOARD_IS_LIVE_WINDOW(draggedActor))
	{
		EsdashboardWindowsView			*sourceWindowsView;
		EsdashboardLiveWindow			*liveWindowActor;

		/* Get source windows view */
		sourceWindowsView=ESDASHBOARD_WINDOWS_VIEW(dragSource);

		/* Do nothing if source windows view is the same as target windows view
		 * that means this one.
		 */
		if(sourceWindowsView==self)
		{
			ESDASHBOARD_DEBUG(self, ACTOR,
						"Will not handle drop of %s at %s because source and target are the same.",
						G_OBJECT_TYPE_NAME(draggedActor),
						G_OBJECT_TYPE_NAME(dragSource));
			return;
		}

		/* Get dragged window */
		liveWindowActor=ESDASHBOARD_LIVE_WINDOW(draggedActor);

		/* Move dragged window to monitor of this window view */
		_esdashboard_windows_view_move_live_to_view(self, liveWindowActor);

		/* Drop action handled so return here */
		return;
	}

	/* Handle drop of an window from a live workspace */
	if(ESDASHBOARD_IS_LIVE_WORKSPACE(dragSource) &&
		ESDASHBOARD_IS_LIVE_WINDOW_SIMPLE(draggedActor))
	{
		EsdashboardLiveWorkspace			*sourceLiveWorkspace;
		EsdashboardWindowTrackerWorkspace*	sourceWorkspace;
		EsdashboardLiveWindowSimple			*liveWindowActor;
		EsdashboardWindowTrackerWindow		*window;

		/* Get source live workspace and its workspace */
		sourceLiveWorkspace=ESDASHBOARD_LIVE_WORKSPACE(dragSource);
		sourceWorkspace=esdashboard_live_workspace_get_workspace(sourceLiveWorkspace);

		/* Do nothing if source and destination workspaces are the same as nothing
		 * is to do in this case.
		 */
		if(esdashboard_window_tracker_workspace_is_equal(sourceWorkspace, priv->workspace))
		{
			ESDASHBOARD_DEBUG(self, ACTOR,
						"Will not handle drop of %s at %s because source and target workspaces are the same.",
						G_OBJECT_TYPE_NAME(draggedActor),
						G_OBJECT_TYPE_NAME(dragSource));
			return;
		}

		/* Get dragged window */
		liveWindowActor=ESDASHBOARD_LIVE_WINDOW_SIMPLE(draggedActor);
		window=esdashboard_live_window_simple_get_window(liveWindowActor);

		/* Move dragged window to workspace of this window view */
		esdashboard_window_tracker_window_move_to_workspace(window, priv->workspace);

		/* Drop action handled so return here */
		return;
	}

	/* If we get here we did not handle drop action properly
	 * and this should never happen.
	 */
	g_critical("Did not handle drop action for dragged actor %s of source %s at target %s",
				G_OBJECT_TYPE_NAME(draggedActor),
				G_OBJECT_TYPE_NAME(dragSource),
				G_OBJECT_TYPE_NAME(self));
}

/* A child actor was added to view */
static void _esdashboard_windows_view_on_child_added(ClutterContainer *inContainer,
														ClutterActor *inChild,
														gpointer inUserData)
{
	ClutterActorIter			iter;
	ClutterActor				*child;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inContainer));

	/* Iterate through list of current actors and enable allocation animation */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(ESDASHBOARD_IS_ACTOR(child))
		{
			esdashboard_actor_enable_allocation_animation_once(ESDASHBOARD_ACTOR(child));
		}
	}
}

/* A child actor was removed from view */
static void _esdashboard_windows_view_on_child_removed(ClutterContainer *inContainer,
														ClutterActor *inChild,
														gpointer inUserData)
{
	ClutterActorIter			iter;
	ClutterActor				*child;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inContainer));

	/* Iterate through list of current actors and enable allocation animation */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(ESDASHBOARD_IS_ACTOR(child))
		{
			esdashboard_actor_enable_allocation_animation_once(ESDASHBOARD_ACTOR(child));
		}
	}
}

/* Active workspace was changed */
static void _esdashboard_windows_view_on_active_workspace_changed(EsdashboardWindowsView *self,
																	EsdashboardWindowTrackerWorkspace *inPrevWorkspace,
																	EsdashboardWindowTrackerWorkspace *inNewWorkspace,
																	gpointer inUserData)
{
	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));

	/* Update window list */
	_esdashboard_windows_view_set_active_workspace(self, inNewWorkspace);
}

/* A window was opened */
static void _esdashboard_windows_view_on_window_opened(EsdashboardWindowsView *self,
														EsdashboardWindowTrackerWindow *inWindow,
														gpointer inUserData)
{
	EsdashboardLiveWindow				*liveWindow;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	/* Check if parent stage interface changed. If not just add window actor.
	 * Otherwise recreate all window actors for changed stage interface and
	 * monitor.
	 */
	if(!_esdashboard_windows_view_update_stage_and_monitor(self))
	{
		/* Check if window is visible on this workspace */
		if(!_esdashboard_windows_view_is_visible_window(self, inWindow)) return;

		/* Create actor if it does not exist already */
		liveWindow=_esdashboard_windows_view_find_by_window(self, inWindow);
		if(G_LIKELY(!liveWindow))
		{
			liveWindow=_esdashboard_windows_view_create_actor(self, inWindow);
			if(liveWindow)
			{
				clutter_actor_insert_child_below(CLUTTER_ACTOR(self), CLUTTER_ACTOR(liveWindow), NULL);
				_esdashboard_windows_view_update_window_number_in_actors(self);
			}
		}
	}
		else
		{
			/* Recreate all window actors because parent stage interface changed */
			_esdashboard_windows_view_recreate_window_actors(self);
		}
}

/* A window has changed monitor */
static void _esdashboard_windows_view_on_window_monitor_changed(EsdashboardWindowsView *self,
																EsdashboardWindowTrackerWindow *inWindow,
																EsdashboardWindowTrackerMonitor *inOldMonitor,
																EsdashboardWindowTrackerMonitor *inNewMonitor,
																gpointer inUserData)
{
	EsdashboardWindowsViewPrivate		*priv;
	EsdashboardLiveWindow				*liveWindow;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));
	g_return_if_fail(inOldMonitor==NULL || ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inOldMonitor));
	g_return_if_fail(inNewMonitor==NULL || ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inNewMonitor));

	priv=self->priv;

	/* Check if parent stage interface changed. If not check if window has
	 * moved away from this view and destroy it or it has moved to this view
	 * and create it. Otherwise recreate all window actors for changed stage
	 * interface and monitor.
	 */
	if(!_esdashboard_windows_view_update_stage_and_monitor(self) &&
		G_LIKELY(!inOldMonitor) &&
		G_LIKELY(!inNewMonitor))
	{
		/* Check if window moved away from this view */
		if(priv->currentMonitor==inOldMonitor &&
			!_esdashboard_windows_view_is_visible_window(self, inWindow))
		{
			/* Find live window for window to destroy it */
			liveWindow=_esdashboard_windows_view_find_by_window(self, inWindow);
			if(G_LIKELY(liveWindow))
			{
				/* Destroy actor */
				esdashboard_actor_destroy(CLUTTER_ACTOR(liveWindow));
			}
		}

		/* Check if window moved to this view */
		if(priv->currentMonitor==inNewMonitor &&
			_esdashboard_windows_view_is_visible_window(self, inWindow))
		{
			/* Create actor if it does not exist already */
			liveWindow=_esdashboard_windows_view_find_by_window(self, inWindow);
			if(G_LIKELY(!liveWindow))
			{
				liveWindow=_esdashboard_windows_view_create_actor(self, inWindow);
				if(liveWindow)
				{
					clutter_actor_insert_child_below(CLUTTER_ACTOR(self), CLUTTER_ACTOR(liveWindow), NULL);
					_esdashboard_windows_view_update_window_number_in_actors(self);
				}
			}
		}
	}
		else
		{
			/* Recreate all window actors because parent stage interface changed */
			_esdashboard_windows_view_recreate_window_actors(self);
		}
}

/* A live window was clicked */
static void _esdashboard_windows_view_on_window_clicked(EsdashboardWindowsView *self,
														gpointer inUserData)
{
	EsdashboardWindowsViewPrivate		*priv;
	EsdashboardLiveWindowSimple			*liveWindow;
	EsdashboardWindowTrackerWindow		*window;
	EsdashboardWindowTrackerWorkspace	*activeWorkspace;
	EsdashboardWindowTrackerWorkspace	*windowWorkspace;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW_SIMPLE(inUserData));

	priv=self->priv;
	liveWindow=ESDASHBOARD_LIVE_WINDOW_SIMPLE(inUserData);

	/* Get window to activate */
	window=esdashboard_live_window_simple_get_window(liveWindow);

	/* Move to workspace if window to active is on a different one than the active one */
	activeWorkspace=esdashboard_window_tracker_get_active_workspace(priv->windowTracker);
	if(!esdashboard_window_tracker_window_is_on_workspace(window, activeWorkspace))
	{
		windowWorkspace=esdashboard_window_tracker_window_get_workspace(window);
		esdashboard_window_tracker_workspace_activate(windowWorkspace);
	}

	/* Activate window */
	esdashboard_window_tracker_window_activate(window);

	/* Quit application */
	esdashboard_application_suspend_or_quit(NULL);
}

/* The close button of a live window was clicked */
static void _esdashboard_windows_view_on_window_close_clicked(EsdashboardWindowsView *self,
																gpointer inUserData)
{
	EsdashboardLiveWindowSimple			*liveWindow;
	EsdashboardWindowTrackerWindow		*window;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW_SIMPLE(inUserData));

	liveWindow=ESDASHBOARD_LIVE_WINDOW_SIMPLE(inUserData);

	/* Close clicked window */
	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW(esdashboard_live_window_simple_get_window(liveWindow));
	esdashboard_window_tracker_window_close(window);
}

/* A window was moved or resized */
static void _esdashboard_windows_view_on_window_geometry_changed(EsdashboardWindowsView *self,
																	gpointer inUserData)
{
	EsdashboardLiveWindow				*liveWindow;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(inUserData));

	liveWindow=ESDASHBOARD_LIVE_WINDOW(inUserData);

	/* Force a relayout to reflect new size of window */
	clutter_actor_queue_relayout(CLUTTER_ACTOR(liveWindow));
}

/* A window was hidden or shown */
static void _esdashboard_windows_view_on_window_visibility_changed(EsdashboardWindowsView *self,
																	gboolean inIsVisible,
																	gpointer inUserData)
{
	EsdashboardLiveWindow				*liveWindow;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(inUserData));

	liveWindow=ESDASHBOARD_LIVE_WINDOW(inUserData);

	/* If window is shown, show it in window list - otherwise hide it.
	 * We should not destroy the live window actor as the window might
	 * get visible again.
	 */
	if(inIsVisible) clutter_actor_show(CLUTTER_ACTOR(liveWindow));
		else
		{
			/* Hide actor */
			clutter_actor_hide(CLUTTER_ACTOR(liveWindow));
		}
}

/* A window changed workspace or was pinned to all workspaces */
static void _esdashboard_windows_view_on_window_workspace_changed(EsdashboardWindowsView *self,
																	EsdashboardWindowTrackerWindow *inWindow,
																	EsdashboardWindowTrackerWorkspace *inWorkspace,
																	gpointer inUserData)
{
	EsdashboardWindowsViewPrivate		*priv;
	EsdashboardLiveWindow				*liveWindow;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));
	g_return_if_fail(!inWorkspace || ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace));

	priv=self->priv;

	/* Check if parent stage interface changed. If not check if window has
	 * moved away from this view and destroy it or it has moved to this view
	 * and create it. Otherwise recreate all window actors for changed stage
	 * interface and monitor.
	 */
	if(!_esdashboard_windows_view_update_stage_and_monitor(self))
	{
		/* Check if window moved away from this view*/
		if(priv->workspace!=inWorkspace &&
			!_esdashboard_windows_view_is_visible_window(self, inWindow))
		{
			/* Find live window for window to destroy it */
			liveWindow=_esdashboard_windows_view_find_by_window(self, inWindow);
			if(G_LIKELY(liveWindow))
			{
				/* Destroy actor */
				esdashboard_actor_destroy(CLUTTER_ACTOR(liveWindow));
			}
		}

		/* Check if window moved to this view */
		if(priv->workspace==inWorkspace &&
			_esdashboard_windows_view_is_visible_window(self, inWindow))
		{
			/* Create actor if it does not exist already */
			liveWindow=_esdashboard_windows_view_find_by_window(self, inWindow);
			if(G_LIKELY(!liveWindow))
			{
				liveWindow=_esdashboard_windows_view_create_actor(self, inWindow);
				if(liveWindow)
				{
					clutter_actor_insert_child_below(CLUTTER_ACTOR(self), CLUTTER_ACTOR(liveWindow), NULL);
					_esdashboard_windows_view_update_window_number_in_actors(self);
				}
			}
		}
	}
		else
		{
			/* Recreate all window actors because parent stage interface changed */
			_esdashboard_windows_view_recreate_window_actors(self);
		}
}

/* Drag of a live window begins */
static void _esdashboard_windows_view_on_drag_begin(ClutterDragAction *inAction,
													ClutterActor *inActor,
													gfloat inStageX,
													gfloat inStageY,
													ClutterModifierType inModifiers,
													gpointer inUserData)
{
	ClutterActor					*dragHandle;
	ClutterStage					*stage;
	GdkPixbuf						*windowIcon;
	ClutterContent					*image;
	EsdashboardLiveWindowSimple		*liveWindow;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW_SIMPLE(inActor));
	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inUserData));

	liveWindow=ESDASHBOARD_LIVE_WINDOW_SIMPLE(inActor);

	/* Prevent signal "clicked" from being emitted on dragged icon */
	g_signal_handlers_block_by_func(inActor, _esdashboard_windows_view_on_window_clicked, inUserData);

	/* Get stage */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));

	/* Create a application icon for drag handle */
	windowIcon=esdashboard_window_tracker_window_get_icon(esdashboard_live_window_simple_get_window(liveWindow));
	image=esdashboard_image_content_new_for_pixbuf(windowIcon);

	dragHandle=esdashboard_background_new();
	clutter_actor_set_position(dragHandle, inStageX, inStageY);
	clutter_actor_set_size(dragHandle, DEFAULT_DRAG_HANDLE_SIZE, DEFAULT_DRAG_HANDLE_SIZE);
	esdashboard_background_set_image(ESDASHBOARD_BACKGROUND(dragHandle), CLUTTER_IMAGE(image));
	clutter_actor_add_child(CLUTTER_ACTOR(stage), dragHandle);

	clutter_drag_action_set_drag_handle(inAction, dragHandle);

	g_object_unref(image);
}

/* Drag of a live window ends */
static void _esdashboard_windows_view_on_drag_end(ClutterDragAction *inAction,
													ClutterActor *inActor,
													gfloat inStageX,
													gfloat inStageY,
													ClutterModifierType inModifiers,
													gpointer inUserData)
{
	ClutterActor					*dragHandle;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(inActor));
	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inUserData));

	/* Destroy clone of application icon used as drag handle */
	dragHandle=clutter_drag_action_get_drag_handle(inAction);
	if(dragHandle)
	{
#if CLUTTER_CHECK_VERSION(1, 14, 0)
		/* Only unset drag handle if not running Clutter in version
		 * 1.12. This prevents a critical warning message in 1.12.
		 * Later versions of Clutter are fixed already.
		 */
		clutter_drag_action_set_drag_handle(inAction, NULL);
#endif
		esdashboard_actor_destroy(dragHandle);
	}

	/* Allow signal "clicked" from being emitted again */
	g_signal_handlers_unblock_by_func(inActor, _esdashboard_windows_view_on_window_clicked, inUserData);
}

/* Create actor for wnck-window and connect signals */
static EsdashboardLiveWindow* _esdashboard_windows_view_create_actor(EsdashboardWindowsView *self,
																		EsdashboardWindowTrackerWindow *inWindow)
{
	ClutterActor	*actor;
	ClutterAction	*dragAction;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	/* Check if window opened is a stage window */
	if(esdashboard_window_tracker_window_is_stage(inWindow))
	{
		ESDASHBOARD_DEBUG(self, ACTOR, "Will not create live-window actor for stage window.");
		return(NULL);
	}

	/* Create actor and connect signals */
	actor=esdashboard_live_window_new();
	g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_esdashboard_windows_view_on_window_clicked), self);
	g_signal_connect_swapped(actor, "close", G_CALLBACK(_esdashboard_windows_view_on_window_close_clicked), self);
	g_signal_connect_swapped(actor, "geometry-changed", G_CALLBACK(_esdashboard_windows_view_on_window_geometry_changed), self);
	g_signal_connect_swapped(actor, "visibility-changed", G_CALLBACK(_esdashboard_windows_view_on_window_visibility_changed), self);
	esdashboard_live_window_simple_set_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(actor), inWindow);

	dragAction=esdashboard_drag_action_new_with_source(CLUTTER_ACTOR(self));
	clutter_drag_action_set_drag_threshold(CLUTTER_DRAG_ACTION(dragAction), -1, -1);
	clutter_actor_add_action(actor, dragAction);
	g_signal_connect(dragAction, "drag-begin", G_CALLBACK(_esdashboard_windows_view_on_drag_begin), self);
	g_signal_connect(dragAction, "drag-end", G_CALLBACK(_esdashboard_windows_view_on_drag_end), self);

	return(ESDASHBOARD_LIVE_WINDOW(actor));
}

/* Set active screen */
static void _esdashboard_windows_view_set_active_workspace(EsdashboardWindowsView *self,
															EsdashboardWindowTrackerWorkspace *inWorkspace)
{
	EsdashboardWindowsViewPrivate			*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(inWorkspace==NULL || ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace));

	priv=ESDASHBOARD_WINDOWS_VIEW(self)->priv;

	/* Check if parent stage interface or workspace changed. If both have not
	 * changed do nothing and return immediately.
	 */
	if(!_esdashboard_windows_view_update_stage_and_monitor(self) &&
		inWorkspace==priv->workspace)
	{
		return;
	}

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Set new workspace if changed */
	if(priv->workspace!=inWorkspace)
	{
		/* Set new workspace */
		priv->workspace=inWorkspace;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowsViewProperties[PROP_WORKSPACE]);
	}

	/* Recreate all window actors */
	_esdashboard_windows_view_recreate_window_actors(self);

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}

/* A scroll event occured in workspace selector (e.g. by mouse-wheel) */
static gboolean _esdashboard_windows_view_on_scroll_event(ClutterActor *inActor,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	EsdashboardWindowsView					*self;
	EsdashboardWindowsViewPrivate			*priv;
	gint									direction;
	gint									workspace;
	gint									maxWorkspace;
	EsdashboardWindowTrackerWorkspace		*activeWorkspace;
	EsdashboardWindowTrackerWorkspace		*newWorkspace;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inActor), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inEvent, CLUTTER_EVENT_PROPAGATE);

	self=ESDASHBOARD_WINDOWS_VIEW(inActor);
	priv=self->priv;

	/* Do not handle event if scroll event of mouse-wheel should not
	 * change workspace. In this case propagate event to get it handled
	 * by next actor in chain.
	 */
	if(!priv->isScrollEventChangingWorkspace) return(CLUTTER_EVENT_PROPAGATE);

	/* Get direction of scroll event */
	switch(clutter_event_get_scroll_direction(inEvent))
	{
		case CLUTTER_SCROLL_UP:
		case CLUTTER_SCROLL_LEFT:
			direction=-1;
			break;

		case CLUTTER_SCROLL_DOWN:
		case CLUTTER_SCROLL_RIGHT:
			direction=1;
			break;

		/* Unhandled directions */
		default:
			ESDASHBOARD_DEBUG(self, ACTOR,
								"Cannot handle scroll direction %d in %s",
								clutter_event_get_scroll_direction(inEvent),
								G_OBJECT_TYPE_NAME(self));
			return(CLUTTER_EVENT_PROPAGATE);
	}

	/* Get next workspace in scroll direction */
	activeWorkspace=esdashboard_window_tracker_get_active_workspace(priv->windowTracker);
	maxWorkspace=esdashboard_window_tracker_get_workspaces_count(priv->windowTracker);

	workspace=esdashboard_window_tracker_workspace_get_number(activeWorkspace)+direction;
	if(workspace<0 || workspace>=maxWorkspace) return(CLUTTER_EVENT_STOP);

	/* Activate new workspace */
	newWorkspace=esdashboard_window_tracker_get_workspace_by_number(priv->windowTracker, workspace);
	esdashboard_window_tracker_workspace_activate(newWorkspace);

	return(CLUTTER_EVENT_STOP);
}

/* Set flag if scroll events (e.g. mouse-wheel up or down) should change active workspace
 * and set up scroll event listener or remove an existing one.
 */
static void _esdashboard_windows_view_set_scroll_event_changes_workspace(EsdashboardWindowsView *self, gboolean inMouseWheelChangingWorkspace)
{
	EsdashboardWindowsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->isScrollEventChangingWorkspace!=inMouseWheelChangingWorkspace)
	{
		/* Set value */
		priv->isScrollEventChangingWorkspace=inMouseWheelChangingWorkspace;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowsViewProperties[PROP_SCROLL_EVENT_CHANGES_WORKSPACE]);
	}
}

/* Set flag if this view should show all windows of all monitors or only the windows
 * which are at the monitor where this view is placed at.
 */
static void _esdashboard_windows_view_set_filter_monitor_windows(EsdashboardWindowsView *self, gboolean inFilterMonitorWindows)
{
	EsdashboardWindowsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->filterMonitorWindows!=inFilterMonitorWindows)
	{
		/* Set value */
		priv->filterMonitorWindows=inFilterMonitorWindows;

		/* Recreate all window actors */
		_esdashboard_windows_view_recreate_window_actors(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowsViewProperties[PROP_FILTER_MONITOR_WINDOWS]);
	}
}

/* Set flag if this view should show all windows of all workspaces or only the windows
 * which are at current workspace.
 */
static void _esdashboard_windows_view_set_filter_workspace_windows(EsdashboardWindowsView *self, gboolean inFilterWorkspaceWindows)
{
	EsdashboardWindowsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->filterWorkspaceWindows!=inFilterWorkspaceWindows)
	{
		/* Set value */
		priv->filterWorkspaceWindows=inFilterWorkspaceWindows;

		/* Recreate all window actors */
		_esdashboard_windows_view_recreate_window_actors(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowsViewProperties[PROP_FILTER_WORKSPACE_WINDOWS]);
	}
}

/* Action signal to close currently selected window was emitted */
static gboolean _esdashboard_windows_view_window_close(EsdashboardWindowsView *self,
														EsdashboardFocusable *inSource,
														const gchar *inAction,
														ClutterEvent *inEvent)
{
	EsdashboardWindowsViewPrivate			*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inSource), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Check if a window is currenly selected */
	if(!priv->selectedItem)
	{
		ESDASHBOARD_DEBUG(self, ACTOR, "No window to close is selected.");
		return(CLUTTER_EVENT_STOP);
	}

	/* Close selected window */
	_esdashboard_windows_view_on_window_close_clicked(self, ESDASHBOARD_LIVE_WINDOW(priv->selectedItem));

	/* We handled this event */
	return(CLUTTER_EVENT_STOP);
}

/* Action signal to show window numbers was emitted */
static gboolean _esdashboard_windows_view_windows_show_numbers(EsdashboardWindowsView *self,
																EsdashboardFocusable *inSource,
																const gchar *inAction,
																ClutterEvent *inEvent)
{
	EsdashboardWindowsViewPrivate			*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inSource), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* If window numbers are already shown do nothing */
	if(priv->isWindowsNumberShown) return(CLUTTER_EVENT_PROPAGATE);

	/* Set flag that window numbers are shown already
	 * to prevent do it twice concurrently.
	 */
	priv->isWindowsNumberShown=TRUE;

	/* Show window numbers */
	_esdashboard_windows_view_update_window_number_in_actors(self);

	/* Action handled but do not prevent further processing */
	return(CLUTTER_EVENT_PROPAGATE);
}

/* Action signal to hide window numbers was emitted */
static gboolean _esdashboard_windows_view_windows_hide_numbers(EsdashboardWindowsView *self,
																EsdashboardFocusable *inSource,
																const gchar *inAction,
																ClutterEvent *inEvent)
{
	EsdashboardWindowsViewPrivate			*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inSource), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* If no window numbers are shown do nothing */
	if(!priv->isWindowsNumberShown) return(CLUTTER_EVENT_PROPAGATE);

	/* Set flag that window numbers are hidden already
	 * to prevent do it twice concurrently.
	 */
	priv->isWindowsNumberShown=FALSE;

	/* Hide window numbers */
	_esdashboard_windows_view_update_window_number_in_actors(self);

	/* Action handled but do not prevent further processing */
	return(CLUTTER_EVENT_PROPAGATE);
}

/* Action signal to hide window numbers was emitted */
static gboolean _esdashboard_windows_view_windows_activate_window_by_number(EsdashboardWindowsView *self,
																				guint inWindowNumber)
{
	ClutterActor						*child;
	ClutterActorIter					iter;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), CLUTTER_EVENT_PROPAGATE);

	/* Iterate through list of current actors and at each live window actor
	 * check if its window number matches the requested one. If it does
	 * activate this window.
	 */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		guint							windowNumber;

		/* Only live window actors can be handled */
		if(!ESDASHBOARD_IS_LIVE_WINDOW(child)) continue;

		/* Get window number set at live window actor */
		windowNumber=0;
		g_object_get(child, "window-number", &windowNumber, NULL);

		/* If window number at live window actor matches requested one
		 * activate this window.
		 */
		if(windowNumber==inWindowNumber)
		{
			/* Activate window */
			_esdashboard_windows_view_on_window_clicked(self, ESDASHBOARD_LIVE_WINDOW(child));

			/* Action was handled */
			return(CLUTTER_EVENT_STOP);
		}
	}

	/* If we get here the requested window was not found
	 * so this action could not be handled by this actor.
	 */
	return(CLUTTER_EVENT_PROPAGATE);
}

static gboolean _esdashboard_windows_view_windows_activate_window_one(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 1));
}

static gboolean _esdashboard_windows_view_windows_activate_window_two(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 2));
}

static gboolean _esdashboard_windows_view_windows_activate_window_three(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 3));
}

static gboolean _esdashboard_windows_view_windows_activate_window_four(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 4));
}

static gboolean _esdashboard_windows_view_windows_activate_window_five(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 5));
}

static gboolean _esdashboard_windows_view_windows_activate_window_six(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 6));
}

static gboolean _esdashboard_windows_view_windows_activate_window_seven(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 7));
}

static gboolean _esdashboard_windows_view_windows_activate_window_eight(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 8));
}

static gboolean _esdashboard_windows_view_windows_activate_window_nine(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 9));
}

static gboolean _esdashboard_windows_view_windows_activate_window_ten(EsdashboardWindowsView *self,
																		EsdashboardFocusable *inSource,
																		const gchar *inAction,
																		ClutterEvent *inEvent)
{
	return(_esdashboard_windows_view_windows_activate_window_by_number(self, 10));
}

/* IMPLEMENTATION: Interface EsdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _esdashboard_windows_view_focusable_can_focus(EsdashboardFocusable *inFocusable)
{
	EsdashboardWindowsView			*self;
	EsdashboardFocusableInterface	*selfIface;
	EsdashboardFocusableInterface	*parentIface;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inFocusable), FALSE);

	self=ESDASHBOARD_WINDOWS_VIEW(inFocusable);

	/* Call parent class interface function */
	selfIface=ESDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->can_focus)
	{
		if(!parentIface->can_focus(inFocusable)) return(FALSE);
	}

	/* If this view is not enabled it is not focusable */
	if(!esdashboard_view_get_enabled(ESDASHBOARD_VIEW(self))) return(FALSE);

	/* If we get here this actor can be focused */
	return(TRUE);
}

/* Actor lost focus */
static void _esdashboard_windows_view_focusable_unset_focus(EsdashboardFocusable *inFocusable)
{
	EsdashboardWindowsView			*self;
	EsdashboardFocusableInterface	*selfIface;
	EsdashboardFocusableInterface	*parentIface;

	g_return_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable));
	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inFocusable));

	self=ESDASHBOARD_WINDOWS_VIEW(inFocusable);

	/* Call parent class interface function */
	selfIface=ESDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->unset_focus)
	{
		parentIface->unset_focus(inFocusable);
	}

	/* Actor lost focus so ensure window numbers are hiding again */
	_esdashboard_windows_view_windows_hide_numbers(self, ESDASHBOARD_FOCUSABLE(self), NULL, NULL);
}

/* Determine if this actor supports selection */
static gboolean _esdashboard_windows_view_focusable_supports_selection(EsdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}

/* Get current selection */
static ClutterActor* _esdashboard_windows_view_focusable_get_selection(EsdashboardFocusable *inFocusable)
{
	EsdashboardWindowsView					*self;
	EsdashboardWindowsViewPrivate			*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inFocusable), NULL);

	self=ESDASHBOARD_WINDOWS_VIEW(inFocusable);
	priv=self->priv;

	/* Return current selection */
	return(priv->selectedItem);
}

/* Set new selection */
static gboolean _esdashboard_windows_view_focusable_set_selection(EsdashboardFocusable *inFocusable,
																	ClutterActor *inSelection)
{
	EsdashboardWindowsView					*self;
	EsdashboardWindowsViewPrivate			*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_WINDOWS_VIEW(inFocusable);
	priv=self->priv;

	/* Check that selection is a child of this actor */
	if(inSelection &&
		!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		g_warning("%s is not a child of %s and cannot be selected",
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Remove weak reference at current selection */
	if(priv->selectedItem)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
	}

	/* Set new selection */
	priv->selectedItem=inSelection;

	/* Add weak reference at new selection */
	if(priv->selectedItem) g_object_add_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);

	/* New selection was set successfully */
	return(TRUE);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _esdashboard_windows_view_focusable_find_selection(EsdashboardFocusable *inFocusable,
																			ClutterActor *inSelection,
																			EsdashboardSelectionTarget inDirection)
{
	EsdashboardWindowsView					*self;
	EsdashboardWindowsViewPrivate			*priv;
	ClutterActor							*selection;
	ClutterActor							*newSelection;
	gint									numberChildren;
	gint									rows;
	gint									columns;
	gint									currentSelectionIndex;
	gint									currentSelectionRow;
	gint									currentSelectionColumn;
	gint									newSelectionIndex;
	ClutterActorIter						iter;
	ClutterActor							*child;
	gchar									*valueName;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inFocusable), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	self=ESDASHBOARD_WINDOWS_VIEW(inFocusable);
	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;

	/* If there is nothing selected, select first actor and return */
	if(!inSelection)
	{
		newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(self));

		valueName=esdashboard_get_enum_value_name(ESDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
		ESDASHBOARD_DEBUG(self, ACTOR,
							"No selection at %s, so select first child %s for direction %s",
							G_OBJECT_TYPE_NAME(self),
							newSelection ? G_OBJECT_TYPE_NAME(newSelection) : "<nil>",
							valueName);
		g_free(valueName);

		return(newSelection);
	}

	/* Check that selection is a child of this actor otherwise return NULL */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning("Cannot lookup selection target at %s because %s is a child of %s",
					G_OBJECT_TYPE_NAME(self),
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>");

		return(NULL);
	}

	/* Get number of rows and columns and also get number of children
	 * of layout manager.
	 */
	numberChildren=esdashboard_scaled_table_layout_get_number_children(ESDASHBOARD_SCALED_TABLE_LAYOUT(priv->layout));
	rows=esdashboard_scaled_table_layout_get_rows(ESDASHBOARD_SCALED_TABLE_LAYOUT(priv->layout));
	columns=esdashboard_scaled_table_layout_get_columns(ESDASHBOARD_SCALED_TABLE_LAYOUT(priv->layout));

	/* Get index of current selection */
	currentSelectionIndex=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child) &&
			child!=inSelection)
	{
		currentSelectionIndex++;
	}

	currentSelectionRow=(currentSelectionIndex / columns);
	currentSelectionColumn=(currentSelectionIndex % columns);

	/* Find target selection */
	switch(inDirection)
	{
		case ESDASHBOARD_SELECTION_TARGET_LEFT:
			currentSelectionColumn--;
			if(currentSelectionColumn<0)
			{
				currentSelectionRow++;
				newSelectionIndex=(currentSelectionRow*columns)-1;
			}
				else newSelectionIndex=currentSelectionIndex-1;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_RIGHT:
			currentSelectionColumn++;
			if(currentSelectionColumn==columns ||
				currentSelectionIndex==numberChildren)
			{
				newSelectionIndex=(currentSelectionRow*columns);
			}
				else newSelectionIndex=currentSelectionIndex+1;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_UP:
			currentSelectionRow--;
			if(currentSelectionRow<0) currentSelectionRow=rows-1;
			newSelectionIndex=(currentSelectionRow*columns)+currentSelectionColumn;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_DOWN:
			currentSelectionRow++;
			if(currentSelectionRow>=rows) currentSelectionRow=0;
			newSelectionIndex=(currentSelectionRow*columns)+currentSelectionColumn;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_FIRST:
			newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(self));
			break;

		case ESDASHBOARD_SELECTION_TARGET_LAST:
			newSelection=clutter_actor_get_last_child(CLUTTER_ACTOR(self));
			break;

		case ESDASHBOARD_SELECTION_TARGET_NEXT:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			if(!newSelection) newSelection=clutter_actor_get_previous_sibling(inSelection);
			break;

		case ESDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
			newSelectionIndex=(currentSelectionRow*columns);
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
			newSelectionIndex=((currentSelectionRow+1)*columns)-1;
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_PAGE_UP:
			newSelectionIndex=currentSelectionColumn;
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			newSelectionIndex=((rows-1)*columns)+currentSelectionColumn;
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		default:
			{
				valueName=esdashboard_get_enum_value_name(ESDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical("Focusable object %s does not handle selection direction of type %s.",
							G_OBJECT_TYPE_NAME(self),
							valueName);
				g_free(valueName);
			}
			break;
	}

	/* If new selection could be found override current selection with it */
	if(newSelection) selection=newSelection;

	/* Return new selection found */
	ESDASHBOARD_DEBUG(self, ACTOR,
						"Selecting %s at %s for current selection %s in direction %u",
						selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
						G_OBJECT_TYPE_NAME(self),
						inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
						inDirection);

	return(selection);
}

/* Activate selection */
static gboolean _esdashboard_windows_view_focusable_activate_selection(EsdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	EsdashboardWindowsView					*self;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inFocusable), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_WINDOWS_VIEW(inFocusable);

	/* Check that selection is a child of this actor */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning("%s is a child of %s and cannot be activated at %s",
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>",
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Check that child is a live window */
	if(!ESDASHBOARD_IS_LIVE_WINDOW(inSelection))
	{
		g_warning("Cannot activate selection of type %s at %s because expecting type %s",
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self),
					g_type_name(ESDASHBOARD_TYPE_LIVE_WINDOW));

		return(FALSE);
	}

	/* Activate selection means clicking on window */
	_esdashboard_windows_view_on_window_clicked(self, ESDASHBOARD_LIVE_WINDOW(inSelection));

	return(TRUE);
}

/* Interface initialization
 * Set up default functions
 */
void _esdashboard_windows_view_focusable_iface_init(EsdashboardFocusableInterface *iface)
{
	iface->can_focus=_esdashboard_windows_view_focusable_can_focus;
	iface->unset_focus=_esdashboard_windows_view_focusable_unset_focus;

	iface->supports_selection=_esdashboard_windows_view_focusable_supports_selection;
	iface->get_selection=_esdashboard_windows_view_focusable_get_selection;
	iface->set_selection=_esdashboard_windows_view_focusable_set_selection;
	iface->find_selection=_esdashboard_windows_view_focusable_find_selection;
	iface->activate_selection=_esdashboard_windows_view_focusable_activate_selection;
}

/* IMPLEMENTATION: ClutterActor */

/* Actor will be mapped */
static void _esdashboard_windows_view_map(ClutterActor *inActor)
{
	EsdashboardWindowsView			*self;
	EsdashboardWindowsViewPrivate	*priv;
	ClutterActorClass				*clutterActorClass;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inActor));

	self=ESDASHBOARD_WINDOWS_VIEW(inActor);
	priv=self->priv;

	/* Call parent's virtual function */
	clutterActorClass=CLUTTER_ACTOR_CLASS(esdashboard_windows_view_parent_class);
	if(clutterActorClass->map) clutterActorClass->map(inActor);

	/* Disconnect signal handler if available */
	if(priv->scrollEventChangingWorkspaceStage)
	{
		if(priv->scrollEventChangingWorkspaceStageSignalID)
		{
			g_signal_handler_disconnect(priv->scrollEventChangingWorkspaceStage, priv->scrollEventChangingWorkspaceStageSignalID);
			priv->scrollEventChangingWorkspaceStageSignalID=0;
		}

		priv->scrollEventChangingWorkspaceStage=NULL;
	}

	/* Get stage interface where this actor belongs to and connect
	 * signal handler if found.
	 */
	priv->scrollEventChangingWorkspaceStage=esdashboard_get_stage_of_actor(CLUTTER_ACTOR(self));
	if(priv->scrollEventChangingWorkspaceStage)
	{
		priv->scrollEventChangingWorkspaceStageSignalID=g_signal_connect_swapped(priv->scrollEventChangingWorkspaceStage,
																					"scroll-event",
																					G_CALLBACK(_esdashboard_windows_view_on_scroll_event),
																					self);
	}
}

/* Actor will be unmapped */
static void _esdashboard_windows_view_unmap(ClutterActor *inActor)
{
	EsdashboardWindowsView			*self;
	EsdashboardWindowsViewPrivate	*priv;
	ClutterActorClass				*clutterActorClass;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(inActor));

	self=ESDASHBOARD_WINDOWS_VIEW(inActor);
	priv=self->priv;

	/* Call parent's virtual function */
	clutterActorClass=CLUTTER_ACTOR_CLASS(esdashboard_windows_view_parent_class);
	if(clutterActorClass->unmap) clutterActorClass->unmap(inActor);

	/* Disconnect signal handler if available */
	if(priv->scrollEventChangingWorkspaceStage)
	{
		if(priv->scrollEventChangingWorkspaceStageSignalID)
		{
			g_signal_handler_disconnect(priv->scrollEventChangingWorkspaceStage, priv->scrollEventChangingWorkspaceStageSignalID);
			priv->scrollEventChangingWorkspaceStageSignalID=0;
		}

		priv->scrollEventChangingWorkspaceStage=NULL;
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_windows_view_dispose(GObject *inObject)
{
	EsdashboardWindowsView			*self=ESDASHBOARD_WINDOWS_VIEW(inObject);
	EsdashboardWindowsViewPrivate	*priv=ESDASHBOARD_WINDOWS_VIEW(self)->priv;

	/* Release allocated resources */
	if(priv->selectedItem)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
		priv->selectedItem=NULL;
	}

	if(priv->scrollEventChangingWorkspaceStage)
	{
		if(priv->scrollEventChangingWorkspaceStageSignalID)
		{
			g_signal_handler_disconnect(priv->scrollEventChangingWorkspaceStage, priv->scrollEventChangingWorkspaceStageSignalID);
			priv->scrollEventChangingWorkspaceStageSignalID=0;
		}

		priv->scrollEventChangingWorkspaceStage=NULL;
	}

	if(priv->esconfChannel)
	{
		priv->esconfChannel=NULL;
	}

	if(priv->esconfScrollEventChangingWorkspaceBindingID)
	{
		esconf_g_property_unbind(priv->esconfScrollEventChangingWorkspaceBindingID);
		priv->esconfScrollEventChangingWorkspaceBindingID=0;
	}

	if(priv->workspace)
	{
		_esdashboard_windows_view_set_active_workspace(self, NULL);
	}

	if(priv->layout)
	{
		priv->layout=NULL;
	}

	if(priv->currentMonitor)
	{
		priv->currentMonitor=NULL;
	}

	if(priv->currentStage)
	{
		if(priv->currentStageMonitorBindingID)
		{
			g_signal_handler_disconnect(priv->currentStage, priv->currentStageMonitorBindingID);
			priv->currentStageMonitorBindingID=0;
		}

		priv->currentStage=NULL;
	}

	if(priv->windowTracker)
	{
		g_signal_handlers_disconnect_by_data(priv->windowTracker, self);
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_windows_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_windows_view_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardWindowsView		*self=ESDASHBOARD_WINDOWS_VIEW(inObject);
	
	switch(inPropID)
	{
		case PROP_WORKSPACE:
			_esdashboard_windows_view_set_active_workspace(self, g_value_get_object(inValue));
			break;

		case PROP_SPACING:
			esdashboard_windows_view_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_PREVENT_UPSCALING:
			esdashboard_windows_view_set_prevent_upscaling(self, g_value_get_boolean(inValue));
			break;

		case PROP_SCROLL_EVENT_CHANGES_WORKSPACE:
			_esdashboard_windows_view_set_scroll_event_changes_workspace(self, g_value_get_boolean(inValue));
			break;

		case PROP_FILTER_MONITOR_WINDOWS:
			_esdashboard_windows_view_set_filter_monitor_windows(self, g_value_get_boolean(inValue));
			break;

		case PROP_FILTER_WORKSPACE_WINDOWS:
			_esdashboard_windows_view_set_filter_workspace_windows(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_windows_view_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardWindowsView			*self=ESDASHBOARD_WINDOWS_VIEW(inObject);
	EsdashboardWindowsViewPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_WORKSPACE:
			g_value_set_object(outValue, priv->workspace);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_PREVENT_UPSCALING:
			g_value_set_boolean(outValue, self->priv->preventUpscaling);
			break;

		case PROP_SCROLL_EVENT_CHANGES_WORKSPACE:
			g_value_set_boolean(outValue, self->priv->isScrollEventChangingWorkspace);
			break;

		case PROP_FILTER_MONITOR_WINDOWS:
			g_value_set_boolean(outValue, self->priv->filterMonitorWindows);
			break;

		case PROP_FILTER_WORKSPACE_WINDOWS:
			g_value_set_boolean(outValue, self->priv->filterWorkspaceWindows);
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
static void esdashboard_windows_view_class_init(EsdashboardWindowsViewClass *klass)
{
	EsdashboardActorClass	*actorClass=ESDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_windows_view_dispose;
	gobjectClass->set_property=_esdashboard_windows_view_set_property;
	gobjectClass->get_property=_esdashboard_windows_view_get_property;

	clutterActorClass->map=_esdashboard_windows_view_map;
	clutterActorClass->unmap=_esdashboard_windows_view_unmap;

	klass->window_close=_esdashboard_windows_view_window_close;
	klass->windows_show_numbers=_esdashboard_windows_view_windows_show_numbers;
	klass->windows_hide_numbers=_esdashboard_windows_view_windows_hide_numbers;
	klass->windows_activate_window_one=_esdashboard_windows_view_windows_activate_window_one;
	klass->windows_activate_window_two=_esdashboard_windows_view_windows_activate_window_two;
	klass->windows_activate_window_three=_esdashboard_windows_view_windows_activate_window_three;
	klass->windows_activate_window_four=_esdashboard_windows_view_windows_activate_window_four;
	klass->windows_activate_window_five=_esdashboard_windows_view_windows_activate_window_five;
	klass->windows_activate_window_six=_esdashboard_windows_view_windows_activate_window_six;
	klass->windows_activate_window_seven=_esdashboard_windows_view_windows_activate_window_seven;
	klass->windows_activate_window_eight=_esdashboard_windows_view_windows_activate_window_eight;
	klass->windows_activate_window_nine=_esdashboard_windows_view_windows_activate_window_nine;
	klass->windows_activate_window_ten=_esdashboard_windows_view_windows_activate_window_ten;

	/* Define properties */
	EsdashboardWindowsViewProperties[PROP_WORKSPACE]=
		g_param_spec_object("workspace",
							"Current workspace",
							"The current workspace whose windows are shown",
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowsViewProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							"Spacing",
							"Spacing between each element in view",
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowsViewProperties[PROP_PREVENT_UPSCALING]=
		g_param_spec_boolean("prevent-upscaling",
								"Prevent upscaling",
								"Whether this view should prevent upsclaing any window beyond its real size",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowsViewProperties[PROP_SCROLL_EVENT_CHANGES_WORKSPACE]=
		g_param_spec_boolean("scroll-event-changes-workspace",
								"Scroll event changes workspace",
								"Whether this view should change active workspace on scroll events",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowsViewProperties[PROP_FILTER_MONITOR_WINDOWS]=
		g_param_spec_boolean("filter-monitor-windows",
								"Filter monitor windows",
								"Whether this view should only show windows of monitor where it placed at",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowsViewProperties[PROP_FILTER_WORKSPACE_WINDOWS]=
		g_param_spec_boolean("filter-workspace-windows",
								"Filter workspace windows",
								"Whether this view should only show windows of active workspace",
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardWindowsViewProperties);

	/* Define stylable properties */
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardWindowsViewProperties[PROP_SPACING]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardWindowsViewProperties[PROP_PREVENT_UPSCALING]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardWindowsViewProperties[PROP_FILTER_MONITOR_WINDOWS]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardWindowsViewProperties[PROP_FILTER_WORKSPACE_WINDOWS]);

	/* Define actions */
	EsdashboardWindowsViewSignals[ACTION_WINDOW_CLOSE]=
		g_signal_new("window-close",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, window_close),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_SHOW_NUMBERS]=
		g_signal_new("windows-show-numbers",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_show_numbers),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_HIDE_NUMBERS]=
		g_signal_new("windows-hide-numbers",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_hide_numbers),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_ONE]=
		g_signal_new("windows-activate-window-one",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_one),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_TWO]=
		g_signal_new("windows-activate-window-two",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_two),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_THREE]=
		g_signal_new("windows-activate-window-three",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_three),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_FOUR]=
		g_signal_new("windows-activate-window-four",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_four),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_FIVE]=
		g_signal_new("windows-activate-window-five",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_five),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_SIX]=
		g_signal_new("windows-activate-window-six",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_six),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_SEVEN]=
		g_signal_new("windows-activate-window-seven",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_seven),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_EIGHT]=
		g_signal_new("windows-activate-window-eight",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_eight),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_NINE]=
		g_signal_new("windows-activate-window-nine",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_nine),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardWindowsViewSignals[ACTION_WINDOWS_ACTIVATE_WINDOW_TEN]=
		g_signal_new("windows-activate-window-ten",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardWindowsViewClass, windows_activate_window_ten),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_windows_view_init(EsdashboardWindowsView *self)
{
	EsdashboardWindowsViewPrivate		*priv;
	ClutterAction						*action;
	EsdashboardWindowTrackerWorkspace	*activeWorkspace;

	self->priv=priv=esdashboard_windows_view_get_instance_private(self);

	/* Set up default values */
	priv->windowTracker=esdashboard_window_tracker_get_default();
	priv->workspace=NULL;
	priv->spacing=0.0f;
	priv->preventUpscaling=FALSE;
	priv->selectedItem=NULL;
	priv->isWindowsNumberShown=FALSE;
	priv->esconfChannel=esdashboard_application_get_esconf_channel(NULL);
	priv->isScrollEventChangingWorkspace=FALSE;
	priv->scrollEventChangingWorkspaceStage=NULL;
	priv->scrollEventChangingWorkspaceStageSignalID=0;
	priv->filterMonitorWindows=FALSE;
	priv->filterWorkspaceWindows=TRUE;
	priv->currentStage=NULL;
	priv->currentMonitor=NULL;
	priv->currentStageMonitorBindingID=0;

	/* Set up view */
	esdashboard_view_set_name(ESDASHBOARD_VIEW(self), _("Windows"));
	esdashboard_view_set_icon(ESDASHBOARD_VIEW(self), DEFAULT_VIEW_ICON);
	esdashboard_view_set_view_fit_mode(ESDASHBOARD_VIEW(self), ESDASHBOARD_VIEW_FIT_MODE_BOTH);

	/* Setup actor */
	esdashboard_actor_set_can_focus(ESDASHBOARD_ACTOR(self), TRUE);

	priv->layout=esdashboard_scaled_table_layout_new();
	esdashboard_scaled_table_layout_set_relative_scale(ESDASHBOARD_SCALED_TABLE_LAYOUT(priv->layout), TRUE);
	esdashboard_scaled_table_layout_set_prevent_upscaling(ESDASHBOARD_SCALED_TABLE_LAYOUT(priv->layout), priv->preventUpscaling);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);

	action=esdashboard_drop_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), action);
	g_signal_connect_swapped(action, "begin", G_CALLBACK(_esdashboard_windows_view_on_drop_begin), self);
	g_signal_connect_swapped(action, "drop", G_CALLBACK(_esdashboard_windows_view_on_drop_drop), self);

	/* Bind to esconf to react on changes */
	priv->esconfScrollEventChangingWorkspaceBindingID=esconf_g_property_bind(priv->esconfChannel,
																				SCROLL_EVENT_CHANGES_WORKSPACE_ESCONF_PROP,
																				G_TYPE_BOOLEAN,
																				self,
																				"scroll-event-changes-workspace");

	/* Connect signals */
	g_signal_connect(self,
						"actor-added",
						G_CALLBACK(_esdashboard_windows_view_on_child_added),
						NULL);

	g_signal_connect(self,
						"actor-removed",
						G_CALLBACK(_esdashboard_windows_view_on_child_removed),
						NULL);

	g_signal_connect_swapped(priv->windowTracker,
								"active-workspace-changed",
								G_CALLBACK(_esdashboard_windows_view_on_active_workspace_changed),
								self);

	g_signal_connect_swapped(priv->windowTracker,
								"window-workspace-changed",
								G_CALLBACK(_esdashboard_windows_view_on_window_workspace_changed),
								self);

	g_signal_connect_swapped(priv->windowTracker,
								"window-opened",
								G_CALLBACK(_esdashboard_windows_view_on_window_opened),
								self);

	g_signal_connect_swapped(priv->windowTracker,
								"window-monitor-changed",
								G_CALLBACK(_esdashboard_windows_view_on_window_monitor_changed),
								self);

	/* If active workspace is already available then set up this view */
	activeWorkspace=esdashboard_window_tracker_get_active_workspace(priv->windowTracker);
	if(activeWorkspace)
	{
		_esdashboard_windows_view_set_active_workspace(self, activeWorkspace);
	}
}

/* IMPLEMENTATION: Public API */

/* Get/set spacing between elements */
gfloat esdashboard_windows_view_get_spacing(EsdashboardWindowsView *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), 0.0f);

	return(self->priv->spacing);
}

void esdashboard_windows_view_set_spacing(EsdashboardWindowsView *self, const gfloat inSpacing)
{
	EsdashboardWindowsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set value */
		priv->spacing=inSpacing;

		/* Update layout manager */
		if(priv->layout)
		{
			esdashboard_scaled_table_layout_set_spacing(ESDASHBOARD_SCALED_TABLE_LAYOUT(priv->layout), priv->spacing);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowsViewProperties[PROP_SPACING]);
	}
}

/* Get/set if layout manager should prevent to size any child larger than its real size */
gboolean esdashboard_windows_view_get_prevent_upscaling(EsdashboardWindowsView *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self), FALSE);

	return(self->priv->preventUpscaling);
}

void esdashboard_windows_view_set_prevent_upscaling(EsdashboardWindowsView *self, gboolean inPreventUpscaling)
{
	EsdashboardWindowsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOWS_VIEW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->preventUpscaling!=inPreventUpscaling)
	{
		/* Set value */
		priv->preventUpscaling=inPreventUpscaling;

		/* Update layout manager */
		if(priv->layout)
		{
			esdashboard_scaled_table_layout_set_prevent_upscaling(ESDASHBOARD_SCALED_TABLE_LAYOUT(priv->layout), priv->preventUpscaling);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowsViewProperties[PROP_PREVENT_UPSCALING]);
	}
}
