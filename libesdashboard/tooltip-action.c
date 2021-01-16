/*
 * tooltip-action: An action to display a tooltip after a short timeout
 *                 without movement at the referred actor
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

#include <libesdashboard/tooltip-action.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <libesdashboard/actor.h>
#include <libesdashboard/stage.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
struct _EsdashboardTooltipActionPrivate
{
	/* Properties related */
	gchar			*tooltipText;

	/* Instance related */
	ClutterPoint	lastPosition;

	guint			enterSignalID;
	guint			motionSignalID;
	guint			leaveSignalID;

	guint			captureSignalID;
	ClutterActor	*captureSignalActor;

	guint			timeoutSourceID;

	gboolean		isVisible;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardTooltipAction,
							esdashboard_tooltip_action,
							CLUTTER_TYPE_ACTION);

/* Properties */
enum
{
	PROP_0,

	PROP_TOOLTIP_TEXT,

	PROP_LAST
};

GParamSpec* EsdashboardTooltipActionProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ACTIVATING,

	SIGNAL_LAST
};

static guint EsdashboardTooltipActionSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_TOOLTIP_TIMEOUT 	500		/* Keep this tooltip timeout in sync with GTK+ */

static ClutterActor					*_esdashboard_tooltip_last_event_actor=NULL;

/* Pointer left actor with tooltip */
static gboolean _esdashboard_tooltip_action_on_leave_event(EsdashboardTooltipAction *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	EsdashboardTooltipActionPrivate		*priv;
	ClutterActor						*actor;
	ClutterActor						*stage;
	ClutterActor						*actorMeta;

	g_return_val_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;
	actor=CLUTTER_ACTOR(inUserData);

	/* Get current actor this action belongs to */
	actorMeta=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));

	/* Release all sources and signal handler (except for enter event) */
	if(priv->motionSignalID!=0)
	{
		if(actorMeta) g_signal_handler_disconnect(actorMeta, priv->motionSignalID);
		priv->motionSignalID=0;
	}

	if(priv->leaveSignalID!=0)
	{
		if(actorMeta) g_signal_handler_disconnect(actorMeta, priv->leaveSignalID);
		priv->leaveSignalID=0;
	}

	if(priv->captureSignalID)
	{
		if(priv->captureSignalActor) g_signal_handler_disconnect(priv->captureSignalActor, priv->captureSignalID);
		priv->captureSignalActor=NULL;
		priv->captureSignalID=0;
	}

	if(priv->timeoutSourceID!=0)
	{
		g_source_remove(priv->timeoutSourceID);
		priv->timeoutSourceID=0;
	}

	/* Clear last actor we remembered if it is pointing to this actor */
	if(_esdashboard_tooltip_last_event_actor==actor)
	{
		_esdashboard_tooltip_last_event_actor=NULL;
	}

	/* Hide tooltip now */
	stage=clutter_actor_get_stage(actor);
	if(stage && ESDASHBOARD_IS_STAGE(stage))
	{
		g_signal_emit_by_name(stage, "hide-tooltip", self, NULL);
		priv->isVisible=FALSE;
	}

	return(CLUTTER_EVENT_PROPAGATE);
}

/* An event after a tooltip was shown so check if tooltip should be hidden again */
static gboolean _esdashboard_tooltip_action_on_captured_event_after_tooltip(EsdashboardTooltipAction *self,
																			ClutterEvent *inEvent,
																			gpointer inUserData)
{
	gboolean		doHide;

	g_return_val_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(ESDASHBOARD_IS_STAGE(inUserData), CLUTTER_EVENT_PROPAGATE);

	/* Check if tooltip should be hidden depending on event type */
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_NOTHING:
		case CLUTTER_MOTION:
			doHide=FALSE;
			break;

		default:
			doHide=TRUE;
			break;
	}

	/* Hide tooltip if requested */
	if(doHide)
	{
		_esdashboard_tooltip_action_on_leave_event(self, inEvent, inUserData);
	}

	return(CLUTTER_EVENT_PROPAGATE);
}

/* Timeout for tooltip has been reached */
static gboolean _esdashboard_tooltip_action_on_timeout(gpointer inUserData)
{
	EsdashboardTooltipAction			*self;
	EsdashboardTooltipActionPrivate		*priv;
	ClutterActor						*actor;
	ClutterActor						*stage;

	g_return_val_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(inUserData), G_SOURCE_REMOVE);

	self=ESDASHBOARD_TOOLTIP_ACTION(inUserData);
	priv=self->priv;

	/* Regardless how this function ends we will let this source be
	 * removed from main loop. So forget source ID ;)
	 */
	priv->timeoutSourceID=0;

	/* Check if last seen actor is this actor. If not we cannot display
	 * a tooltip and remove this source on return.
	 */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
	if(actor!=_esdashboard_tooltip_last_event_actor) return(G_SOURCE_REMOVE);

	/* Show tooltip */
	stage=clutter_actor_get_stage(actor);
	if(stage && ESDASHBOARD_IS_STAGE(stage))
	{
		/* Emit 'activating' signal for last chance to update tooltip text */
		g_signal_emit(self, EsdashboardTooltipActionSignals[SIGNAL_ACTIVATING], 0);

		/* Show tooltip */
		g_signal_emit_by_name(stage, "show-tooltip", self, NULL);
		priv->isVisible=TRUE;
	}

	/* Remove source */
	return(G_SOURCE_REMOVE);
}

/* Pointer was moved over actor with tooltip */
static gboolean _esdashboard_tooltip_action_on_motion_event(EsdashboardTooltipAction *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	EsdashboardTooltipActionPrivate		*priv;
	ClutterActor						*actor;
	guint								tooltipTimeout;
	ClutterActor						*stage;

	g_return_val_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;
	actor=CLUTTER_ACTOR(inUserData);
	tooltipTimeout=0;

	/* Do nothing if tooltip is already visible */
	if(priv->isVisible) return(CLUTTER_EVENT_PROPAGATE);

	/* Remove any timeout source we have added for this actor */
	if(priv->timeoutSourceID!=0)
	{
		g_source_remove(priv->timeoutSourceID);
		priv->timeoutSourceID=0;
	}

	/* Remember position and actor */
	clutter_event_get_position(inEvent, &priv->lastPosition);
	_esdashboard_tooltip_last_event_actor=actor;

	/* Set up new timeout source */
#if GTK_CHECK_VERSION(3, 14 ,0)
	/* Since GTK+ version 3.10 the setting "gtk-tooltip-timeout" is
	 * not supported anymore and ignored by GTK+ derived application.
	 * So we should also. We set the timeout statically to the default
	 * duration which GTK+ is also using.
	 * This also prevents warning about forthcoming deprecation of this
	 * setting printed to console.
	 */
	tooltipTimeout=DEFAULT_TOOLTIP_TIMEOUT;
#else
	/* Get configured duration when a tooltip should be shown from
	 * GTK+ settings.
	 */
	g_object_get(gtk_settings_get_default(),
					"gtk-tooltip-timeout", &tooltipTimeout,
					NULL);
#endif

	priv->timeoutSourceID=clutter_threads_add_timeout(tooltipTimeout,
														(GSourceFunc)_esdashboard_tooltip_action_on_timeout,
														self);

	/* Capture next events to check if tooltip should be hidden again */
	stage=clutter_actor_get_stage(actor);
	if(stage && ESDASHBOARD_IS_STAGE(stage))
	{
		g_warn_if_fail((priv->captureSignalID==0 && priv->captureSignalActor==NULL) || (priv->captureSignalID!=0 && priv->captureSignalActor==stage));
		if((priv->captureSignalID==0 && priv->captureSignalActor==NULL) ||
			(priv->captureSignalID && priv->captureSignalActor!=stage))
		{
			if(priv->captureSignalActor) g_signal_handler_disconnect(priv->captureSignalActor, priv->captureSignalID);
			priv->captureSignalActor=NULL;
			priv->captureSignalID=0;

			priv->captureSignalActor=stage;
			priv->captureSignalID=g_signal_connect_swapped(stage,
															"captured-event",
															G_CALLBACK(_esdashboard_tooltip_action_on_captured_event_after_tooltip),
															self);
		}
	}

	return(CLUTTER_EVENT_PROPAGATE);
}

/* Pointer entered an actor with tooltip */
static gboolean _esdashboard_tooltip_action_on_enter_event(EsdashboardTooltipAction *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	EsdashboardTooltipActionPrivate		*priv;
	ClutterActor						*actor;

	g_return_val_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;
	actor=CLUTTER_ACTOR(inUserData);

	/* Remove any timeout source we have added for this actor */
	if(priv->timeoutSourceID!=0)
	{
		g_source_remove(priv->timeoutSourceID);
		priv->timeoutSourceID=0;
	}

	/* Connect signals */
	g_warn_if_fail(priv->motionSignalID==0);
	priv->motionSignalID=g_signal_connect_swapped(actor,
													"motion-event",
													G_CALLBACK(_esdashboard_tooltip_action_on_motion_event),
													self);

	g_warn_if_fail(priv->leaveSignalID==0);
	priv->leaveSignalID=g_signal_connect_swapped(actor,
													"leave-event",
													G_CALLBACK(_esdashboard_tooltip_action_on_leave_event),
													self);

	return(CLUTTER_EVENT_PROPAGATE);
}

/* IMPLEMENTATION: ClutterActorMeta */

/* Called when attaching and detaching a ClutterActorMeta instance to a ClutterActor */
static void _esdashboard_tooltip_action_set_actor(ClutterActorMeta *inActorMeta, ClutterActor *inActor)
{
	EsdashboardTooltipAction			*self;
	EsdashboardTooltipActionPrivate		*priv;
	ClutterActor						*oldActor;

	g_return_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(inActorMeta));

	self=ESDASHBOARD_TOOLTIP_ACTION(inActorMeta);
	priv=self->priv;

	/* Get current actor this action belongs to */
	oldActor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));

	/* Do nothing if new actor to set is the current one */
	if(oldActor==inActor) return;

	/* Release signals */
	if(priv->enterSignalID!=0)
	{
		if(oldActor!=NULL) g_signal_handler_disconnect(oldActor, priv->enterSignalID);
		priv->enterSignalID=0;
	}

	if(priv->motionSignalID!=0)
	{
		if(oldActor!=NULL) g_signal_handler_disconnect(oldActor, priv->motionSignalID);
		priv->motionSignalID=0;
	}

	if(priv->leaveSignalID!=0)
	{
		if(oldActor!=NULL) g_signal_handler_disconnect(oldActor, priv->leaveSignalID);
		priv->leaveSignalID=0;
	}

	if(priv->captureSignalID)
	{
		if(priv->captureSignalActor) g_signal_handler_disconnect(priv->captureSignalActor, priv->captureSignalID);
		priv->captureSignalActor=NULL;
		priv->captureSignalID=0;
	}

	/* Release sources */
	if(priv->timeoutSourceID!=0)
	{
		g_source_remove(priv->timeoutSourceID);
		priv->timeoutSourceID=0;
	}

	/* Connect signals */
	if(inActor!=NULL)
	{
		priv->enterSignalID=g_signal_connect_swapped(inActor,
													"enter-event",
													G_CALLBACK(_esdashboard_tooltip_action_on_enter_event),
													self);
	}

	/* Call parent's class method */
	if(CLUTTER_ACTOR_META_CLASS(esdashboard_tooltip_action_parent_class)->set_actor)
	{
		CLUTTER_ACTOR_META_CLASS(esdashboard_tooltip_action_parent_class)->set_actor(inActorMeta, inActor);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_tooltip_action_dispose(GObject *inObject)
{
	EsdashboardTooltipAction			*self=ESDASHBOARD_TOOLTIP_ACTION(inObject);
	EsdashboardTooltipActionPrivate		*priv=self->priv;
	ClutterActor						*actor;

	/* Get current actor this action belongs to */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));

	/* Release allocated resources */
	if(priv->enterSignalID!=0)
	{
		if(actor!=NULL) g_signal_handler_disconnect(actor, priv->enterSignalID);
		priv->enterSignalID=0;
	}

	if(priv->motionSignalID!=0)
	{
		if(actor!=NULL) g_signal_handler_disconnect(actor, priv->motionSignalID);
		priv->motionSignalID=0;
	}

	if(priv->leaveSignalID!=0)
	{
		if(actor!=NULL) g_signal_handler_disconnect(actor, priv->leaveSignalID);
		priv->leaveSignalID=0;
	}

	if(priv->captureSignalID)
	{
		if(actor!=NULL) g_signal_handler_disconnect(actor, priv->captureSignalID);
		priv->captureSignalID=0;
	}

	if(priv->timeoutSourceID!=0)
	{
		g_source_remove(priv->timeoutSourceID);
		priv->timeoutSourceID=0;
	}

	if(priv->tooltipText)
	{
		g_free(priv->tooltipText);
		priv->tooltipText=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_tooltip_action_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_tooltip_action_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardTooltipAction			*self=ESDASHBOARD_TOOLTIP_ACTION(inObject);

	switch(inPropID)
	{
		case PROP_TOOLTIP_TEXT:
			esdashboard_tooltip_action_set_text(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_tooltip_action_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardTooltipAction			*self=ESDASHBOARD_TOOLTIP_ACTION(inObject);
	EsdashboardTooltipActionPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_TOOLTIP_TEXT:
			g_value_set_string(outValue, priv->tooltipText);
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
static void esdashboard_tooltip_action_class_init(EsdashboardTooltipActionClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorMetaClass	*actorMetaClass=CLUTTER_ACTOR_META_CLASS(klass);

	/* Override functions */
	actorMetaClass->set_actor=_esdashboard_tooltip_action_set_actor;

	gobjectClass->dispose=_esdashboard_tooltip_action_dispose;
	gobjectClass->set_property=_esdashboard_tooltip_action_set_property;
	gobjectClass->get_property=_esdashboard_tooltip_action_get_property;

	/* Define properties */
	EsdashboardTooltipActionProperties[PROP_TOOLTIP_TEXT]=
		g_param_spec_string("tooltip-text",
								"Tooltip text",
								"The text to display in a tooltip",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardTooltipActionProperties);

	/* Define signals */
	EsdashboardTooltipActionSignals[SIGNAL_ACTIVATING]=
		g_signal_new("activating",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
						G_STRUCT_OFFSET(EsdashboardTooltipActionClass, activating),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_tooltip_action_init(EsdashboardTooltipAction *self)
{
	EsdashboardTooltipActionPrivate		*priv;

	priv=self->priv=esdashboard_tooltip_action_get_instance_private(self);

	/* Set up default values */
	priv->tooltipText=NULL;
	priv->enterSignalID=0;
	priv->motionSignalID=0;
	priv->leaveSignalID=0;
	priv->captureSignalID=0;
	priv->timeoutSourceID=0;
}

/* IMPLEMENTATION: Public API */

/* Create new action */
ClutterAction* esdashboard_tooltip_action_new(void)
{
	return(CLUTTER_ACTION(g_object_new(ESDASHBOARD_TYPE_TOOLTIP_ACTION, NULL)));
}

/* Get/set text of tooltip */
const gchar* esdashboard_tooltip_action_get_text(EsdashboardTooltipAction *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(self), NULL);

	return(self->priv->tooltipText);
}

void esdashboard_tooltip_action_set_text(EsdashboardTooltipAction *self, const gchar *inTooltipText)
{
	EsdashboardTooltipActionPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->tooltipText, inTooltipText)!=0)
	{
		/* Set value */
		if(priv->tooltipText)
		{
			g_free(priv->tooltipText);
			priv->tooltipText=NULL;
		}

		if(inTooltipText) priv->tooltipText=g_strdup(inTooltipText);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardTooltipActionProperties[PROP_TOOLTIP_TEXT]);
	}
}

/* Get position relative to actor where last event happened */
void esdashboard_tooltip_action_get_position(EsdashboardTooltipAction *self, gfloat *outX, gfloat *outY)
{
	EsdashboardTooltipActionPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(self));

	priv=self->priv;

	/* Set position */
	if(outX) *outX=priv->lastPosition.x;
	if(outY) *outY=priv->lastPosition.y;
}
