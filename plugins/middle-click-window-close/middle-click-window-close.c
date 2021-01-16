/*
 * middle-click-window-close: Closes windows in window by middle-click
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

#include "middle-click-window-close.h"

#include <libesdashboard/libesdashboard.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>


/* Define this class in GObject system */
struct _EsdashboardMiddleClickWindowClosePrivate
{
	/* Instance related */
	EsdashboardStage						*stage;
	guint									stageActorCreatedSignalID;
	guint									stageDestroySignalID;

	EsdashboardCssSelector					*liveWindowSelector;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(EsdashboardMiddleClickWindowClose,
								esdashboard_middle_click_window_close,
								G_TYPE_OBJECT,
								0,
								G_ADD_PRIVATE_DYNAMIC(EsdashboardMiddleClickWindowClose))

/* Define this class in this plugin */
ESDASHBOARD_DEFINE_PLUGIN_TYPE(esdashboard_middle_click_window_close);

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_WINDOW_CLOSE_BUTTON							ESDASHBOARD_CLICK_ACTION_MIDDLE_BUTTON
#define ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_ACTION_NAME	"middle-click-window-close-action"

/* A configured live window actor was clicked */
static void _esdashboard_middle_click_window_close_on_clicked(EsdashboardMiddleClickWindowClose *self,
																ClutterActor *inActor,
																gpointer inUserData)
{
	EsdashboardLiveWindowSimple						*liveWindow;
	EsdashboardClickAction							*action;
	guint											button;
	EsdashboardWindowTrackerWindow					*window;

	g_return_if_fail(ESDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(self));
	g_return_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(inActor));
	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(inUserData));

	liveWindow=ESDASHBOARD_LIVE_WINDOW_SIMPLE(inActor);
	action=ESDASHBOARD_CLICK_ACTION(inUserData);

	/* Get button used for click action */
	button=esdashboard_click_action_get_button(action);
	if(button==DEFAULT_WINDOW_CLOSE_BUTTON)
	{
		window=esdashboard_live_window_simple_get_window(liveWindow);
		esdashboard_window_tracker_window_close(window);
	}
}

/* An actor was created so check if we are interested at this one */
static void _esdashboard_middle_click_window_close_on_actor_created(EsdashboardMiddleClickWindowClose *self,
																	ClutterActor *inActor,
																	gpointer inUserData)
{
	EsdashboardMiddleClickWindowClosePrivate		*priv;
	gint											score;
	ClutterAction									*action;

	g_return_if_fail(ESDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));

	priv=self->priv;

	/* Check if we are interested in this newly created actor and set it up */
	if(ESDASHBOARD_IS_STYLABLE(inActor))
	{
		score=esdashboard_css_selector_score(priv->liveWindowSelector, ESDASHBOARD_STYLABLE(inActor));
		if(score>0)
		{
			action=esdashboard_click_action_new();
			clutter_actor_add_action_with_name(inActor, ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_ACTION_NAME, action);
			g_signal_connect_swapped(action, "clicked", G_CALLBACK(_esdashboard_middle_click_window_close_on_clicked), self);
		}
	}
}

/* Callback for traversal to setup live window for use with this plugin */
static gboolean _esdashboard_middle_click_window_close_traverse_acquire(ClutterActor *inActor,
																		gpointer inUserData)
{
	EsdashboardMiddleClickWindowClose				*self;
	ClutterAction									*action;

	g_return_val_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(inActor), ESDASHBOARD_TRAVERSAL_CONTINUE);
	g_return_val_if_fail(ESDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(inUserData), ESDASHBOARD_TRAVERSAL_CONTINUE);

	self=ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE(inUserData);

	/* Set up live window */
	action=esdashboard_click_action_new();
	clutter_actor_add_action_with_name(inActor, ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_ACTION_NAME, action);
	g_signal_connect_swapped(action, "clicked", G_CALLBACK(_esdashboard_middle_click_window_close_on_clicked), self);

	/* All done continue with traversal */
	return(ESDASHBOARD_TRAVERSAL_CONTINUE);
}

/* Callback for traversal to deconfigure live window from use at this plugin */
static gboolean _esdashboard_middle_click_window_close_traverse_release(ClutterActor *inActor,
																		gpointer inUserData)
{
	g_return_val_if_fail(ESDASHBOARD_IS_LIVE_WINDOW(inActor), ESDASHBOARD_TRAVERSAL_CONTINUE);
	g_return_val_if_fail(ESDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(inUserData), ESDASHBOARD_TRAVERSAL_CONTINUE);

	/* Set up live window */
	clutter_actor_remove_action_by_name(inActor, ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_ACTION_NAME);

	/* All done continue with traversal */
	return(ESDASHBOARD_TRAVERSAL_CONTINUE);
}

/* Stage is going to be destroyed */
static void _esdashboard_middle_click_window_close_on_stage_destroyed(EsdashboardMiddleClickWindowClose *self,
																		gpointer inUserData)
{
	EsdashboardMiddleClickWindowClosePrivate		*priv;
	EsdashboardStage								*stage;

	g_return_if_fail(ESDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(self));
	g_return_if_fail(ESDASHBOARD_IS_STAGE(inUserData));

	priv=self->priv;
	stage=ESDASHBOARD_STAGE(inUserData);

	/* Iterate through all existing live window actors that may still exist
	 * and deconfigure them from use at this plugin. We traverse the stage
	 * which is going to be destroyed and provided as function parameter
	 * regardless if it the stage we have set up initially or if it is any other.
	 */
	esdashboard_traverse_actor(CLUTTER_ACTOR(stage),
								priv->liveWindowSelector,
								_esdashboard_middle_click_window_close_traverse_release,
								self);

	/* Disconnect signals from stage as it will be destroyed and reset variables
	 * but only if it the stage we are handling right now (this should always be
	 * the case!)
	 */
	if(priv->stage==stage)
	{
		/* Disconnect signals */
		if(priv->stageActorCreatedSignalID)
		{
			g_signal_handler_disconnect(priv->stage, priv->stageActorCreatedSignalID);
			priv->stageActorCreatedSignalID=0;
		}

		if(priv->stageDestroySignalID)
		{
			g_signal_handler_disconnect(priv->stage, priv->stageDestroySignalID);
			priv->stageDestroySignalID=0;
		}

		/* Release stage */
		priv->stage=NULL;
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_middle_click_window_close_dispose(GObject *inObject)
{
	EsdashboardMiddleClickWindowClose				*self=ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE(inObject);
	EsdashboardMiddleClickWindowClosePrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->stage)
	{
		/* Iterate through all existing live window actors that may still exist
		 * and deconfigure them from use at this plugin.
		 */
		esdashboard_traverse_actor(CLUTTER_ACTOR(priv->stage),
									priv->liveWindowSelector,
									_esdashboard_middle_click_window_close_traverse_release,
									self);

		/* Disconnect signals from stage */
		if(priv->stageActorCreatedSignalID)
		{
			g_signal_handler_disconnect(priv->stage, priv->stageActorCreatedSignalID);
			priv->stageActorCreatedSignalID=0;
		}

		if(priv->stageDestroySignalID)
		{
			g_signal_handler_disconnect(priv->stage, priv->stageDestroySignalID);
			priv->stageDestroySignalID=0;
		}

		/* Release stage */
		priv->stage=NULL;
	}

	if(priv->liveWindowSelector)
	{
		g_object_unref(priv->liveWindowSelector);
		priv->liveWindowSelector=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_middle_click_window_close_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_middle_click_window_close_class_init(EsdashboardMiddleClickWindowCloseClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_middle_click_window_close_dispose;
}

/* Class finalization */
void esdashboard_middle_click_window_close_class_finalize(EsdashboardMiddleClickWindowCloseClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_middle_click_window_close_init(EsdashboardMiddleClickWindowClose *self)
{
	EsdashboardMiddleClickWindowClosePrivate		*priv;

	self->priv=priv=esdashboard_middle_click_window_close_get_instance_private(self);

	/* Set up default values */
	priv->stage=esdashboard_application_get_stage(NULL);
	priv->stageActorCreatedSignalID=0;
	priv->stageDestroySignalID=0;
	priv->liveWindowSelector=esdashboard_css_selector_new_from_string("EsdashboardWindowsView EsdashboardLiveWindow");

	/* Iterate through all already existing live window actors and configure
	 * them for use with this plugin.
	 */
	esdashboard_traverse_actor(CLUTTER_ACTOR(priv->stage),
								priv->liveWindowSelector,
								_esdashboard_middle_click_window_close_traverse_acquire,
								self);

	/* Connect signal to get notified about actor creations  and filter out
	 * and set up the ones we are interested in.
	 */
	priv->stageActorCreatedSignalID=
		g_signal_connect_swapped(priv->stage,
									"actor-created",
									G_CALLBACK(_esdashboard_middle_click_window_close_on_actor_created),
									self);

	/* Connect signal to get notified when stage is getting destoyed */
	priv->stageDestroySignalID=
		g_signal_connect_swapped(priv->stage,
									"destroy",
									G_CALLBACK(_esdashboard_middle_click_window_close_on_stage_destroyed),
									self);
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
EsdashboardMiddleClickWindowClose* esdashboard_middle_click_window_close_new(void)
{
	GObject		*middleClickWindowClose;

	middleClickWindowClose=g_object_new(ESDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE, NULL);
	if(!middleClickWindowClose) return(NULL);

	return(ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE(middleClickWindowClose));
}
