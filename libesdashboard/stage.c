/*
 * stage: Global stage of application
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

#include <libesdashboard/stage.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <math.h>

#include <libesdashboard/application.h>
#include <libesdashboard/viewpad.h>
#include <libesdashboard/view-selector.h>
#include <libesdashboard/text-box.h>
#include <libesdashboard/quicklaunch.h>
#include <libesdashboard/applications-view.h>
#include <libesdashboard/windows-view.h>
#include <libesdashboard/search-view.h>
#include <libesdashboard/toggle-button.h>
#include <libesdashboard/workspace-selector.h>
#include <libesdashboard/collapse-box.h>
#include <libesdashboard/tooltip-action.h>
#include <libesdashboard/stylable.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/focus-manager.h>
#include <libesdashboard/enums.h>
#include <libesdashboard/window-tracker.h>
#include <libesdashboard/window-content.h>
#include <libesdashboard/stage-interface.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
struct _EsdashboardStagePrivate
{
	/* Properties related */
	EsdashboardStageBackgroundImageType		backgroundType;

	ClutterColor							*backgroundColor;

	/* Actors */
	ClutterActor							*backgroundImageLayer;
	ClutterActor							*backgroundColorLayer;

	gpointer								primaryInterface;
	gpointer								quicklaunch;
	gpointer								searchbox;
	gpointer								workspaces;
	gpointer								viewpad;
	gpointer								viewSelector;
	gpointer								notification;
	gpointer								tooltip;

	/* Instance related */
	EsdashboardWindowTracker				*windowTracker;
	EsdashboardWindowTrackerWindow			*stageWindow;

	gboolean								searchActive;
	gint									lastSearchTextLength;
	EsdashboardView							*viewBeforeSearch;
	gchar									*switchToView;
	gpointer								focusActorOnShow;

	guint									notificationTimeoutID;

	EsdashboardFocusManager					*focusManager;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardStage,
							esdashboard_stage,
							CLUTTER_TYPE_STAGE)

/* Properties */
enum
{
	PROP_0,

	PROP_BACKGROUND_IMAGE_TYPE,
	PROP_BACKGROUND_COLOR,

	PROP_SWITCH_TO_VIEW,

	PROP_LAST
};

static GParamSpec* EsdashboardStageProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ACTOR_CREATED,

	SIGNAL_SEARCH_STARTED,
	SIGNAL_SEARCH_CHANGED,
	SIGNAL_SEARCH_ENDED,

	SIGNAL_SHOW_TOOLTIP,
	SIGNAL_HIDE_TOOLTIP,

	SIGNAL_LAST
};

static guint EsdashboardStageSignals[SIGNAL_LAST]={ 0, };


/* Forward declaration */
static void _esdashboard_stage_on_window_opened(EsdashboardStage *self,
													EsdashboardWindowTrackerWindow *inWindow,
													gpointer inUserData);


/* IMPLEMENTATION: Private variables and methods */
#define NOTIFICATION_TIMEOUT_ESCONF_PROP				"/min-notification-timeout"
#define DEFAULT_NOTIFICATION_TIMEOUT					3000
#define RESET_SEARCH_ON_RESUME_ESCONF_PROP				"/reset-search-on-resume"
#define DEFAULT_RESET_SEARCH_ON_RESUME					TRUE
#define SWITCH_VIEW_ON_RESUME_ESCONF_PROP				"/switch-to-view-on-resume"
#define DEFAULT_SWITCH_VIEW_ON_RESUME					NULL
#define RESELECT_THEME_FOCUS_ON_RESUME_ESCONF_PROP		"/reselect-theme-focus-on-resume"
#define DEFAULT_RESELECT_THEME_FOCUS_ON_RESUME			FALSE
#define ESDASHBOARD_THEME_LAYOUT_PRIMARY				"primary"
#define ESDASHBOARD_THEME_LAYOUT_SECONDARY				"secondary"

typedef struct _EsdashboardStageThemeInterfaceData		EsdashboardStageThemeInterfaceData;
struct _EsdashboardStageThemeInterfaceData
{
	ClutterActor		*actor;
	GPtrArray			*focusables;
	ClutterActor		*focus;
};

/* Handle an event */
static gboolean _esdashboard_stage_event(ClutterActor *inActor, ClutterEvent *inEvent)
{
	EsdashboardStage			*self;
	EsdashboardStagePrivate		*priv;
	gboolean					result;

	g_return_val_if_fail(ESDASHBOARD_IS_STAGE(inActor), CLUTTER_EVENT_PROPAGATE);

	self=ESDASHBOARD_STAGE(inActor);
	priv=self->priv;

	/* Do only intercept any event if a focus manager is available */
	if(!priv->focusManager) return(CLUTTER_EVENT_PROPAGATE);

	/* Do only intercept "key-press" and "key-release" events */
	if(clutter_event_type(inEvent)!=CLUTTER_KEY_PRESS &&
		clutter_event_type(inEvent)!=CLUTTER_KEY_RELEASE)
	{
		return(CLUTTER_EVENT_PROPAGATE);
	}

	/* Handle key release event */
	if(clutter_event_type(inEvent)==CLUTTER_KEY_RELEASE)
	{
		/* Handle key */
		switch(inEvent->key.keyval)
		{
			/* Handle ESC key to clear search box or quit/suspend application */
			case CLUTTER_KEY_Escape:
			{
				/* If search is active then end search by clearing search box ... */
				if(priv->searchbox &&
					!esdashboard_text_box_is_empty(ESDASHBOARD_TEXT_BOX(priv->searchbox)))
				{
					esdashboard_text_box_set_text(ESDASHBOARD_TEXT_BOX(priv->searchbox), NULL);
					return(CLUTTER_EVENT_STOP);
				}
					/* ... otherwise quit application */
					else
					{
						esdashboard_application_suspend_or_quit(NULL);
						return(CLUTTER_EVENT_STOP);
					}
			}
			break;

			default:
				/* Fallthrough */
				break;
		}
	}

	/* Ask focus manager to handle this event */
	result=esdashboard_focus_manager_handle_key_event(priv->focusManager, inEvent, NULL);
	if(result==CLUTTER_EVENT_STOP) return(result);

	/* If even focus manager did not handle this event send this event to searchbox */
	if(priv->searchbox &&
		ESDASHBOARD_IS_FOCUSABLE(priv->searchbox) &&
		esdashboard_focus_manager_is_registered(priv->focusManager, ESDASHBOARD_FOCUSABLE(priv->searchbox)))
	{
		/* Ask search to handle this event if it has not the focus currently
		 * because in this case it has already handled the event and we do
		 * not to do this twice.
		 */
		if(esdashboard_focus_manager_get_focus(priv->focusManager)!=ESDASHBOARD_FOCUSABLE(priv->searchbox))
		{
			result=esdashboard_focus_manager_handle_key_event(priv->focusManager, inEvent, ESDASHBOARD_FOCUSABLE(priv->searchbox));
			if(result==CLUTTER_EVENT_STOP) return(result);
		}
	}

	/* If we get here there was no searchbox or it could not handle the event
	 * so stop further processing.
	 */
	return(CLUTTER_EVENT_STOP);
}

/* Get view to switch to by first looking upr temporary view ID set via command-line
 * and if not found or not set then looking up view ID configured via settings.
 */
static EsdashboardView* _esdashboard_stage_get_view_to_switch_to(EsdashboardStage *self)
{
	EsdashboardStagePrivate				*priv;
	EsdashboardView						*view;

	g_return_val_if_fail(ESDASHBOARD_IS_STAGE(self), NULL);

	priv=self->priv;
	view=NULL;

	/* First lookup view at private variable 'switchToView' which has higher
	 * priority as it is a temporary value and is usually set via command-line.
	 */
	if(priv->switchToView)
	{
		view=esdashboard_viewpad_find_view_by_id(ESDASHBOARD_VIEWPAD(priv->viewpad), priv->switchToView);
		if(!view) g_warning("Will not switch to unknown view '%s'", priv->switchToView);

		/* Regardless if we could find view by its internal name or not
		 * reset variable because the switch should happen once only.
		 */
		g_free(priv->switchToView);
		priv->switchToView=NULL;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardStageProperties[PROP_SWITCH_TO_VIEW]);
	}

	/* If we have not to switch to a specific view or if this view cannot be found
	 * then lookup the configured view in settings by its internal name
	 */
	if(!view)
	{
		gchar							*resumeViewID;

		/* Get view ID from settings and look up view */
		resumeViewID=esconf_channel_get_string(esdashboard_application_get_esconf_channel(NULL),
												SWITCH_VIEW_ON_RESUME_ESCONF_PROP,
												DEFAULT_SWITCH_VIEW_ON_RESUME);
		if(resumeViewID)
		{
			/* Lookup view by its ID set configured settings */
			view=esdashboard_viewpad_find_view_by_id(ESDASHBOARD_VIEWPAD(priv->viewpad), resumeViewID);
			if(!view) g_warning("Cannot switch to unknown view '%s'", resumeViewID);

			/* Release allocated resources */
			g_free(resumeViewID);
		}
	}

	/* Return view found */
	return(view);
}

/* Set focus in stage */
static void _esdashboard_stage_set_focus(EsdashboardStage *self)
{
	EsdashboardStagePrivate		*priv;
	EsdashboardFocusable		*actor;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));

	priv=self->priv;

	/* Set focus if no focus is set */
	actor=esdashboard_focus_manager_get_focus(priv->focusManager);
	if(!actor)
	{
		EsdashboardFocusable	*focusable;

		/* First try to set focus to searchbox ... */
		if(ESDASHBOARD_IS_FOCUSABLE(priv->searchbox) &&
			esdashboard_focusable_can_focus(ESDASHBOARD_FOCUSABLE(priv->searchbox)))
		{
			esdashboard_focus_manager_set_focus(priv->focusManager, ESDASHBOARD_FOCUSABLE(priv->searchbox));
		}
			/* ... then lookup first focusable actor */
			else
			{
				focusable=esdashboard_focus_manager_get_next_focusable(priv->focusManager, NULL);
				if(focusable) esdashboard_focus_manager_set_focus(priv->focusManager, focusable);
			}
	}
}

/* Stage got signal to show a tooltip */
static void _esdashboard_stage_show_tooltip(EsdashboardStage *self, ClutterAction *inAction)
{
	EsdashboardStagePrivate		*priv;
	EsdashboardTooltipAction	*tooltipAction;
	const gchar					*tooltipText;
	gfloat						tooltipX, tooltipY;
	gfloat						tooltipWidth, tooltipHeight;
	guint						cursorSize;
	gfloat						x, y;
	gfloat						stageWidth, stageHeight;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(inAction));
	g_return_if_fail(self->priv->tooltip);

	priv=self->priv;
	tooltipAction=ESDASHBOARD_TOOLTIP_ACTION(inAction);

	/* Hide tooltip while setup to avoid flicker */
	clutter_actor_hide(priv->tooltip);

	/* Get tooltip text and update text in tooltip actor */
	tooltipText=esdashboard_tooltip_action_get_text(tooltipAction);
	esdashboard_text_box_set_text(ESDASHBOARD_TEXT_BOX(priv->tooltip), tooltipText);

	/* Determine coordinates where to show tooltip at */
	esdashboard_tooltip_action_get_position(tooltipAction, &tooltipX, &tooltipY);
	clutter_actor_get_size(priv->tooltip, &tooltipWidth, &tooltipHeight);

	cursorSize=gdk_display_get_default_cursor_size(gdk_display_get_default());

	clutter_actor_get_size(CLUTTER_ACTOR(self), &stageWidth, &stageHeight);

	x=tooltipX+cursorSize;
	y=tooltipY+cursorSize;
	if((x+tooltipWidth)>stageWidth) x=tooltipX-tooltipWidth;
	if((y+tooltipHeight)>stageHeight) y=tooltipY-tooltipHeight;

	clutter_actor_set_position(priv->tooltip, floor(x), floor(y));

	/* Show tooltip */
	clutter_actor_show(priv->tooltip);
}

/* Stage got signal to hide tooltip */
static void _esdashboard_stage_hide_tooltip(EsdashboardStage *self, ClutterAction *inAction)
{
	EsdashboardStagePrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));

	priv=self->priv;

	/* Hide tooltip */
	clutter_actor_hide(priv->tooltip);
}

/* Notification timeout has been reached */
static void _esdashboard_stage_on_notification_timeout_destroyed(gpointer inUserData)
{
	EsdashboardStage			*self;
	EsdashboardStagePrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(inUserData));

	self=ESDASHBOARD_STAGE(inUserData);
	priv=self->priv;

	/* Timeout source was destroy so just reset ID to 0 */
	priv->notificationTimeoutID=0;
}

static gboolean _esdashboard_stage_on_notification_timeout(gpointer inUserData)
{
	EsdashboardStage			*self;
	EsdashboardStagePrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_STAGE(inUserData), G_SOURCE_REMOVE);

	self=ESDASHBOARD_STAGE(inUserData);
	priv=self->priv;

	/* Timeout reached so hide notification */
	clutter_actor_hide(priv->notification);

	/* Tell main context to remove this source */
	return(G_SOURCE_REMOVE);
}

/* App-button was toggled */
static void _esdashboard_stage_on_quicklaunch_apps_button_toggled(EsdashboardStage *self, gpointer inUserData)
{
	EsdashboardStagePrivate		*priv;
	EsdashboardToggleButton		*appsButton;
	gboolean					state;
	EsdashboardView				*view;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	priv=self->priv;
	appsButton=ESDASHBOARD_TOGGLE_BUTTON(inUserData);

	/* Get state of apps button */
	state=esdashboard_toggle_button_get_toggle_state(appsButton);

	/* Depending on state activate views */
	if(state==FALSE)
	{
		/* Find "windows-view" view and activate */
		view=esdashboard_viewpad_find_view_by_type(ESDASHBOARD_VIEWPAD(priv->viewpad), ESDASHBOARD_TYPE_WINDOWS_VIEW);
		if(view) esdashboard_viewpad_set_active_view(ESDASHBOARD_VIEWPAD(priv->viewpad), view);
	}
		else
		{
			/* Find "applications" or "search" view and activate */
			if(!priv->searchActive) view=esdashboard_viewpad_find_view_by_type(ESDASHBOARD_VIEWPAD(priv->viewpad), ESDASHBOARD_TYPE_APPLICATIONS_VIEW);
				else view=esdashboard_viewpad_find_view_by_type(ESDASHBOARD_VIEWPAD(priv->viewpad), ESDASHBOARD_TYPE_SEARCH_VIEW);
			if(view) esdashboard_viewpad_set_active_view(ESDASHBOARD_VIEWPAD(priv->viewpad), view);
		}
}

/* Text in search text-box has changed */
static void _esdashboard_stage_on_searchbox_text_changed(EsdashboardStage *self,
															gchar *inText,
															gpointer inUserData)
{
	EsdashboardStagePrivate		*priv;
	EsdashboardTextBox			*textBox=ESDASHBOARD_TEXT_BOX(inUserData);
	EsdashboardView				*searchView;
	gint						textLength;
	const gchar					*text;
	EsdashboardToggleButton		*appsButton;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_TEXT_BOX(inUserData));

	priv=self->priv;

	/* Get search view */
	searchView=esdashboard_viewpad_find_view_by_type(ESDASHBOARD_VIEWPAD(priv->viewpad), ESDASHBOARD_TYPE_SEARCH_VIEW);
	if(searchView==NULL)
	{
		g_critical("Cannot perform search because search view was not found in viewpad.");
		return;
	}

	/* Get text and length of text in text-box */
	text=esdashboard_text_box_get_text(textBox);
	textLength=esdashboard_text_box_get_length(textBox);

	/* Get apps button of quicklaunch */
	appsButton=esdashboard_quicklaunch_get_apps_button(ESDASHBOARD_QUICKLAUNCH(priv->quicklaunch));

	/* Check if current text length if greater than zero and previous text length
	 * was zero. If check is successful it marks the start of a search. Emit the
	 * "search-started" signal. There is no need to start a search a search over
	 * all search providers as it will be done later by updating search criteria.
	 * There is also no need to activate search view because we will ensure that
	 * search view is activate on any change in search text box but we enable that
	 * view to be able to activate it ;)
	 */
	if(textLength>0 && priv->lastSearchTextLength==0)
	{
		/* Remember current active view to restore it when search ended */
		priv->viewBeforeSearch=ESDASHBOARD_VIEW(g_object_ref(esdashboard_viewpad_get_active_view(ESDASHBOARD_VIEWPAD(priv->viewpad))));

		/* Enable search view and set focus to viewpad which will show the
		 * search view so this search view will get the focus finally
		 */
		esdashboard_view_set_enabled(searchView, TRUE);
		if(priv->viewpad && priv->focusManager)
		{
			esdashboard_focus_manager_set_focus(priv->focusManager, ESDASHBOARD_FOCUSABLE(priv->viewpad));
		}

		/* Activate "clear" button on text box */
		esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(priv->searchbox), "search-active");

		/* Change apps button appearance */
		if(appsButton) esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(appsButton), "search-active");

		/* Emit "search-started" signal */
		g_signal_emit(self, EsdashboardStageSignals[SIGNAL_SEARCH_STARTED], 0);
		priv->searchActive=TRUE;
	}

	/* Ensure that search view is active, emit signal for text changed,
	 * update search criteria and set active toggle state at apps button
	 */
	esdashboard_viewpad_set_active_view(ESDASHBOARD_VIEWPAD(priv->viewpad), searchView);
	esdashboard_search_view_update_search(ESDASHBOARD_SEARCH_VIEW(searchView), text);
	g_signal_emit(self, EsdashboardStageSignals[SIGNAL_SEARCH_CHANGED], 0, text);

	if(appsButton) esdashboard_toggle_button_set_toggle_state(appsButton, TRUE);

	/* Check if current text length is zero and previous text length was greater
	 * than zero. If check is successful it marks the end of current search. Emit
	 * the "search-ended" signal, reactivate view before search was started and
	 * disable search view.
	 */
	if(textLength==0 && priv->lastSearchTextLength>0)
	{
		/* Reactivate active view before search has started */
		if(priv->viewBeforeSearch)
		{
			esdashboard_viewpad_set_active_view(ESDASHBOARD_VIEWPAD(priv->viewpad), priv->viewBeforeSearch);
			g_object_unref(priv->viewBeforeSearch);
			priv->viewBeforeSearch=NULL;
		}

		/* Deactivate "clear" button on text box */
		esdashboard_stylable_remove_class(ESDASHBOARD_STYLABLE(priv->searchbox), "search-active");

		/* Disable search view */
		esdashboard_view_set_enabled(searchView, FALSE);

		/* Change apps button appearance */
		if(appsButton) esdashboard_stylable_remove_class(ESDASHBOARD_STYLABLE(appsButton), "search-active");

		/* Emit "search-ended" signal */
		g_signal_emit(self, EsdashboardStageSignals[SIGNAL_SEARCH_ENDED], 0);
		priv->searchActive=FALSE;
	}

	/* Trace text length changes */
	priv->lastSearchTextLength=textLength;
}

/* Secondary icon ("clear") on text box was clicked */
static void _esdashboard_stage_on_searchbox_secondary_icon_clicked(EsdashboardStage *self, gpointer inUserData)
{
	EsdashboardTextBox			*textBox;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_TEXT_BOX(inUserData));

	textBox=ESDASHBOARD_TEXT_BOX(inUserData);

	/* Clear search text box */
	esdashboard_text_box_set_text(textBox, NULL);
}

/* Active view in viewpad has changed */
static void _esdashboard_stage_on_view_activated(EsdashboardStage *self, EsdashboardView *inView, gpointer inUserData)
{
	EsdashboardStagePrivate		*priv;
	EsdashboardViewpad			*viewpad G_GNUC_UNUSED;
	EsdashboardToggleButton		*appsButton;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_VIEWPAD(inUserData));

	priv=self->priv;
	viewpad=ESDASHBOARD_VIEWPAD(inUserData);

	/* If we have remembered a view "before-search" then a search is going on.
	 * If user switches between views while a search is going on remember the
	 * last one activated to restore it when search ends but do not remember
	 * the search view!
	 */
	if(priv->viewBeforeSearch &&
		G_OBJECT_TYPE(inView)!=ESDASHBOARD_TYPE_SEARCH_VIEW)
	{
		/* Release old remembered view */
		g_object_unref(priv->viewBeforeSearch);

		/* Remember new active view */
		priv->viewBeforeSearch=ESDASHBOARD_VIEW(g_object_ref(inView));
	}

	/* Toggle application button in quicklaunch */
	appsButton=esdashboard_quicklaunch_get_apps_button(ESDASHBOARD_QUICKLAUNCH(priv->quicklaunch));
	if(appsButton)
	{
		/* Block our signal handler at stage which is called when apps button's
		 * state changes because it will enforce a specific view depending on its 
		 * state which may not be the view which is going to be activated.
		 */
		g_signal_handlers_block_by_func(appsButton, _esdashboard_stage_on_quicklaunch_apps_button_toggled, self);

		/* Update toggle state of apps button */
		if(G_OBJECT_TYPE(inView)==ESDASHBOARD_TYPE_SEARCH_VIEW ||
			G_OBJECT_TYPE(inView)==ESDASHBOARD_TYPE_APPLICATIONS_VIEW)
		{
			esdashboard_toggle_button_set_toggle_state(appsButton, TRUE);
		}
			else
			{
				esdashboard_toggle_button_set_toggle_state(appsButton, FALSE);
			}

		/* Unblock any handler we blocked before */
		g_signal_handlers_unblock_by_func(appsButton, _esdashboard_stage_on_quicklaunch_apps_button_toggled, self);
	}
}

/* A window was closed
 * Check if stage window was closed then unset up window properties and reinstall
 * signal handler to find new stage window.
 */
static void _esdashboard_stage_on_window_closed(EsdashboardStage *self,
												gpointer inUserData)
{
	EsdashboardStagePrivate				*priv;
	EsdashboardWindowTrackerWindow		*window;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inUserData));

	priv=self->priv;
	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW(inUserData);

	/* Check if window closed is this stage window */
	if(priv->stageWindow!=window) return;

	/* Disconnect this signal handler as this stage window was closed*/
	ESDASHBOARD_DEBUG(self, ACTOR, "Stage window was closed. Removing signal handler");
	g_signal_handlers_disconnect_by_func(priv->stageWindow, G_CALLBACK(_esdashboard_stage_on_window_closed), self);

	/* Forget stage window as it was closed */
	priv->stageWindow=NULL;

	/* Instead reconnect signal handler to find new stage window */
	ESDASHBOARD_DEBUG(self, ACTOR, "Reconnecting signal to find new stage window as this one as closed");
	g_signal_connect_swapped(priv->windowTracker, "window-opened", G_CALLBACK(_esdashboard_stage_on_window_opened), self);

	/* Set focus */
	_esdashboard_stage_set_focus(self);
}


/* A window was created
 * Check for stage window and set up window properties
 */
static void _esdashboard_stage_on_window_opened(EsdashboardStage *self,
													EsdashboardWindowTrackerWindow *inWindow,
													gpointer inUserData)
{
	EsdashboardStagePrivate				*priv;
	EsdashboardWindowTrackerWindow		*stageWindow;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if window opened is this stage window */
	stageWindow=esdashboard_window_tracker_get_stage_window(priv->windowTracker, CLUTTER_STAGE(self));
	if(stageWindow!=inWindow) return;

	/* Set up window for use as stage window */
	priv->stageWindow=inWindow;
	esdashboard_window_tracker_window_show_stage(priv->stageWindow);

	/* Disconnect this signal handler as this is a one-time setup of stage window */
	ESDASHBOARD_DEBUG(self, ACTOR, "Stage window was opened and set up. Removing signal handler");
	g_signal_handlers_disconnect_by_func(priv->windowTracker, G_CALLBACK(_esdashboard_stage_on_window_opened), self);

	/* Instead connect signal handler to get notified when this stage window was
	 * destroyed as we need to forget this window and to reinstall this signal
	 * handler again.
	 */
	ESDASHBOARD_DEBUG(self, ACTOR, "Connecting signal signal handler to get notified about destruction of stage window");
	g_signal_connect_swapped(priv->stageWindow,
								"closed",
								G_CALLBACK(_esdashboard_stage_on_window_closed),
								self);

	/* Set focus */
	_esdashboard_stage_set_focus(self);
}

/* A window was created
 * Check if window opened is desktop background window
 */
static void _esdashboard_stage_on_desktop_window_opened(EsdashboardStage *self,
														EsdashboardWindowTrackerWindow *inWindow,
														gpointer inUserData)
{
	EsdashboardStagePrivate				*priv;
	EsdashboardWindowTrackerWindow		*desktopWindow;
	ClutterContent						*windowContent;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Get desktop background window and check if it is the new window opened */
	desktopWindow=esdashboard_window_tracker_get_root_window(priv->windowTracker);
	if(desktopWindow)
	{
		windowContent=esdashboard_window_tracker_window_get_content(desktopWindow);
		clutter_actor_set_content(priv->backgroundImageLayer, windowContent);
		clutter_actor_show(priv->backgroundImageLayer);
		g_object_unref(windowContent);

		g_signal_handlers_disconnect_by_func(priv->windowTracker, G_CALLBACK(_esdashboard_stage_on_desktop_window_opened), self);
		ESDASHBOARD_DEBUG(self, ACTOR, "Found desktop window with signal 'window-opened', so disconnecting signal handler");
	}
}

/* The application will be suspended */
static void _esdashboard_stage_on_application_suspend(EsdashboardStage *self, gpointer inUserData)
{
	EsdashboardStagePrivate				*priv;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;

	/* Instead of hiding stage actor just hide stage's window. It should be safe
	 * to just hide the window as it should be listed on any task list and is not
	 * selectable by user. The advantage should be that the window is already mapped
	 * and its state is already set up like fullscreen, sticky and so on. This
	 * prevents that window will not be shown in fullscreen again (just maximized
	 * and flickers) if we use clutter_actor_show to show stage actor and its window
	 * again. But we can only do this if the window is known and set up ;)
	 */
	if(priv->stageWindow)
	{
		esdashboard_window_tracker_window_hide_stage(priv->stageWindow);
	}

	/* Hide tooltip */
	if(priv->tooltip) clutter_actor_hide(priv->tooltip);
}

/* The application will be resumed */
static void _esdashboard_stage_on_application_resume(EsdashboardStage *self, gpointer inUserData)
{
	EsdashboardStagePrivate				*priv;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;

	/* If stage window is known just show it again ... */
	if(priv->stageWindow)
	{
		gboolean						doResetSearch;
		EsdashboardView					*searchView;
		EsdashboardView					*resumeView;

		/* Get configured options */
		doResetSearch=esconf_channel_get_bool(esdashboard_application_get_esconf_channel(NULL),
												RESET_SEARCH_ON_RESUME_ESCONF_PROP,
												DEFAULT_RESET_SEARCH_ON_RESUME);

		/* Find search view */
		searchView=esdashboard_viewpad_find_view_by_type(ESDASHBOARD_VIEWPAD(priv->viewpad), ESDASHBOARD_TYPE_SEARCH_VIEW);
		if(!searchView) g_critical("Cannot find search view in viewpad to reset view.");

		/* Find view to switch to if requested */
		resumeView=_esdashboard_stage_get_view_to_switch_to(self);

		/* If view to switch to is the search view behave like we did not find the view
		 * because it does not make sense to switch to a view which might be hidden,
		 * e.g. when resetting search on resume which causes the search view to be hidden
		 * and the previous view to be shown.
		 */
		if(resumeView &&
			searchView &&
			resumeView==searchView)
		{
			resumeView=NULL;
		}

		/* If search is active then end search by clearing search box if requested ... */
		if(priv->searchbox &&
			doResetSearch &&
			!esdashboard_text_box_is_empty(ESDASHBOARD_TEXT_BOX(priv->searchbox)))
		{
			/* If user wants to switch to a specific view set it as "previous" view now.
			 * It will be restored automatically when search box is cleared.
			 */
			if(resumeView)
			{
				/* Release old remembered view */
				if(priv->viewBeforeSearch) g_object_unref(priv->viewBeforeSearch);

				/* Remember new active view */
				priv->viewBeforeSearch=ESDASHBOARD_VIEW(g_object_ref(resumeView));
			}

			/* Reset search in search view */
			if(searchView) esdashboard_search_view_reset_search(ESDASHBOARD_SEARCH_VIEW(searchView));

			/* Reset text in search box */
			esdashboard_text_box_set_text(ESDASHBOARD_TEXT_BOX(priv->searchbox), NULL);
		}
			/* ... otherwise just switch to view if requested */
			else if(resumeView)
			{
				esdashboard_viewpad_set_active_view(ESDASHBOARD_VIEWPAD(priv->viewpad), resumeView);
			}

		/* Now move focus to actor if user requested to refocus preselected actor
		 * as specified by theme.
		 */
		if(priv->focusActorOnShow)
		{
			gboolean				reselectFocusOnResume;

			/* Determine if user (also) requests to reselect focus on resume */
			reselectFocusOnResume=esconf_channel_get_bool(esdashboard_application_get_esconf_channel(NULL),
															RESELECT_THEME_FOCUS_ON_RESUME_ESCONF_PROP,
															DEFAULT_RESELECT_THEME_FOCUS_ON_RESUME);
			if(reselectFocusOnResume)
			{
				/* Move focus to actor */
				esdashboard_focus_manager_set_focus(priv->focusManager, ESDASHBOARD_FOCUSABLE(priv->focusActorOnShow));

				ESDASHBOARD_DEBUG(self, ACTOR,
									"Moved focus to actor %s because it should be reselected on resume",
									G_OBJECT_TYPE_NAME(priv->focusActorOnShow));
			}
				else
				{
					/* Forget actor to focus now the user did not requested to reselect
					 * this focus again and again when stage window is shown ;)
					 */
					g_object_remove_weak_pointer(G_OBJECT(priv->focusActorOnShow), &priv->focusActorOnShow);
					priv->focusActorOnShow=NULL;
				}
		}

		/* Set up stage and show it */
		esdashboard_window_tracker_window_show_stage(priv->stageWindow);
	}
		/* ... otherwise set it up by calling clutter_actor_show() etc. */
		else
		{
			/* Show stage and force window creation. It will also handle
			 * the switch to a specific view.
			 */
			clutter_actor_show(CLUTTER_ACTOR(self));
		}

	/* In any case force a redraw */
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* Theme in application has changed */
static void _esdashboard_stage_theme_interface_data_free(EsdashboardStageThemeInterfaceData *inData)
{
	g_return_if_fail(inData);

	/* Release each data in data structure but do not unref the interface actor
	 * as it might be used at stage. The stage is responsible to destroy the
	 * interface actor in *any* case.
	 */
	if(inData->focusables) g_ptr_array_unref(inData->focusables);
	if(inData->focus) g_object_unref(inData->focus);

	/* Release allocated memory */
	g_free(inData);
}

static EsdashboardStageThemeInterfaceData* _esdashboard_stage_theme_interface_data_new(void)
{
	EsdashboardStageThemeInterfaceData   *data;

	/* Allocate memory for data structure */
	data=g_new0(EsdashboardStageThemeInterfaceData, 1);
	if(!data) return(NULL);

	/* Return newly create and initialized data structure */
	return(data);
}

static void _esdashboard_stage_on_application_theme_changed(EsdashboardStage *self,
															EsdashboardTheme *inTheme,
															gpointer inUserData)
{
	EsdashboardStagePrivate				*priv;
	EsdashboardThemeLayout				*themeLayout;
	GList								*interfaces;
	EsdashboardStageThemeInterfaceData	*interface;
	GList								*iter;
	GList								*monitors;
	EsdashboardWindowTrackerMonitor		*monitor;
	ClutterActorIter					childIter;
	ClutterActor						*child;
	GObject								*focusObject;
	guint								i;
	gboolean							reselectFocusOnResume;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_THEME(inTheme));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;

	/* Get theme layout */
	themeLayout=esdashboard_theme_get_layout(inTheme);

	/* Create interface for each monitor if multiple monitors are supported */
	interfaces=NULL;
	if(esdashboard_window_tracker_supports_multiple_monitors(priv->windowTracker))
	{
		monitors=esdashboard_window_tracker_get_monitors(priv->windowTracker);
		for(iter=monitors; iter; iter=g_list_next(iter))
		{
			/* Get monitor */
			monitor=ESDASHBOARD_WINDOW_TRACKER_MONITOR(iter->data);

			/* Get interface  */
			if(esdashboard_window_tracker_monitor_is_primary(monitor))
			{
				/* Get interface for primary monitor */
				interface=_esdashboard_stage_theme_interface_data_new();
				interface->actor=esdashboard_theme_layout_build_interface(themeLayout,
																			ESDASHBOARD_THEME_LAYOUT_PRIMARY,
																			ESDASHBOARD_THEME_LAYOUT_BUILD_GET_FOCUSABLES, &interface->focusables,
																			ESDASHBOARD_THEME_LAYOUT_BUILD_GET_SELECTED_FOCUS, &interface->focus,
																			-1);
				if(!interface->actor)
				{
					g_critical("Could not build interface '%s' from theme '%s'",
								ESDASHBOARD_THEME_LAYOUT_PRIMARY,
								esdashboard_theme_get_theme_name(inTheme));

					/* Release allocated resources */
					_esdashboard_stage_theme_interface_data_free(interface);
					g_list_free_full(interfaces, (GDestroyNotify)_esdashboard_stage_theme_interface_data_free);

					return;
				}

				if(!ESDASHBOARD_IS_STAGE_INTERFACE(interface->actor))
				{
					g_critical("Interface '%s' from theme '%s' must be an actor of type %s",
								ESDASHBOARD_THEME_LAYOUT_PRIMARY,
								esdashboard_theme_get_theme_name(inTheme),
								g_type_name(ESDASHBOARD_TYPE_STAGE_INTERFACE));

					/* Release allocated resources */
					_esdashboard_stage_theme_interface_data_free(interface);
					g_list_free_full(interfaces, (GDestroyNotify)_esdashboard_stage_theme_interface_data_free);

					return;
				}
			}
				else
				{
					/* Get interface for non-primary monitors. If no interface
					 * is defined in theme then create an empty interface.
					 */
					interface=_esdashboard_stage_theme_interface_data_new();
					interface->actor=esdashboard_theme_layout_build_interface(themeLayout,
																				ESDASHBOARD_THEME_LAYOUT_SECONDARY,
																				ESDASHBOARD_THEME_LAYOUT_BUILD_GET_FOCUSABLES, &interface->focusables,
																				ESDASHBOARD_THEME_LAYOUT_BUILD_GET_SELECTED_FOCUS, &interface->focus,
																				-1);
					if(!interface->actor)
					{
						interface->actor=esdashboard_stage_interface_new();
					}

					if(!ESDASHBOARD_IS_STAGE_INTERFACE(interface->actor))
					{
						g_critical("Interface '%s' from theme '%s' must be an actor of type %s",
									ESDASHBOARD_THEME_LAYOUT_SECONDARY,
									esdashboard_theme_get_theme_name(inTheme),
									g_type_name(ESDASHBOARD_TYPE_STAGE_INTERFACE));

						/* Release allocated resources */
						_esdashboard_stage_theme_interface_data_free(interface);
						g_list_free_full(interfaces, (GDestroyNotify)_esdashboard_stage_theme_interface_data_free);

						return;
					}
				}

			/* Set monitor at interface */
			esdashboard_stage_interface_set_monitor(ESDASHBOARD_STAGE_INTERFACE(interface->actor), monitor);

			/* Add interface to list of interfaces */
			interfaces=g_list_prepend(interfaces, interface);
		}
	}
		/* Otherwise create only a primary stage interface and set no monitor
		 * because no one is available if multiple monitors are not supported.
		 */
		else
		{
			/* Get interface for primary monitor */
			interface=_esdashboard_stage_theme_interface_data_new();
			interface->actor=esdashboard_theme_layout_build_interface(themeLayout,
																		ESDASHBOARD_THEME_LAYOUT_PRIMARY,
																		ESDASHBOARD_THEME_LAYOUT_BUILD_GET_FOCUSABLES, &interface->focusables,
																		ESDASHBOARD_THEME_LAYOUT_BUILD_GET_SELECTED_FOCUS, &interface->focus,
																		-1);
			if(!interface->actor)
			{
				g_critical("Could not build interface '%s' from theme '%s'",
							ESDASHBOARD_THEME_LAYOUT_PRIMARY,
							esdashboard_theme_get_theme_name(inTheme));

				/* Release allocated resources */
				_esdashboard_stage_theme_interface_data_free(interface);
				g_list_free_full(interfaces, (GDestroyNotify)_esdashboard_stage_theme_interface_data_free);

				return;
			}

			if(!ESDASHBOARD_IS_STAGE_INTERFACE(interface->actor))
			{
				g_critical("Interface '%s' from theme '%s' must be an actor of type %s",
							ESDASHBOARD_THEME_LAYOUT_PRIMARY,
							esdashboard_theme_get_theme_name(inTheme),
							g_type_name(ESDASHBOARD_TYPE_STAGE_INTERFACE));

				/* Release allocated resources */
				_esdashboard_stage_theme_interface_data_free(interface);
				g_list_free_full(interfaces, (GDestroyNotify)_esdashboard_stage_theme_interface_data_free);

				return;
			}

			/* Add interface to list of interfaces */
			interfaces=g_list_prepend(interfaces, interface);

			ESDASHBOARD_DEBUG(self, ACTOR, "Creating primary interface only because of no support for multiple monitors");
		}

	/* Destroy all interfaces from stage.
	 * There is no need to reset pointer variables to quicklaunch, searchbox etc.
	 * because they should be set NULL to by calling _esdashboard_stage_reset_reference_on_destroy
	 * when stage actor was destroyed.
	 */
	clutter_actor_iter_init(&childIter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&childIter, &child))
	{
		if(ESDASHBOARD_IS_STAGE_INTERFACE(child))
		{
			clutter_actor_iter_destroy(&childIter);
		}
	}

	/* Add all new interfaces to stage */
	for(iter=interfaces; iter; iter=g_list_next(iter))
	{
		/* Get interface to add to stage */
		interface=(EsdashboardStageThemeInterfaceData*)(iter->data);
		if(!interface) continue;

		/* Check for interface actor to add to stage */
		if(!interface->actor) continue;

		/* Add interface to stage */
		clutter_actor_add_child(CLUTTER_ACTOR(self), interface->actor);

		/* Only check children, set up pointer variables to quicklaunch, searchbox etc.
		 * and connect signals for primary monitor.
		 */
		monitor=esdashboard_stage_interface_get_monitor(ESDASHBOARD_STAGE_INTERFACE(interface->actor));
		if(!monitor || esdashboard_window_tracker_monitor_is_primary(monitor))
		{
			/* Remember primary interface */
			if(!priv->primaryInterface)
			{
				priv->primaryInterface=interface->actor;
				g_object_add_weak_pointer(G_OBJECT(priv->primaryInterface), &priv->primaryInterface);
			}
				else g_critical("Invalid multiple stages for primary monitor");

			/* Get children from built stage and connect signals */
			priv->viewSelector=NULL;
			child=esdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "view-selector");
			if(child && ESDASHBOARD_IS_VIEW_SELECTOR(child))
			{
				priv->viewSelector=child;
				g_object_add_weak_pointer(G_OBJECT(priv->viewSelector), &priv->viewSelector);

				/* Register this focusable actor if it is focusable */
				if(!interface->focusables && ESDASHBOARD_IS_FOCUSABLE(priv->viewSelector))
				{
					esdashboard_focus_manager_register(priv->focusManager,
														ESDASHBOARD_FOCUSABLE(priv->viewSelector));
				}
			}

			priv->searchbox=NULL;
			child=esdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "searchbox");
			if(child && ESDASHBOARD_IS_TEXT_BOX(child))
			{
				priv->searchbox=child;
				g_object_add_weak_pointer(G_OBJECT(priv->searchbox), &priv->searchbox);

				/* If no hint-text was defined, set default one */
				if(!esdashboard_text_box_is_hint_text_set(ESDASHBOARD_TEXT_BOX(priv->searchbox)))
				{
					esdashboard_text_box_set_hint_text(ESDASHBOARD_TEXT_BOX(priv->searchbox),
														_("Just type to search..."));
				}

				/* Connect signals */
				g_signal_connect_swapped(priv->searchbox,
											"text-changed",
											G_CALLBACK(_esdashboard_stage_on_searchbox_text_changed),
											self);
				g_signal_connect_swapped(priv->searchbox,
											"secondary-icon-clicked",
											G_CALLBACK(_esdashboard_stage_on_searchbox_secondary_icon_clicked),
											self);

				/* Register this focusable actor if it is focusable */
				if(!interface->focusables && ESDASHBOARD_IS_FOCUSABLE(priv->searchbox))
				{
					esdashboard_focus_manager_register(priv->focusManager,
														ESDASHBOARD_FOCUSABLE(priv->searchbox));
				}
			}

			priv->viewpad=NULL;
			child=esdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "viewpad");
			if(child && ESDASHBOARD_IS_VIEWPAD(child))
			{
				priv->viewpad=child;
				g_object_add_weak_pointer(G_OBJECT(priv->viewpad), &priv->viewpad);

				/* Connect signals */
				g_signal_connect_swapped(priv->viewpad, "view-activated", G_CALLBACK(_esdashboard_stage_on_view_activated), self);

				/* Register this focusable actor if it is focusable */
				if(!interface->focusables && ESDASHBOARD_IS_FOCUSABLE(priv->viewpad))
				{
					esdashboard_focus_manager_register(priv->focusManager,
														ESDASHBOARD_FOCUSABLE(priv->viewpad));

					/* Check if viewpad can be focused to enforce all focusable views
					 * will be registered too. We need to do it now to get all focusable
					 * views registered before first use of any function of focus manager.
					 */
					esdashboard_focusable_can_focus(ESDASHBOARD_FOCUSABLE(priv->viewpad));
				}
			}

			priv->quicklaunch=NULL;
			child=esdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "quicklaunch");
			if(child && ESDASHBOARD_IS_QUICKLAUNCH(child))
			{
				EsdashboardToggleButton		*appsButton;

				priv->quicklaunch=child;
				g_object_add_weak_pointer(G_OBJECT(priv->quicklaunch), &priv->quicklaunch);

				/* Connect signals */
				appsButton=esdashboard_quicklaunch_get_apps_button(ESDASHBOARD_QUICKLAUNCH(priv->quicklaunch));
				if(appsButton)
				{
					g_signal_connect_swapped(appsButton,
												"toggled",
												G_CALLBACK(_esdashboard_stage_on_quicklaunch_apps_button_toggled),
												self);
				}

				/* Register this focusable actor if it is focusable */
				if(!interface->focusables && ESDASHBOARD_IS_FOCUSABLE(priv->quicklaunch))
				{
					esdashboard_focus_manager_register(priv->focusManager,
														ESDASHBOARD_FOCUSABLE(priv->quicklaunch));
				}
			}

			priv->workspaces=NULL;
			child=esdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "workspace-selector");
			if(child && ESDASHBOARD_IS_WORKSPACE_SELECTOR(child))
			{
				priv->workspaces=child;
				g_object_add_weak_pointer(G_OBJECT(priv->workspaces), &priv->workspaces);

				/* Register this focusable actor if it is focusable */
				if(!interface->focusables && ESDASHBOARD_IS_FOCUSABLE(priv->workspaces))
				{
					esdashboard_focus_manager_register(priv->focusManager,
														ESDASHBOARD_FOCUSABLE(priv->workspaces));
				}
			}

			priv->notification=NULL;
			child=esdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "notification");
			if(child && ESDASHBOARD_IS_TEXT_BOX(child))
			{
				priv->notification=child;
				g_object_add_weak_pointer(G_OBJECT(priv->notification), &priv->notification);

				/* Register this focusable actor if it is focusable */
				if(!interface->focusables && ESDASHBOARD_IS_FOCUSABLE(priv->notification))
				{
					esdashboard_focus_manager_register(priv->focusManager,
														ESDASHBOARD_FOCUSABLE(priv->notification));
				}

				/* Hide notification by default */
				clutter_actor_hide(priv->notification);
				clutter_actor_set_reactive(priv->notification, FALSE);
			}

			priv->tooltip=NULL;
			child=esdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "tooltip");
			if(child && ESDASHBOARD_IS_TEXT_BOX(child))
			{
				priv->tooltip=child;
				g_object_add_weak_pointer(G_OBJECT(priv->tooltip), &priv->tooltip);

				/* Register this focusable actor if it is focusable */
				if(!interface->focusables && ESDASHBOARD_IS_FOCUSABLE(priv->tooltip))
				{
					esdashboard_focus_manager_register(priv->focusManager,
														ESDASHBOARD_FOCUSABLE(priv->tooltip));
				}

				/* Hide tooltip by default */
				clutter_actor_hide(priv->tooltip);
				clutter_actor_set_reactive(priv->tooltip, FALSE);
			}

			/* Register focusable actors at focus manager */
			if(interface->focusables)
			{
				for(i=0; i<interface->focusables->len; i++)
				{
					/* Get actor to register at focus manager */
					focusObject=G_OBJECT(g_ptr_array_index(interface->focusables, i));
					if(!focusObject) continue;

					/* Check that actor is focusable */
					if(!ESDASHBOARD_IS_FOCUSABLE(focusObject))
					{
						g_warning("Object %s is not focusable and cannot be registered.",
									G_OBJECT_TYPE_NAME(focusObject));
						continue;
					}

					/* Register actor at focus manager */
					esdashboard_focus_manager_register(priv->focusManager,
														ESDASHBOARD_FOCUSABLE(focusObject));
					ESDASHBOARD_DEBUG(self, ACTOR,
										"Registering actor %s of interface with ID '%s' at focus manager",
										G_OBJECT_TYPE_NAME(focusObject),
										clutter_actor_get_name(interface->actor));
				}
			}

			/* Move focus to selected actor or remember actor focus to set it later
			 * but only if selected actor is a focusable actor and is registered
			 * to focus manager.
			 */
			if(interface->focus &&
				ESDASHBOARD_IS_FOCUSABLE(interface->focus) &&
				esdashboard_focus_manager_is_registered(priv->focusManager, ESDASHBOARD_FOCUSABLE(interface->focus)))
			{
				/* If actor can be focused then move focus to actor ... */
				if(esdashboard_focusable_can_focus(ESDASHBOARD_FOCUSABLE(interface->focus)))
				{
					esdashboard_focus_manager_set_focus(priv->focusManager, ESDASHBOARD_FOCUSABLE(interface->focus));
					ESDASHBOARD_DEBUG(self, ACTOR,
										"Moved focus to actor %s of interface with ID '%s'",
										G_OBJECT_TYPE_NAME(interface->focus),
										clutter_actor_get_name(interface->actor));

					/* Determine if user (also) requests to reselect focus on resume
					 * because then remember the actor to focus to move the focus
					 * each time the stage window gets shown after it was hidden.
					 */
					reselectFocusOnResume=esconf_channel_get_bool(esdashboard_application_get_esconf_channel(NULL),
																	RESELECT_THEME_FOCUS_ON_RESUME_ESCONF_PROP,
																	DEFAULT_RESELECT_THEME_FOCUS_ON_RESUME);
					if(reselectFocusOnResume)
					{
						priv->focusActorOnShow=ESDASHBOARD_FOCUSABLE(interface->focus);
						g_object_add_weak_pointer(G_OBJECT(priv->focusActorOnShow), &priv->focusActorOnShow);

						ESDASHBOARD_DEBUG(self, ACTOR,
											"Will move focus to actor %s of interface with ID '%s' any time the stage gets visible",
											G_OBJECT_TYPE_NAME(interface->focus),
											clutter_actor_get_name(interface->actor));
					}
				}
					/* ... otherwise if stage is not visible, remember the actor
					 * to focus to move the focus to it as soon as stage is
					 * visible ...
					 */
					else if(!clutter_actor_is_visible(CLUTTER_ACTOR(self)))
					{
						priv->focusActorOnShow=ESDASHBOARD_FOCUSABLE(interface->focus);
						g_object_add_weak_pointer(G_OBJECT(priv->focusActorOnShow), &priv->focusActorOnShow);

						ESDASHBOARD_DEBUG(self, ACTOR,
											"Cannot move focus to actor %s of interface with ID '%s' but will try again when stage is visible",
											G_OBJECT_TYPE_NAME(interface->focus),
											clutter_actor_get_name(interface->actor));
					}
					/* ... otherwise just show a debug message */
					else
					{
						ESDASHBOARD_DEBUG(self, ACTOR,
											"Cannot move focus to actor %s of interface with ID '%s' because actor cannot be focused",
											G_OBJECT_TYPE_NAME(interface->focus),
											clutter_actor_get_name(interface->actor));
					}
			}
				else
				{
					ESDASHBOARD_DEBUG(self, ACTOR, "Cannot move focus to any actor because no one was selected in theme");
				}
		}
	}

	/* Release allocated resources */
	g_list_free_full(interfaces, (GDestroyNotify)_esdashboard_stage_theme_interface_data_free);

	/* Set focus */
	_esdashboard_stage_set_focus(self);
}

/* Primary monitor changed */
static void _esdashboard_stage_on_primary_monitor_changed(EsdashboardStage *self,
															EsdashboardWindowTrackerMonitor *inOldMonitor,
															EsdashboardWindowTrackerMonitor *inNewMonitor,
															gpointer inUserData)
{
	EsdashboardStagePrivate					*priv;
	EsdashboardStageInterface				*oldStageInterface;
	EsdashboardWindowTrackerMonitor			*oldPrimaryStageInterfaceMonitor;
	ClutterActorIter						childIter;
	ClutterActor							*child;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(!inOldMonitor || ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inOldMonitor));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inNewMonitor));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(inUserData));

	priv=self->priv;

	/* If we do not have a primary stage interface yet do nothing */
	if(!priv->primaryInterface) return;

	/* If primary stage interface has already new monitor set do nothing */
	oldPrimaryStageInterfaceMonitor=esdashboard_stage_interface_get_monitor(ESDASHBOARD_STAGE_INTERFACE(priv->primaryInterface));
	if(oldPrimaryStageInterfaceMonitor==inNewMonitor) return;

	/* Find stage interface currently using the new primary monitor */
	oldStageInterface=NULL;

	clutter_actor_iter_init(&childIter, CLUTTER_ACTOR(self));
	while(!oldStageInterface && clutter_actor_iter_next(&childIter, &child))
	{
		EsdashboardStageInterface			*interface;

		/* Check for stage interface */
		if(!ESDASHBOARD_IS_STAGE_INTERFACE(child)) continue;

		/* Get stage interface */
		interface=ESDASHBOARD_STAGE_INTERFACE(child);

		/* Check if stage interface is using new primary monitor then remember it */
		if(esdashboard_stage_interface_get_monitor(interface)==inNewMonitor)
		{
			oldStageInterface=interface;
		}
	}

	/* Set old primary monitor at stage interface which is using new primary monitor */
	if(oldStageInterface)
	{
		/* Set old monitor at found stage interface */
		esdashboard_stage_interface_set_monitor(oldStageInterface, oldPrimaryStageInterfaceMonitor);
	}

	/* Set new primary monitor at primary stage interface */
	esdashboard_stage_interface_set_monitor(ESDASHBOARD_STAGE_INTERFACE(priv->primaryInterface), inNewMonitor);
	ESDASHBOARD_DEBUG(self, ACTOR,
						"Primary monitor changed from %d to %d",
						esdashboard_window_tracker_monitor_get_number(oldPrimaryStageInterfaceMonitor),
						esdashboard_window_tracker_monitor_get_number(inNewMonitor));
}

/* A monitor was added */
static void _esdashboard_stage_on_monitor_added(EsdashboardStage *self,
												EsdashboardWindowTrackerMonitor *inMonitor,
												gpointer inUserData)
{
	ClutterActor							*interface;
	EsdashboardTheme						*theme;
	EsdashboardThemeLayout					*themeLayout;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(inUserData));

	/* Get theme and theme layout */
	theme=esdashboard_application_get_theme(NULL);
	themeLayout=esdashboard_theme_get_layout(theme);

	/* Create interface for non-primary monitors. If no interface is defined in theme
	 * then create an empty interface.
	 */
	interface=esdashboard_theme_layout_build_interface(themeLayout, ESDASHBOARD_THEME_LAYOUT_SECONDARY);
	if(!interface)
	{
		interface=esdashboard_stage_interface_new();
	}

	if(!ESDASHBOARD_IS_STAGE_INTERFACE(interface))
	{
		g_critical("Interface '%s' from theme '%s' must be an actor of type %s",
					ESDASHBOARD_THEME_LAYOUT_SECONDARY,
					esdashboard_theme_get_theme_name(theme),
					g_type_name(ESDASHBOARD_TYPE_STAGE_INTERFACE));
		return;
	}

	/* Set monitor at interface */
	esdashboard_stage_interface_set_monitor(ESDASHBOARD_STAGE_INTERFACE(interface), inMonitor);

	/* Add interface to stage */
	clutter_actor_add_child(CLUTTER_ACTOR(self), interface);
	ESDASHBOARD_DEBUG(self, ACTOR,
						"Added stage interface for new monitor %d",
						esdashboard_window_tracker_monitor_get_number(inMonitor));

	/* If monitor added is the primary monitor then swap now the stage interfaces */
	if(esdashboard_window_tracker_monitor_is_primary(inMonitor))
	{
		_esdashboard_stage_on_primary_monitor_changed(self, NULL, inMonitor, inUserData);
	}
}

/* A monitor was removed */
static void _esdashboard_stage_on_monitor_removed(EsdashboardStage *self,
													EsdashboardWindowTrackerMonitor *inMonitor,
													gpointer inUserData)
{
	EsdashboardStagePrivate					*priv;
	ClutterActorIter						childIter;
	ClutterActor							*child;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(inUserData));

	priv=self->priv;

	/* If monitor removed is the primary monitor swap primary interface with first
	 * stage interface to keep it alive. We should afterward receive a signal that
	 * primary monitor has changed, then the primary interface will be set to its
	 * right place.
	 */
	if(esdashboard_window_tracker_monitor_is_primary(inMonitor))
	{
		EsdashboardWindowTrackerMonitor*	firstMonitor;

		/* Get first monitor */
		firstMonitor=esdashboard_window_tracker_get_monitor_by_number(priv->windowTracker, 0);

		/* Swp stage interfaces */
		_esdashboard_stage_on_primary_monitor_changed(self, inMonitor, firstMonitor, inUserData);
	}

	/* Look up stage interface for removed monitor and destroy it */
	clutter_actor_iter_init(&childIter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&childIter, &child))
	{
		EsdashboardStageInterface			*interface;

		/* Only check stage interfaces */
		if(!ESDASHBOARD_IS_STAGE_INTERFACE(child)) continue;

		/* If stage interface is the one for this monitor then destroy it */
		interface=ESDASHBOARD_STAGE_INTERFACE(child);
		if(esdashboard_stage_interface_get_monitor(interface)==inMonitor)
		{
			clutter_actor_iter_destroy(&childIter);
			ESDASHBOARD_DEBUG(self, ACTOR,
								"Removed stage interface for removed monitor %d",
								esdashboard_window_tracker_monitor_get_number(inMonitor));
		}
	}
}

/* Screen size has changed */
static void _esdashboard_stage_on_screen_size_changed(EsdashboardStage *self,
														gpointer inUserData)
{
	EsdashboardWindowTracker	*windowTracker;
	gint						screenWidth, screenHeight;
	gfloat						stageWidth, stageHeight;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER(inUserData));

	windowTracker=ESDASHBOARD_WINDOW_TRACKER(inUserData);

	/* Get screen size */
	esdashboard_window_tracker_get_screen_size(windowTracker, &screenWidth, &screenHeight);

	/* Get current size of stage */
	clutter_actor_get_size(CLUTTER_ACTOR(self), &stageWidth, &stageHeight);

	/* If either stage's width or height does not match screen's width or height
	 * resize the stage.
	 */
	if((gint)stageWidth!=screenWidth ||
		(gint)stageHeight!=screenHeight)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Screen resized to %dx%d but stage has size of %dx%d - resizing stage",
							screenWidth, screenHeight,
							(gint)stageWidth, (gint)stageHeight);

		clutter_actor_set_size(CLUTTER_ACTOR(self), screenWidth, screenHeight);
	}
}

/* IMPLEMENTATION: ClutterActor */

/* The stage actor should be shown */
static void _esdashboard_stage_show(ClutterActor *inActor)
{
	EsdashboardStage			*self;
	EsdashboardStagePrivate		*priv;
	EsdashboardView				*switchView;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(inActor));

	self=ESDASHBOARD_STAGE(inActor);
	priv=self->priv;

	/* Find view to switch to if requested and switch to this view */
	switchView=_esdashboard_stage_get_view_to_switch_to(self);
	if(switchView)
	{
		esdashboard_viewpad_set_active_view(ESDASHBOARD_VIEWPAD(priv->viewpad), switchView);
	}

	/* Set stage to fullscreen as it may will be a newly created window */
	clutter_stage_set_fullscreen(CLUTTER_STAGE(self), TRUE);

	/* If we do not know the stage window connect signal to find it */
	if(!priv->stageWindow)
	{
		/* Connect signals */
		ESDASHBOARD_DEBUG(self, ACTOR, "Connecting signal to find stage window");
		g_signal_connect_swapped(priv->windowTracker, "window-opened", G_CALLBACK(_esdashboard_stage_on_window_opened), self);
	}

	/* Call parent's show method */
	if(CLUTTER_ACTOR_CLASS(esdashboard_stage_parent_class)->show)
	{
		CLUTTER_ACTOR_CLASS(esdashboard_stage_parent_class)->show(inActor);
	}

	/* Now move focus to actor is one was remembered when theme was loaded */
	if(priv->focusActorOnShow)
	{
		gboolean				reselectFocusOnResume;

		/* Determine if user (also) requests to reselect focus on resume */
		reselectFocusOnResume=esconf_channel_get_bool(esdashboard_application_get_esconf_channel(NULL),
														RESELECT_THEME_FOCUS_ON_RESUME_ESCONF_PROP,
														DEFAULT_RESELECT_THEME_FOCUS_ON_RESUME);

		/* Move focus to actor */
		esdashboard_focus_manager_set_focus(priv->focusManager, ESDASHBOARD_FOCUSABLE(priv->focusActorOnShow));

		ESDASHBOARD_DEBUG(self, ACTOR,
							"Moved focus to actor %s %s",
							G_OBJECT_TYPE_NAME(priv->focusActorOnShow),
							!reselectFocusOnResume ? "now as it was delayed to when stage is visible" : "because it should be reselected on resume");

		/* Forget actor to focus now if user did not requested to reselect
		 * this focus again and again when stage window is shown ;)
		 */
		if(!reselectFocusOnResume)
		{
			g_object_remove_weak_pointer(G_OBJECT(priv->focusActorOnShow), &priv->focusActorOnShow);
			priv->focusActorOnShow=NULL;
		}
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_stage_dispose(GObject *inObject)
{
	EsdashboardStage			*self=ESDASHBOARD_STAGE(inObject);
	EsdashboardStagePrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->stageWindow)
	{
		g_signal_handlers_disconnect_by_func(priv->stageWindow, G_CALLBACK(_esdashboard_stage_on_window_closed), self);
		esdashboard_window_tracker_window_hide_stage(priv->stageWindow);
		priv->stageWindow=NULL;
	}

	if(priv->focusManager)
	{
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	if(priv->notificationTimeoutID)
	{
		g_source_remove(priv->notificationTimeoutID);
		priv->notificationTimeoutID=0;
	}

	if(priv->windowTracker)
	{
		g_signal_handlers_disconnect_by_data(priv->windowTracker, self);
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->backgroundColor)
	{
		clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=NULL;
	}

	if(priv->notification)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->notification));
		priv->notification=NULL;
	}

	if(priv->tooltip)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->tooltip));
		priv->tooltip=NULL;
	}

	if(priv->quicklaunch)
	{
		clutter_actor_destroy(priv->quicklaunch);
		priv->quicklaunch=NULL;
	}

	if(priv->searchbox)
	{
		clutter_actor_destroy(priv->searchbox);
		priv->searchbox=NULL;
	}

	if(priv->workspaces)
	{
		clutter_actor_destroy(priv->workspaces);
		priv->workspaces=NULL;
	}

	if(priv->viewSelector)
	{
		clutter_actor_destroy(priv->viewSelector);
		priv->viewSelector=NULL;
	}

	if(priv->viewpad)
	{
		clutter_actor_destroy(priv->viewpad);
		priv->viewpad=NULL;
	}

	if(priv->primaryInterface)
	{
		clutter_actor_destroy(priv->primaryInterface);
		priv->primaryInterface=NULL;
	}

	if(priv->viewBeforeSearch)
	{
		g_object_unref(priv->viewBeforeSearch);
		priv->viewBeforeSearch=NULL;
	}

	if(priv->backgroundImageLayer)
	{
		clutter_actor_destroy(priv->backgroundImageLayer);
		priv->backgroundImageLayer=NULL;
	}

	if(priv->backgroundColorLayer)
	{
		clutter_actor_destroy(priv->backgroundColorLayer);
		priv->backgroundColorLayer=NULL;
	}

	if(priv->switchToView)
	{
		g_free(priv->switchToView);
		priv->switchToView=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_stage_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_stage_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	EsdashboardStage			*self=ESDASHBOARD_STAGE(inObject);

	switch(inPropID)
	{
		case PROP_BACKGROUND_IMAGE_TYPE:
			esdashboard_stage_set_background_image_type(self, g_value_get_enum(inValue));
			break;

		case PROP_BACKGROUND_COLOR:
			esdashboard_stage_set_background_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_SWITCH_TO_VIEW:
			esdashboard_stage_set_switch_to_view(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_stage_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	EsdashboardStage			*self=ESDASHBOARD_STAGE(inObject);
	EsdashboardStagePrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_BACKGROUND_IMAGE_TYPE:
			g_value_set_enum(outValue, priv->backgroundType);
			break;

		case PROP_BACKGROUND_COLOR:
			clutter_value_set_color(outValue, priv->backgroundColor);
			break;

		case PROP_SWITCH_TO_VIEW:
			g_value_set_string(outValue, priv->switchToView);
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
static void esdashboard_stage_class_init(EsdashboardStageClass *klass)
{
	ClutterActorClass				*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	klass->show_tooltip=_esdashboard_stage_show_tooltip;
	klass->hide_tooltip=_esdashboard_stage_hide_tooltip;

	actorClass->show=_esdashboard_stage_show;
	actorClass->event=_esdashboard_stage_event;

	gobjectClass->dispose=_esdashboard_stage_dispose;
	gobjectClass->set_property=_esdashboard_stage_set_property;
	gobjectClass->get_property=_esdashboard_stage_get_property;

	/* Define properties */
	EsdashboardStageProperties[PROP_BACKGROUND_IMAGE_TYPE]=
		g_param_spec_enum("background-image-type",
							"Background image type",
							"Background image type",
							ESDASHBOARD_TYPE_STAGE_BACKGROUND_IMAGE_TYPE,
							ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardStageProperties[PROP_BACKGROUND_COLOR]=
		clutter_param_spec_color("background-color",
									"Background color",
									"Color of stage's background",
									NULL,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardStageProperties[PROP_SWITCH_TO_VIEW]=
		g_param_spec_string("switch-to-view",
							"Switch to view",
							"Switch to this named view as soon as stage gets visible",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardStageProperties);

	/* Define signals */
	EsdashboardStageSignals[SIGNAL_ACTOR_CREATED]=
		g_signal_new("actor-created",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardStageClass, actor_created),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						CLUTTER_TYPE_ACTOR);

	EsdashboardStageSignals[SIGNAL_SEARCH_STARTED]=
		g_signal_new("search-started",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardStageClass, search_started),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	EsdashboardStageSignals[SIGNAL_SEARCH_CHANGED]=
		g_signal_new("search-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardStageClass, search_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__STRING,
						G_TYPE_NONE,
						1,
						G_TYPE_STRING);

	EsdashboardStageSignals[SIGNAL_SEARCH_ENDED]=
		g_signal_new("search-ended",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardStageClass, search_ended),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	EsdashboardStageSignals[SIGNAL_SHOW_TOOLTIP]=
		g_signal_new("show-tooltip",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardStageClass, show_tooltip),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						CLUTTER_TYPE_ACTION);

	EsdashboardStageSignals[SIGNAL_HIDE_TOOLTIP]=
		g_signal_new("hide-tooltip",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardStageClass, hide_tooltip),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						CLUTTER_TYPE_ACTION);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_stage_init(EsdashboardStage *self)
{
	EsdashboardStagePrivate		*priv;
	EsdashboardApplication		*application;
	ClutterConstraint			*widthConstraint;
	ClutterConstraint			*heightConstraint;
	ClutterColor				transparent;

	priv=self->priv=esdashboard_stage_get_instance_private(self);

	/* Set default values */
	priv->focusManager=esdashboard_focus_manager_get_default();
	priv->windowTracker=esdashboard_window_tracker_get_default();
	priv->stageWindow=NULL;
	priv->primaryInterface=NULL;
	priv->quicklaunch=NULL;
	priv->searchbox=NULL;
	priv->workspaces=NULL;
	priv->viewpad=NULL;
	priv->viewSelector=NULL;
	priv->notification=NULL;
	priv->tooltip=NULL;
	priv->lastSearchTextLength=0;
	priv->viewBeforeSearch=NULL;
	priv->searchActive=FALSE;
	priv->notificationTimeoutID=0;
	priv->backgroundType=ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE;
	priv->backgroundColor=NULL;
	priv->backgroundColorLayer=NULL;
	priv->backgroundImageLayer=NULL;
	priv->switchToView=NULL;
	priv->focusActorOnShow=NULL;

	/* Create background actors but order of adding background children is important */
	widthConstraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_WIDTH, 0.0f);
	heightConstraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_HEIGHT, 0.0f);
	priv->backgroundImageLayer=clutter_actor_new();
	clutter_actor_hide(priv->backgroundImageLayer);
	clutter_actor_add_constraint(priv->backgroundImageLayer, widthConstraint);
	clutter_actor_add_constraint(priv->backgroundImageLayer, heightConstraint);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->backgroundImageLayer);

	widthConstraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_WIDTH, 0.0f);
	heightConstraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_HEIGHT, 0.0f);
	priv->backgroundColorLayer=clutter_actor_new();
	clutter_actor_hide(priv->backgroundColorLayer);
	clutter_actor_add_constraint(priv->backgroundColorLayer, widthConstraint);
	clutter_actor_add_constraint(priv->backgroundColorLayer, heightConstraint);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->backgroundColorLayer);

	/* Set up stage and style it */
	clutter_color_init(&transparent, 0, 0, 0, 0);
	clutter_actor_set_background_color(CLUTTER_ACTOR(self), &transparent);

	clutter_stage_set_use_alpha(CLUTTER_STAGE(self), TRUE);
	clutter_stage_set_user_resizable(CLUTTER_STAGE(self), FALSE);
	clutter_stage_set_fullscreen(CLUTTER_STAGE(self), TRUE);

	/* Connect signals to window tracker */
	g_signal_connect_swapped(priv->windowTracker,
								"monitor-added",
								G_CALLBACK(_esdashboard_stage_on_monitor_added),
								self);
	g_signal_connect_swapped(priv->windowTracker,
								"monitor-removed",
								G_CALLBACK(_esdashboard_stage_on_monitor_removed),
								self);
	g_signal_connect_swapped(priv->windowTracker,
								"primary-monitor-changed",
								G_CALLBACK(_esdashboard_stage_on_primary_monitor_changed),
								self);

	/* Connect signal to application */
	application=esdashboard_application_get_default();
	g_signal_connect_swapped(application,
								"suspend",
								G_CALLBACK(_esdashboard_stage_on_application_suspend),
								self);

	g_signal_connect_swapped(application,
								"resume",
								G_CALLBACK(_esdashboard_stage_on_application_resume),
								self);

	g_signal_connect_swapped(application,
								"theme-changed",
								G_CALLBACK(_esdashboard_stage_on_application_theme_changed),
								self);

	/* Resize stage to match screen size and listen for futher screen size changes
	 * to resize stage again.
	 * This should only be needed when compiled against Clutter prior to 0.17.2
	 * because this version or newer ones seem to handle window resizes correctly.
	 */
	if(clutter_major_version<1 ||
		(clutter_major_version==1 && clutter_minor_version<17) ||
		(clutter_major_version==1 && clutter_minor_version==17 && clutter_micro_version<2))
	{
		_esdashboard_stage_on_screen_size_changed(self, priv->windowTracker);

		g_signal_connect_swapped(priv->windowTracker,
									"screen-size-changed",
									G_CALLBACK(_esdashboard_stage_on_screen_size_changed),
									self);

		ESDASHBOARD_DEBUG(self, ACTOR, "Tracking screen resizes to resize stage");
	}
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* esdashboard_stage_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(ESDASHBOARD_TYPE_STAGE, NULL)));
}

/* Get/set background type */
EsdashboardStageBackgroundImageType esdashboard_stage_get_background_image_type(EsdashboardStage *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_STAGE(self), ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE);

	return(self->priv->backgroundType);
}

void esdashboard_stage_set_background_image_type(EsdashboardStage *self, EsdashboardStageBackgroundImageType inType)
{
	EsdashboardStagePrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));
	g_return_if_fail(inType<=ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_DESKTOP);

	priv=self->priv;


	/* Set value if changed */
	if(priv->backgroundType!=inType)
	{
		/* Set value */
		priv->backgroundType=inType;

		/* Set up background actor depending on type */
		if(priv->backgroundImageLayer)
		{
			switch(priv->backgroundType)
			{
				case ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_DESKTOP:
					{
						EsdashboardWindowTrackerWindow	*backgroundWindow;

						backgroundWindow=esdashboard_window_tracker_get_root_window(priv->windowTracker);
						if(backgroundWindow)
						{
							ClutterContent				*backgroundContent;

							backgroundContent=esdashboard_window_tracker_window_get_content(backgroundWindow);
							clutter_actor_show(priv->backgroundImageLayer);
							clutter_actor_set_content(priv->backgroundImageLayer, backgroundContent);
							g_object_unref(backgroundContent);

							ESDASHBOARD_DEBUG(self, ACTOR, "Desktop window was found and set up as background image for stage");
						}
							else
							{
								g_signal_connect_swapped(priv->windowTracker,
															"window-opened",
															G_CALLBACK(_esdashboard_stage_on_desktop_window_opened),
															self);
								ESDASHBOARD_DEBUG(self, ACTOR, "Desktop window was not found. Setting up signal to get notified when desktop window might be opened.");
							}
					}
					break;

				default:
					clutter_actor_hide(priv->backgroundImageLayer);
					clutter_actor_set_content(priv->backgroundImageLayer, NULL);
					break;
			}
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardStageProperties[PROP_BACKGROUND_IMAGE_TYPE]);
	}
}

/* Get/set background color */
ClutterColor* esdashboard_stage_get_background_color(EsdashboardStage *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_STAGE(self), NULL);

	return(self->priv->backgroundColor);
}

void esdashboard_stage_set_background_color(EsdashboardStage *self, const ClutterColor *inColor)
{
	EsdashboardStagePrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));

	priv=self->priv;

	/* Set value if changed */
	if((priv->backgroundColor && !inColor) ||
		(!priv->backgroundColor && inColor) ||
		(inColor && clutter_color_equal(inColor, priv->backgroundColor)==FALSE))
	{
		/* Set value */
		if(priv->backgroundColor)
		{
			clutter_color_free(priv->backgroundColor);
			priv->backgroundColor=NULL;
		}

		if(inColor) priv->backgroundColor=clutter_color_copy(inColor);

		/* If a color is provided set background color and show background actor
		 * otherwise hide background actor
		 */
		if(priv->backgroundColorLayer)
		{
			if(priv->backgroundColor)
			{
				clutter_actor_set_background_color(priv->backgroundColorLayer,
													priv->backgroundColor);
				clutter_actor_show(priv->backgroundColorLayer);
			}
				else clutter_actor_hide(priv->backgroundColorLayer);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardStageProperties[PROP_BACKGROUND_COLOR]);
	}
}

/* Set name of view to switch to at next resume */
const gchar* esdashboard_stage_get_switch_to_view(EsdashboardStage *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_STAGE(self), NULL);

	return(self->priv->switchToView);
}

void esdashboard_stage_set_switch_to_view(EsdashboardStage *self, const gchar *inViewInternalName)
{
	EsdashboardStagePrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->switchToView, inViewInternalName)!=0)
	{
		if(priv->switchToView)
		{
			g_free(priv->switchToView);
			priv->switchToView=NULL;
		}

		if(inViewInternalName) priv->switchToView=g_strdup(inViewInternalName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardStageProperties[PROP_SWITCH_TO_VIEW]);
	}
}

/* Show a notification on stage */
void esdashboard_stage_show_notification(EsdashboardStage *self, const gchar *inIconName, const gchar *inText)
{
	EsdashboardStagePrivate		*priv;
	gint						interval;

	g_return_if_fail(ESDASHBOARD_IS_STAGE(self));

	priv=self->priv;

	/* Stop current running timeout source because it would hide this
	 * new notification to soon.
	 */
	if(priv->notificationTimeoutID)
	{
		g_source_remove(priv->notificationTimeoutID);
		priv->notificationTimeoutID=0;
	}

	/* Only show notification if a notification box is known where the notification
	 * could be shown at.
	 */
	if(!priv->notification)
	{
		ESDASHBOARD_DEBUG(self, ACTOR, "Cannot show notification because no notification box is available");
		return;
	}

	/* Show notification on stage */
	esdashboard_text_box_set_text(ESDASHBOARD_TEXT_BOX(priv->notification), inText);
	esdashboard_text_box_set_primary_icon(ESDASHBOARD_TEXT_BOX(priv->notification), inIconName);
	clutter_actor_show(CLUTTER_ACTOR(priv->notification));

	/* Set up timeout source. The timeout interval differs and depends on the length
	 * of the notification text to show but never drops below the minimum timeout configured.
	 * The interval is calculated by one second for 30 characters.
	 */
	interval=esconf_channel_get_uint(esdashboard_application_get_esconf_channel(NULL),
										NOTIFICATION_TIMEOUT_ESCONF_PROP,
										DEFAULT_NOTIFICATION_TIMEOUT);
	interval=MAX((gint)((strlen(inText)/30.0f)*1000.0f), interval);

	priv->notificationTimeoutID=clutter_threads_add_timeout_full(G_PRIORITY_DEFAULT,
																	interval,
																	_esdashboard_stage_on_notification_timeout,
																	self,
																	_esdashboard_stage_on_notification_timeout_destroyed);
}
