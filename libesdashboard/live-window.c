/*
 * live-window: An actor showing the content of a window which will
 *              be updated if changed and visible on active workspace.
 *              It also provides controls to manipulate it.
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

#include <libesdashboard/live-window.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtk.h>
#include <math.h>

#include <libesdashboard/button.h>
#include <libesdashboard/stage.h>
#include <libesdashboard/click-action.h>
#include <libesdashboard/window-content.h>
#include <libesdashboard/image-content.h>
#include <libesdashboard/stylable.h>
#include <libesdashboard/application.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
struct _EsdashboardLiveWindowPrivate
{
	/* Properties related */
	guint								windowNumber;

	gfloat								paddingClose;
	gfloat								paddingTitle;

	gboolean							showSubwindows;
	gboolean							allowSubwindows;

	/* Instance related */
	EsdashboardWindowTracker			*windowTracker;

	ClutterActor						*actorSubwindowsLayer;
	ClutterActor						*actorControlLayer;
	ClutterActor						*actorClose;
	ClutterActor						*actorWindowNumber;
	ClutterActor						*actorTitle;

	EsconfChannel						*esconfChannel;
	guint								esconfAllowSubwindowsBindingID;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardLiveWindow,
							esdashboard_live_window,
							ESDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE)

/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW_NUMBER,

	PROP_CLOSE_BUTTON_PADDING,
	PROP_TITLE_ACTOR_PADDING,

	PROP_SHOW_SUBWINDOWS,
	PROP_ALLOW_SUBWINDOWS,

	PROP_LAST
};

static GParamSpec* EsdashboardLiveWindowProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CLICKED,
	SIGNAL_CLOSE,

	SIGNAL_LAST
};

static guint EsdashboardLiveWindowSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define ALLOW_SUBWINDOWS_ESCONF_PROP						"/allow-subwindows"
#define DEFAULT_ALLOW_SUBWINDOWS							TRUE

/* Check if the requested window is a sub-window of this window */
static gboolean _esdashboard_live_window_is_subwindow(EsdashboardLiveWindow *self,
														EsdashboardWindowTrackerWindow *inWindow)
{
	EsdashboardWindowTrackerWindow		*thisWindow;
	EsdashboardWindowTrackerWindow		*requestedWindowParent;

	g_return_val_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), FALSE);

	/* Check if window opened belongs to this window (is transient for this one) */
	thisWindow=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(self));
	if(!thisWindow) return(FALSE);

	requestedWindowParent=esdashboard_window_tracker_window_get_parent(inWindow);
	if(!requestedWindowParent || requestedWindowParent!=thisWindow) return(FALSE);

	/* All checks succeeded so it is a sub-window and we return TRUE here */
	return(TRUE);
}

/* Check if requested sub-window should be displayed */
static gboolean _esdashboard_live_window_should_display_subwindow(EsdashboardLiveWindow *self,
																	EsdashboardWindowTrackerWindow *inWindow)
{
	EsdashboardWindowTrackerWindowState		state;

	g_return_val_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), FALSE);

	/* Check if window opened belong to this window (is transient for this one) */
	if(!_esdashboard_live_window_is_subwindow(self, inWindow)) return(FALSE);

	/* Check if window opened is visible */
	if(!esdashboard_window_tracker_window_is_visible(inWindow)) return(FALSE);

	/* Check if window is either pinned. If it is not then check if it is on the
	 * same workspace as its parent window. We can do this simple check because
	 * the window opened is transient window of its parent and it looks like it
	 * will inherit the same "pin" state.
	 */
	state=esdashboard_window_tracker_window_get_state(inWindow);
	if(!(state & ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED))
	{
		EsdashboardWindowTrackerWindow		*thisWindow;
		EsdashboardWindowTrackerWorkspace	*workspace;

		thisWindow=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(self));
		workspace=esdashboard_window_tracker_window_get_workspace(thisWindow);
		if(workspace && !esdashboard_window_tracker_window_is_on_workspace(inWindow, workspace)) return(FALSE);
	}

	/* All checks passed and we should display this sub-window, so return TRUE */
	return(TRUE);

}

/* Find actor for requested sub-window */
static ClutterActor* _esdashboard_live_window_find_subwindow_actor(EsdashboardLiveWindow *self,
																	EsdashboardWindowTrackerWindow *inWindow)
{
	EsdashboardLiveWindowPrivate		*priv;
	ClutterActorIter					iter;
	ClutterActor						*child;
	EsdashboardWindowTrackerWindow		*childWindow;

	g_return_val_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	priv=self->priv;

	/* Iterate through actors at sub-windows layer and return the actor handling
	 * the requested window.
	 */
	if(priv->actorSubwindowsLayer)
	{
		clutter_actor_iter_init(&iter, priv->actorSubwindowsLayer);
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Skip actor not handling live windows */
			if(!ESDASHBOARD_IS_LIVE_WINDOW_SIMPLE(child)) continue;

			/* Check if this actor handles the requested window */
			childWindow=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(child));
			if(childWindow==inWindow) return(child);
		}
	}

	/* If we get here, we did not find any actor handling the requested window.
	 * So return NULL.
	 */
	return(NULL);
}

/* A sub-window changed workspace, so check if this sub-window should not be
 * shown anymore and find associated actor to destroy.
 */
static void _esdashboard_live_window_on_subwindow_actor_workspace_changed(EsdashboardLiveWindow *self,
																			gpointer inUserData)
{
	EsdashboardWindowTrackerWindow		*window;
	ClutterActor						*actor;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inUserData));

	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW(inUserData);

	/* Check if we should display this window at all. If it should still be
	 * display, return immediately.
	 */
	if(_esdashboard_live_window_should_display_subwindow(self, window)) return;

	/* This window should not displayed anymore. Find associated actor and
	 * destroy it.
	 */
	actor=_esdashboard_live_window_find_subwindow_actor(self, window);
	if(actor) esdashboard_actor_destroy(actor);
}

/* A sub-window changed its state, so check if this sub-window should not be
 * shown anymore and find associated actor to destroy it
 */
static void _esdashboard_live_window_on_subwindow_actor_state_changed(EsdashboardLiveWindow *self,
																		EsdashboardWindowTrackerWindowState inOldState,
																		gpointer inUserData)
{
	EsdashboardWindowTrackerWindow		*window;
	ClutterActor						*actor;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inUserData));

	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW(inUserData);

	/* Check if we should display this window at all. If it should still be
	 * display, return immediately.
	 */
	if(_esdashboard_live_window_should_display_subwindow(self, window)) return;

	/* This window should not displayed anymore. Find associated actor and
	 * destroy it.
	 */
	actor=_esdashboard_live_window_find_subwindow_actor(self, window);
	if(actor) esdashboard_actor_destroy(actor);
}

/* A sub-window actor is going to be destroyed, so clean up */
static void _esdashboard_live_window_on_subwindow_actor_destroyed(EsdashboardLiveWindow *self,
																	gpointer inUserData)
{
	ClutterActor						*actor;
	EsdashboardWindowTrackerWindow		*window;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	actor=CLUTTER_ACTOR(inUserData);

	/* Check if the actor going to be destroyed is an actor showing live windows */
	if(!ESDASHBOARD_IS_LIVE_WINDOW_SIMPLE(actor)) return;

	/* Get associated window of the live window actor going to be destroyed */
	window=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(actor));
	if(!window) return;

	/* Disconnect signals to prevent them get called even when this actor does
	 * not exist anymore.
	 */
	g_signal_handlers_disconnect_by_func(window, _esdashboard_live_window_on_subwindow_actor_workspace_changed, self);
	g_signal_handlers_disconnect_by_func(window, _esdashboard_live_window_on_subwindow_actor_state_changed, self);
}

/* A window was opened and might be a sub-window of this one and should be shown */
static void _esdashboard_live_window_on_subwindow_opened(EsdashboardLiveWindow *self,
															EsdashboardWindowTrackerWindow *inWindow,
															gpointer inUserData)
{
	EsdashboardLiveWindowPrivate			*priv;
	ClutterActor							*actor;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if we should display this window at all */
	if(!_esdashboard_live_window_should_display_subwindow(self, inWindow)) return;

	/* Before adding an actor for this window, check if an actor already exists */
	if(_esdashboard_live_window_find_subwindow_actor(self, inWindow)) return;

	/* Add child to this window */
	actor=esdashboard_live_window_simple_new_for_window(inWindow);
	clutter_actor_set_reactive(actor, FALSE);
	clutter_actor_show(actor);
	clutter_actor_add_child(priv->actorSubwindowsLayer, actor);

	/* Connect signals */
	g_signal_connect_swapped(actor, "destroy", G_CALLBACK(_esdashboard_live_window_on_subwindow_actor_destroyed), self);

	g_signal_connect_swapped(inWindow, "workspace-changed", G_CALLBACK(_esdashboard_live_window_on_subwindow_actor_workspace_changed), self);
	g_signal_connect_swapped(inWindow, "state-changed", G_CALLBACK(_esdashboard_live_window_on_subwindow_actor_state_changed), self);
}

/* A window has changed workspace and might be a sub-window of this one
 * which should be shown.
 */
static void _esdashboard_live_window_on_subwindow_workspace_changed(EsdashboardLiveWindow *self,
																	EsdashboardWindowTrackerWindow *inWindow,
																	EsdashboardWindowTrackerWorkspace *inWorkspace,
																	gpointer inUserData)
{
	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	/* Just call signal handler hanlding new windows opened because it will
	 * perform all needed checks to determine if this window is a sub-window and
	 * should be shown. It will also create the actor needed.
	 * In case the window moved away from the workspace the signal handler
	 * connect to the window directly will perform all check to determine if
	 * this window should not be displayed anymore and destroy the associated
	 * actor in this case. So not need to check this here.
	 */
	_esdashboard_live_window_on_subwindow_opened(self, inWindow, NULL);
}

/* This actor was clicked */
static void _esdashboard_live_window_on_clicked(EsdashboardLiveWindow *self,
												ClutterActor *inActor,
												gpointer inUserData)
{
	EsdashboardLiveWindowPrivate	*priv;
	EsdashboardClickAction			*action;
	gfloat							eventX, eventY;
	gfloat							relX, relY;
	ClutterActorBox					closeBox;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(inUserData));

	priv=self->priv;
	action=ESDASHBOARD_CLICK_ACTION(inUserData);

	/* Only emit any of these signals if click was perform with left button 
	 * or is a short touchscreen touch event.
	 */
	if(!esdashboard_click_action_is_left_button_or_tap(action))
	{
		return;
	}

	/* Check if click happened in "close button" */
	if(clutter_actor_is_visible(priv->actorClose))
	{
		esdashboard_click_action_get_coords(action, &eventX, &eventY);
		if(clutter_actor_transform_stage_point(CLUTTER_ACTOR(self), eventX, eventY, &relX, &relY))
		{
			clutter_actor_get_allocation_box(priv->actorClose, &closeBox);
			if(clutter_actor_box_contains(&closeBox, relX, relY))
			{
				g_signal_emit(self, EsdashboardLiveWindowSignals[SIGNAL_CLOSE], 0);
				return;
			}
		}
	}

	/* Emit "clicked" signal */
	g_signal_emit(self, EsdashboardLiveWindowSignals[SIGNAL_CLICKED], 0);
}

/* Action items of window has changed */
static void _esdashboard_live_window_on_actions_changed(EsdashboardLiveWindow *self,
														EsdashboardWindowTrackerWindow *inWindow,
														gpointer inUserData)
{
	EsdashboardLiveWindowPrivate			*priv;
	gboolean								currentCloseVisible;
	gboolean								newCloseVisible;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if signal is for this window */
	if(inWindow!=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(self))) return;

	/* Determine current and new state of actions */
	currentCloseVisible=(clutter_actor_is_visible(priv->actorClose) ? TRUE : FALSE);
	newCloseVisible=((esdashboard_window_tracker_window_get_actions(inWindow) & ESDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION_CLOSE) ? TRUE : FALSE);

	/* Show or hide close button actor */
	if(newCloseVisible!=currentCloseVisible)
	{
		if(newCloseVisible) clutter_actor_show(priv->actorClose);
			else clutter_actor_hide(priv->actorClose);
	}
}

/* Icon of window has changed */
static void _esdashboard_live_window_on_icon_changed(EsdashboardLiveWindow *self,
														EsdashboardWindowTrackerWindow *inWindow,
														gpointer inUserData)
{
	EsdashboardLiveWindowPrivate	*priv;
	ClutterContent					*icon;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if signal is for this window */
	if(inWindow!=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(self))) return;

	/* Set new icon in title actor */
	icon=esdashboard_image_content_new_for_pixbuf(esdashboard_window_tracker_window_get_icon(inWindow));
	esdashboard_label_set_icon_image(ESDASHBOARD_LABEL(priv->actorTitle), CLUTTER_IMAGE(icon));
	g_object_unref(icon);
}

/* Title of window has changed */
static void _esdashboard_live_window_on_name_changed(EsdashboardLiveWindow *self,
														EsdashboardWindowTrackerWindow *inWindow,
														gpointer inUserData)
{
	EsdashboardLiveWindowPrivate	*priv;
	gchar							*windowName;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if signal is for this window */
	if(inWindow!=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(self))) return;

	/* Set new name in title actor */
	windowName=g_markup_printf_escaped("%s", esdashboard_window_tracker_window_get_name(inWindow));
	esdashboard_label_set_text(ESDASHBOARD_LABEL(priv->actorTitle), windowName);
	g_free(windowName);
}

/* Window number will be modified */
static void _esdashboard_live_window_set_window_number(EsdashboardLiveWindow *self,
														guint inWindowNumber)

{
	EsdashboardLiveWindowPrivate	*priv;
	EsdashboardWindowTrackerWindow	*window;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(inWindowNumber<=10);

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowNumber!=inWindowNumber)
	{
		/* Set value */
		priv->windowNumber=inWindowNumber;

		/* If window number is non-zero hide close button and
		 * show window number instead ...
		 */
		if(priv->windowNumber>0)
		{
			gchar					*numberText;

			/* Update text in window number */
			numberText=g_markup_printf_escaped("%u", priv->windowNumber % 10);
			esdashboard_label_set_text(ESDASHBOARD_LABEL(priv->actorWindowNumber), numberText);
			g_free(numberText);

			/* Show window number and hide close button */
			clutter_actor_show(priv->actorWindowNumber);
			clutter_actor_hide(priv->actorClose);
		}
			/* ... otherwise hide window number and show close button again
			 * if possible which depends on windows state.
			 */
			else
			{
				/* Get window this actor is responsible for */
				window=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(self));

				/* Only show close button again if window supports close action */
				if(esdashboard_window_tracker_window_get_actions(window) & ESDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION_CLOSE)
				{
					clutter_actor_show(priv->actorClose);
				}
				clutter_actor_hide(priv->actorWindowNumber);
			}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardLiveWindowProperties[PROP_WINDOW_NUMBER]);
	}
}

/* Set up sub-windows layer by destryoing all children and re-adding actors for
 * each associated sub-window.
 */
static void _esdashboard_live_window_setup_subwindows_layer(EsdashboardLiveWindow *self)
{
	EsdashboardLiveWindowPrivate	*priv;
	GList							*windowList;
	EsdashboardWindowTrackerWindow	*subwindow;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));

	priv=self->priv;

	/* Do not setup sub-windows layer if there is no such layer */
	if(!priv->actorSubwindowsLayer) return;

	/* Destroy all sub-windows and do not create sub-windows actor if showing
	 * them was disabled.
	 */
	esdashboard_actor_destroy_all_children(priv->actorSubwindowsLayer);
	if(!priv->allowSubwindows || !priv->showSubwindows) return;

	/* Create sub-window actors for the windows belonging to this one */
	windowList=esdashboard_window_tracker_get_windows_stacked(priv->windowTracker);
	for( ; windowList; windowList=g_list_next(windowList))
	{
		/* Get window at current position of iterator */
		subwindow=ESDASHBOARD_WINDOW_TRACKER_WINDOW(windowList->data);
		if(!subwindow) continue;

		/* Call signal handler for the event when a window is opened. It will
		 * check if this window is a visible child of this window and it will
		 * create the actor if needed.
		 */
		_esdashboard_live_window_on_subwindow_opened(self, subwindow, priv->windowTracker);
	}
}

/* Window property changed so set up controls, title and icon */
static void _esdashboard_live_window_on_window_changed(GObject *inObject,
														GParamSpec *inSpec,
														gpointer inUserData)
{
	EsdashboardLiveWindow			*self;
	EsdashboardLiveWindowPrivate	*priv;
	EsdashboardWindowTrackerWindow	*window;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(inObject));

	self=ESDASHBOARD_LIVE_WINDOW(inObject);
	priv=self->priv;

	/* Get new window set */
	window=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(self));

	/* Set up this actor and child actor by calling each signal handler now */
	_esdashboard_live_window_on_actions_changed(self, window, priv->windowTracker);
	_esdashboard_live_window_on_icon_changed(self, window, priv->windowTracker);
	_esdashboard_live_window_on_name_changed(self, window, priv->windowTracker);

	/* Set up sub-windows layer */
	_esdashboard_live_window_setup_subwindows_layer(self);
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _esdashboard_live_window_get_preferred_height(ClutterActor *self,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	EsdashboardLiveWindowPrivate	*priv=ESDASHBOARD_LIVE_WINDOW(self)->priv;
	gfloat							minHeight, naturalHeight;
	gfloat							childMinHeight, childNaturalHeight;
	ClutterActorIter				iter;
	ClutterActor					*child;

	minHeight=naturalHeight=0.0f;

	/* Chain up to determine size of window of this actor (should usually be the largest actor) */
	CLUTTER_ACTOR_CLASS(esdashboard_live_window_parent_class)->get_preferred_height(self, inForWidth, &minHeight, &naturalHeight);

	/* Determine size of each sub-window */
	clutter_actor_iter_init(&iter, priv->actorSubwindowsLayer);
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(clutter_actor_is_visible(child))
		{
			clutter_actor_get_preferred_height(child,
												inForWidth,
												&childMinHeight,
												&childNaturalHeight);
			if(childMinHeight>minHeight) minHeight=childMinHeight;
			if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
		}
	}

	/* Determine size of title actor if visible */
	if(clutter_actor_is_visible(priv->actorTitle))
	{
		clutter_actor_get_preferred_height(priv->actorTitle,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);
		childMinHeight+=(2*priv->paddingTitle);
		childNaturalHeight+=(2*priv->paddingTitle);
		if(childMinHeight>minHeight) minHeight=childMinHeight;
		if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
	}

	/* Determine size of close button actor if visible */
	if(clutter_actor_is_visible(priv->actorClose))
	{
		clutter_actor_get_preferred_height(priv->actorClose,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);
		childMinHeight+=(2*priv->paddingClose);
		childNaturalHeight+=(2*priv->paddingClose);
		if(childMinHeight>minHeight) minHeight=childMinHeight;
		if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
	}

	/* Determine size of window number button actor if visible */
	if(clutter_actor_is_visible(priv->actorWindowNumber))
	{
		clutter_actor_get_preferred_height(priv->actorWindowNumber,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);
		childMinHeight+=(2*priv->paddingClose);
		childNaturalHeight+=(2*priv->paddingClose);
		if(childMinHeight>minHeight) minHeight=childMinHeight;
		if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
	}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _esdashboard_live_window_get_preferred_width(ClutterActor *self,
															gfloat inForHeight,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth)
{
	EsdashboardLiveWindowPrivate	*priv=ESDASHBOARD_LIVE_WINDOW(self)->priv;
	gfloat							minWidth, naturalWidth;
	gfloat							childMinWidth, childNaturalWidth;
	ClutterActorIter				iter;
	ClutterActor					*child;

	minWidth=naturalWidth=0.0f;

	/* Chain up to determine size of window of this actor (should usually be the largest actor) */
	CLUTTER_ACTOR_CLASS(esdashboard_live_window_parent_class)->get_preferred_width(self, inForHeight, &minWidth, &naturalWidth);

	/* Determine size of each sub-window */
	clutter_actor_iter_init(&iter, priv->actorSubwindowsLayer);
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(clutter_actor_is_visible(child))
		{
			clutter_actor_get_preferred_width(child,
												inForHeight,
												&childMinWidth,
												&childNaturalWidth);
			if(childMinWidth>minWidth) minWidth=childMinWidth;
			if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
		}
	}

	/* Determine size of title actor if visible */
	if(clutter_actor_is_visible(priv->actorTitle))
	{
		clutter_actor_get_preferred_width(priv->actorTitle,
											inForHeight,
											&childMinWidth,
											 &childNaturalWidth);
		childMinWidth+=(2*priv->paddingTitle);
		childNaturalWidth+=(2*priv->paddingTitle);
		if(childMinWidth>minWidth) minWidth=childMinWidth;
		if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
	}

	/* Determine size of close button actor if visible */
	if(clutter_actor_is_visible(priv->actorClose))
	{
		clutter_actor_get_preferred_width(priv->actorClose,
											inForHeight,
											&childMinWidth,
											&childNaturalWidth);
		childMinWidth+=(2*priv->paddingClose);
		childNaturalWidth+=(2*priv->paddingClose);
		if(childMinWidth>minWidth) minWidth=childMinWidth;
		if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
	}

	/* Determine size of window number button actor if visible */
	if(clutter_actor_is_visible(priv->actorWindowNumber))
	{
		clutter_actor_get_preferred_width(priv->actorWindowNumber,
											inForHeight,
											&childMinWidth,
											&childNaturalWidth);
		childMinWidth+=(2*priv->paddingClose);
		childNaturalWidth+=(2*priv->paddingClose);
		if(childMinWidth>minWidth) minWidth=childMinWidth;
		if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
	}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _esdashboard_live_window_allocate(ClutterActor *self,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	EsdashboardLiveWindowPrivate		*priv=ESDASHBOARD_LIVE_WINDOW(self)->priv;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(esdashboard_live_window_parent_class)->allocate(self, inBox, inFlags);

	/* Set allocation of sub-windows layer if available */
	if(priv->actorSubwindowsLayer)
	{
		ClutterActorBox					boxActorLayer;
		ClutterActorBox					boxActorChild;
		ClutterActorIter				iter;
		ClutterActor					*child;
		EsdashboardWindowTrackerWindow	*window;
		gfloat							largestWidth, largestHeight;
		gfloat							childWidth, childHeight;
		gfloat							scaleWidth, scaleHeight;
		gfloat							layerWidth, layerHeight;
		gfloat							left, right, top, bottom;

		/* Calculate and set allocation at each sub-window on sub-windows layer.
		 * They need to be scaled down and to keep their aspect ratio to fit into
		 * allocation of sub-windows layer.
		 */
		largestWidth=largestHeight=0.0f;

		window=esdashboard_live_window_simple_get_window(ESDASHBOARD_LIVE_WINDOW_SIMPLE(self));
		if(window)
		{
			gint						windowWidth, windowHeight;

			esdashboard_window_tracker_window_get_geometry(window, NULL, NULL, &windowWidth, &windowHeight);
			if(windowWidth>largestWidth) largestWidth=windowWidth;
			if(windowHeight>largestHeight) largestHeight=windowHeight;
		}

		clutter_actor_iter_init(&iter, priv->actorSubwindowsLayer);
		while(clutter_actor_iter_next(&iter, &child))
		{
			if(clutter_actor_is_visible(child))
			{
				clutter_actor_get_preferred_size(child, NULL, NULL, &childWidth, &childHeight);
				if(childWidth>largestWidth) largestWidth=childWidth;
				if(childHeight>largestHeight) largestHeight=childHeight;
			}
		}

		/* Set allocation on sub-windows layer which covers the largest width and
		 * largest height of all windows (including main window of this actor itself)
		 * scaled down to fit into this actor's allocation and centered position.
		 */
		scaleWidth=clutter_actor_box_get_width(inBox)/largestWidth;
		scaleHeight=clutter_actor_box_get_height(inBox)/largestHeight;

		layerWidth=largestWidth*scaleWidth;
		left=(clutter_actor_box_get_width(inBox)-layerWidth)/2.0f;
		right=left+layerWidth;

		layerHeight=largestHeight*scaleHeight;
		top=(clutter_actor_box_get_height(inBox)-layerHeight)/2.0f;
		bottom=top+layerHeight;

		clutter_actor_box_init(&boxActorLayer, floor(left), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(priv->actorSubwindowsLayer, &boxActorLayer, inFlags);

		clutter_actor_iter_init(&iter, priv->actorSubwindowsLayer);
		while(clutter_actor_iter_next(&iter, &child))
		{
			if(clutter_actor_is_visible(child))
			{
				/* Get natural size of sub-window */
				clutter_actor_get_preferred_size(child, NULL, NULL, &childWidth, &childHeight);

				/* Scale down to fit size of allocation of sub-windows layer */
				childWidth=childWidth*scaleWidth;
				childHeight=childHeight*scaleHeight;

				/* Center sub-window in allocation of sub-windows layer */
				left=(layerWidth-childWidth)/2.0f;
				right=left+childWidth;

				top=(layerHeight-childHeight)/2.0f;
				bottom=top+childHeight;

				/* Set allocation of sub-window */
				clutter_actor_box_init(&boxActorChild, floor(left), floor(top), floor(right), floor(bottom));
				clutter_actor_allocate(child, &boxActorChild, inFlags);
			}
		}
	}

	/* Set allocation of controls layer if available */
	if(priv->actorControlLayer)
	{
		ClutterActorBox					boxActorLayer;
		ClutterActorBox					boxActorClose;
		ClutterActorBox					boxActorWindowNumber;
		ClutterActorBox					boxActorTitle;
		ClutterActorBox					*referedBoxActor;
		gfloat							layerWidth, layerHeight;
		gfloat							closeWidth, closeHeight;
		gfloat							windowNumberWidth, windowNumberHeight;
		gfloat							titleWidth, titleHeight;
		gfloat							left, top, right, bottom;
		gfloat							maxWidth;

		/* Set allocation on control layer which matches the actor's allocation at
		 * width and height but with origin position.
		 */
		clutter_actor_box_get_size(inBox, &layerWidth, &layerHeight);
		clutter_actor_box_init(&boxActorLayer, 0.0f, 0.0f, layerWidth, layerHeight);
		clutter_actor_allocate(priv->actorControlLayer, &boxActorLayer, inFlags);

		/* Set allocation on close actor */
		clutter_actor_get_preferred_size(priv->actorClose, NULL, NULL, &closeWidth, &closeHeight);

		right=clutter_actor_box_get_x(&boxActorLayer)+clutter_actor_box_get_width(&boxActorLayer)-priv->paddingClose;
		left=MAX(right-closeWidth, priv->paddingClose);
		top=clutter_actor_box_get_y(&boxActorLayer)+priv->paddingClose;
		bottom=top+closeHeight;

		right=MAX(left, right);
		bottom=MAX(top, bottom);

		clutter_actor_box_init(&boxActorClose, floor(left), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(priv->actorClose, &boxActorClose, inFlags);

		/* Set allocation on window number actor (expand to size of close button if needed) */
		clutter_actor_get_preferred_size(priv->actorWindowNumber, NULL, NULL, &windowNumberWidth, &windowNumberHeight);

		right=clutter_actor_box_get_x(&boxActorLayer)+clutter_actor_box_get_width(&boxActorLayer)-priv->paddingClose;
		left=MAX(right-windowNumberWidth, priv->paddingClose);
		top=clutter_actor_box_get_y(&boxActorLayer)+priv->paddingClose;
		bottom=top+windowNumberHeight;

		left=MIN(left, clutter_actor_box_get_x(&boxActorClose));
		right=MAX(left, right);
		bottom=MAX(top, bottom);
		bottom=MAX(bottom, clutter_actor_box_get_y(&boxActorClose)+clutter_actor_box_get_height(&boxActorClose));

		clutter_actor_box_init(&boxActorWindowNumber, floor(left), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(priv->actorWindowNumber, &boxActorWindowNumber, inFlags);

		/* Set allocation on title actor
		 * But prevent that title overlaps close button
		 */
		if(priv->windowNumber>0) referedBoxActor=&boxActorWindowNumber;
			else referedBoxActor=&boxActorClose;

		clutter_actor_get_preferred_size(priv->actorTitle, NULL, NULL, &titleWidth, &titleHeight);

		maxWidth=clutter_actor_box_get_width(&boxActorLayer)-(2*priv->paddingTitle);
		if(titleWidth>maxWidth) titleWidth=maxWidth;

		left=clutter_actor_box_get_x(&boxActorLayer)+((clutter_actor_box_get_width(&boxActorLayer)-titleWidth)/2.0f);
		right=left+titleWidth;
		bottom=clutter_actor_box_get_y(&boxActorLayer)+clutter_actor_box_get_height(&boxActorLayer)-(2*priv->paddingTitle);
		top=bottom-titleHeight;
		if(left>right) left=right-1.0f;
		if(top<(clutter_actor_box_get_y(referedBoxActor)+clutter_actor_box_get_height(referedBoxActor)))
		{
			if(right>=clutter_actor_box_get_x(referedBoxActor))
			{
				right=clutter_actor_box_get_x(referedBoxActor)-MIN(priv->paddingTitle, priv->paddingClose);
			}

			if(top<clutter_actor_box_get_y(referedBoxActor))
			{
				top=clutter_actor_box_get_y(referedBoxActor);
				bottom=top+titleHeight;
			}
		}

		right=MAX(left, right);
		bottom=MAX(top, bottom);

		clutter_actor_box_init(&boxActorTitle, floor(left), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(priv->actorTitle, &boxActorTitle, inFlags);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_live_window_dispose(GObject *inObject)
{
	EsdashboardLiveWindow			*self=ESDASHBOARD_LIVE_WINDOW(inObject);
	EsdashboardLiveWindowPrivate	*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->windowTracker)
	{
		g_signal_handlers_disconnect_by_data(priv->windowTracker, self);
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->actorTitle)
	{
		clutter_actor_destroy(priv->actorTitle);
		priv->actorTitle=NULL;
	}

	if(priv->actorClose)
	{
		clutter_actor_destroy(priv->actorClose);
		priv->actorClose=NULL;
	}

	if(priv->actorWindowNumber)
	{
		clutter_actor_destroy(priv->actorWindowNumber);
		priv->actorWindowNumber=NULL;
	}

	if(priv->actorControlLayer)
	{
		clutter_actor_destroy(priv->actorControlLayer);
		priv->actorControlLayer=NULL;
	}

	if(priv->actorSubwindowsLayer)
	{
		clutter_actor_destroy(priv->actorSubwindowsLayer);
		priv->actorSubwindowsLayer=NULL;
	}

	if(priv->esconfChannel)
	{
		priv->esconfChannel=NULL;
	}

	if(priv->esconfAllowSubwindowsBindingID)
	{
		esconf_g_property_unbind(priv->esconfAllowSubwindowsBindingID);
		priv->esconfAllowSubwindowsBindingID=0;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_live_window_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_live_window_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardLiveWindow			*self=ESDASHBOARD_LIVE_WINDOW(inObject);
	
	switch(inPropID)
	{
		case PROP_WINDOW_NUMBER:
			_esdashboard_live_window_set_window_number(self, g_value_get_uint(inValue));
			break;

		case PROP_CLOSE_BUTTON_PADDING:
			esdashboard_live_window_set_close_button_padding(self, g_value_get_float(inValue));
			break;

		case PROP_TITLE_ACTOR_PADDING:
			esdashboard_live_window_set_title_actor_padding(self, g_value_get_float(inValue));
			break;

		case PROP_SHOW_SUBWINDOWS:
			esdashboard_live_window_set_show_subwindows(self, g_value_get_boolean(inValue));
			break;

		case PROP_ALLOW_SUBWINDOWS:
			esdashboard_live_window_set_allow_subwindows(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_live_window_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardLiveWindow	*self=ESDASHBOARD_LIVE_WINDOW(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW_NUMBER:
			g_value_set_uint(outValue, self->priv->windowNumber);
			break;

		case PROP_CLOSE_BUTTON_PADDING:
			g_value_set_float(outValue, self->priv->paddingClose);
			break;

		case PROP_TITLE_ACTOR_PADDING:
			g_value_set_float(outValue, self->priv->paddingTitle);
			break;

		case PROP_SHOW_SUBWINDOWS:
			g_value_set_boolean(outValue, self->priv->showSubwindows);
			break;

		case PROP_ALLOW_SUBWINDOWS:
			g_value_set_boolean(outValue, self->priv->allowSubwindows);
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
static void esdashboard_live_window_class_init(EsdashboardLiveWindowClass *klass)
{
	EsdashboardActorClass	*actorClass=ESDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	clutterActorClass->get_preferred_width=_esdashboard_live_window_get_preferred_width;
	clutterActorClass->get_preferred_height=_esdashboard_live_window_get_preferred_height;
	clutterActorClass->allocate=_esdashboard_live_window_allocate;

	gobjectClass->dispose=_esdashboard_live_window_dispose;
	gobjectClass->set_property=_esdashboard_live_window_set_property;
	gobjectClass->get_property=_esdashboard_live_window_get_property;

	/* Define properties */
	EsdashboardLiveWindowProperties[PROP_WINDOW_NUMBER]=
		g_param_spec_uint("window-number",
							"Window number",
							"The assigned window number. If set to non-zero the close button will be hidden and the window number will be shown instead. If set to zero the close button will be shown again.",
							0, 10,
							0,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardLiveWindowProperties[PROP_CLOSE_BUTTON_PADDING]=
		g_param_spec_float("close-padding",
							"Close button padding",
							"Padding of close button to window actor in pixels",
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardLiveWindowProperties[PROP_TITLE_ACTOR_PADDING]=
		g_param_spec_float("title-padding",
							"Title actor padding",
							"Padding of title actor to window actor in pixels",
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardLiveWindowProperties[PROP_SHOW_SUBWINDOWS]=
		g_param_spec_boolean("show-subwindows",
								"Show sub-windows",
								"Whether to show sub-windows of this main window",
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardLiveWindowProperties[PROP_ALLOW_SUBWINDOWS]=
		g_param_spec_boolean("allow-subwindows",
								"Allow sub-windows",
								"Whether to show sub-windows if requested by theme",
								DEFAULT_ALLOW_SUBWINDOWS,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardLiveWindowProperties);

	/* Define stylable properties */
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardLiveWindowProperties[PROP_CLOSE_BUTTON_PADDING]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardLiveWindowProperties[PROP_TITLE_ACTOR_PADDING]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardLiveWindowProperties[PROP_SHOW_SUBWINDOWS]);

	/* Define signals */
	EsdashboardLiveWindowSignals[SIGNAL_CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardLiveWindowClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	EsdashboardLiveWindowSignals[SIGNAL_CLOSE]=
		g_signal_new("close",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardLiveWindowClass, close),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_live_window_init(EsdashboardLiveWindow *self)
{
	EsdashboardLiveWindowPrivate	*priv;
	ClutterAction					*action;

	priv=self->priv=esdashboard_live_window_get_instance_private(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set default values */
	priv->windowTracker=esdashboard_window_tracker_get_default();
	priv->windowNumber=0;
	priv->paddingTitle=0.0f;
	priv->paddingClose=0.0f;
	priv->showSubwindows=TRUE;
	priv->esconfChannel=esdashboard_application_get_esconf_channel(NULL);
	priv->allowSubwindows=DEFAULT_ALLOW_SUBWINDOWS;

	/* Set up container for sub-windows and add it before the container for controls
	 * to keep the controls on top.
	 */
	priv->actorSubwindowsLayer=esdashboard_actor_new();
	esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(priv->actorSubwindowsLayer), "subwindows-layer");
	clutter_actor_set_reactive(priv->actorSubwindowsLayer, FALSE);
	clutter_actor_show(priv->actorSubwindowsLayer);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorSubwindowsLayer);

	/* Set up container for controls and add child actors (order is important) */
	priv->actorControlLayer=esdashboard_actor_new();
	esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(priv->actorControlLayer), "controls-layer");
	clutter_actor_set_reactive(priv->actorControlLayer, FALSE);
	clutter_actor_show(priv->actorControlLayer);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorControlLayer);

	priv->actorTitle=esdashboard_button_new();
	esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(priv->actorTitle), "title");
	clutter_actor_set_reactive(priv->actorTitle, FALSE);
	clutter_actor_show(priv->actorTitle);
	clutter_actor_add_child(priv->actorControlLayer, priv->actorTitle);

	priv->actorClose=esdashboard_button_new();
	esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(priv->actorClose), "close-button");
	clutter_actor_set_reactive(priv->actorClose, FALSE);
	clutter_actor_show(priv->actorClose);
	clutter_actor_add_child(priv->actorControlLayer, priv->actorClose);

	priv->actorWindowNumber=esdashboard_button_new();
	esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(priv->actorWindowNumber), "window-number");
	clutter_actor_set_reactive(priv->actorWindowNumber, FALSE);
	clutter_actor_hide(priv->actorWindowNumber);
	clutter_actor_add_child(priv->actorControlLayer, priv->actorWindowNumber);

	/* Bind to esconf to react on changes */
	priv->esconfAllowSubwindowsBindingID=esconf_g_property_bind(priv->esconfChannel,
																ALLOW_SUBWINDOWS_ESCONF_PROP,
																G_TYPE_BOOLEAN,
																self,
																"allow-subwindows");

	/* Connect signals */
	action=esdashboard_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), action);
	g_signal_connect_swapped(action, "clicked", G_CALLBACK(_esdashboard_live_window_on_clicked), self);

	g_signal_connect(self, "notify::window", G_CALLBACK(_esdashboard_live_window_on_window_changed), NULL);
	g_signal_connect_swapped(priv->windowTracker, "window-actions-changed", G_CALLBACK(_esdashboard_live_window_on_actions_changed), self);
	g_signal_connect_swapped(priv->windowTracker, "window-icon-changed", G_CALLBACK(_esdashboard_live_window_on_icon_changed), self);
	g_signal_connect_swapped(priv->windowTracker, "window-name-changed", G_CALLBACK(_esdashboard_live_window_on_name_changed), self);
	g_signal_connect_swapped(priv->windowTracker, "window-opened", G_CALLBACK(_esdashboard_live_window_on_subwindow_opened), self);
	g_signal_connect_swapped(priv->windowTracker, "window-workspace-changed", G_CALLBACK(_esdashboard_live_window_on_subwindow_workspace_changed), self);
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* esdashboard_live_window_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(ESDASHBOARD_TYPE_LIVE_WINDOW, NULL)));
}

ClutterActor* esdashboard_live_window_new_for_window(EsdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	return(CLUTTER_ACTOR(g_object_new(ESDASHBOARD_TYPE_LIVE_WINDOW,
										"window", inWindow,
										NULL)));
}

/* Get/set padding of title actor */
gfloat esdashboard_live_window_get_title_actor_padding(EsdashboardLiveWindow *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self), 0.0f);

	return(self->priv->paddingTitle);
}

void esdashboard_live_window_set_title_actor_padding(EsdashboardLiveWindow *self, gfloat inPadding)
{
	EsdashboardLiveWindowPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(inPadding>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->paddingTitle!=inPadding)
	{
		/* Set value */
		priv->paddingTitle=inPadding;
		esdashboard_background_set_corner_radius(ESDASHBOARD_BACKGROUND(priv->actorTitle), priv->paddingTitle);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardLiveWindowProperties[PROP_TITLE_ACTOR_PADDING]);
	}
}

/* Get/set padding of close button actor */
gfloat esdashboard_live_window_get_close_button_padding(EsdashboardLiveWindow *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self), 0.0f);

	return(self->priv->paddingClose);
}

void esdashboard_live_window_set_close_button_padding(EsdashboardLiveWindow *self, gfloat inPadding)
{
	EsdashboardLiveWindowPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(inPadding>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->paddingClose!=inPadding)
	{
		/* Set value */
		priv->paddingClose=inPadding;
		esdashboard_background_set_corner_radius(ESDASHBOARD_BACKGROUND(priv->actorClose), priv->paddingClose);
		esdashboard_background_set_corner_radius(ESDASHBOARD_BACKGROUND(priv->actorWindowNumber), priv->paddingClose);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardLiveWindowProperties[PROP_CLOSE_BUTTON_PADDING]);
	}
}

/* Get/set flag to show sub-windows */
gboolean esdashboard_live_window_get_show_subwindows(EsdashboardLiveWindow *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self), FALSE);

	return(self->priv->showSubwindows);
}

void esdashboard_live_window_set_show_subwindows(EsdashboardLiveWindow *self, gboolean inShowSubwindows)
{
	EsdashboardLiveWindowPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->showSubwindows!=inShowSubwindows)
	{
		/* Set value */
		priv->showSubwindows=inShowSubwindows;

		/* Set up sub-windows layer */
		_esdashboard_live_window_setup_subwindows_layer(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardLiveWindowProperties[PROP_SHOW_SUBWINDOWS]);
	}
}

/* Get/set flag to allow sub-windows at all */
gboolean esdashboard_live_window_get_allow_subwindows(EsdashboardLiveWindow *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self), FALSE);

	return(self->priv->allowSubwindows);
}

void esdashboard_live_window_set_allow_subwindows(EsdashboardLiveWindow *self, gboolean inAllowSubwindows)
{
	EsdashboardLiveWindowPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->allowSubwindows!=inAllowSubwindows)
	{
		/* Set value */
		priv->allowSubwindows=inAllowSubwindows;

		/* Set up sub-windows layer */
		_esdashboard_live_window_setup_subwindows_layer(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardLiveWindowProperties[PROP_ALLOW_SUBWINDOWS]);
	}
}
