/*
 * click-action: Bad workaround for click action which prevent drag actions
 *               to work properly since clutter version 1.12 at least.
 *               This object/file is a complete copy of the original
 *               clutter-click-action.{c,h} files of clutter 1.12 except
 *               for one line, the renamed function names and the applied
 *               coding style. The clutter-click-action.{c,h} files of
 *               later clutter versions do not differ much from this one
 *               so this object should work also for this versions.
 *
 *               See bug: https://bugzilla.gnome.org/show_bug.cgi?id=714993
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
 *         original by Emmanuele Bassi <ebassi@linux.intel.com>
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

/**
 * SECTION:click-action
 * @short_Description: Action for clickable actors
 * @include: esdashboard/click-action.h
 *
 * #EsdashboardClickAction is a sub-class of #ClutterAction that implements
 * the logic for clickable actors, by using the low level events of
 * #ClutterActor, such as #ClutterActor::button-press-event and
 * #ClutterActor::button-release-event, to synthesize the high level
 * #ClutterClickAction::clicked signal.
 *
 * This action is a bad workaround for ClutterClickAction which prevents
 * drag actions to work properly (at least since clutter version 1.12).
 * #EsdashboardClickAction is a complete copy of the original ClutterClickAction
 * except for one line to get the click actions work with other added actions
 * like drag'n'drop actions.
 *
 * To use #EsdashboardClickAction you just need to apply it to a #ClutterActor
 * using clutter_actor_add_action() and connect to the
 * #EsdashboardClickAction::clicked signal:
 *
 * |[
 *   ClutterAction *action = esdashboard_click_action_new ();
 *
 *   clutter_actor_add_action (actor, action);
 *
 *   g_signal_connect (action, "clicked", G_CALLBACK (on_clicked), NULL);
 * ]|
 *
 * #EsdashboardClickAction also supports long press gestures: a long press
 * is activated if the pointer remains pressed within a certain threshold
 * (as defined by the #EsdashboardClickAction:long-press-threshold property)
 * for a minimum amount of time (as the defined by the
 * #EsdashboardClickAction:long-press-duration property).
 * The #EsdashboardClickAction::long-press signal is emitted multiple times,
 * using different #ClutterLongPressState values; to handle long presses
 * you should connect to the #EsdashboardClickAction::long-press signal and
 * handle the different states:
 *
 * |[
 *   static gboolean
 *   on_long_press (EsdashboardClickAction *self,
 *                  ClutterActor           *inActor,
 *                  ClutterLongPressState   inState)
 *   {
 *     switch (inState)
 *     {
 *       case CLUTTER_LONG_PRESS_QUERY:
 *         /&ast; return TRUE if the actor should support long press
 *          &ast; gestures, and FALSE otherwise; this state will be
 *          &ast; emitted on button presses
 *          &ast;/
 *         return(TRUE);
 *
 *       case CLUTTER_LONG_PRESS_ACTIVATE:
 *         /&ast; this state is emitted if the minimum duration has
 *          &ast; been reached without the gesture being cancelled.
 *          &ast; the return value is not used
 *          &ast;/
 *         return(TRUE);
 *
 *       case CLUTTER_LONG_PRESS_CANCEL:
 *         /&ast; this state is emitted if the long press was cancelled;
 *          &ast; for instance, the pointer went outside the actor or the
 *          &ast; allowed threshold, or the button was released before
 *          &ast; the minimum duration was reached. the return value is
 *          &ast; not used
 *          &ast;/
 *         return(FALSE);
 *     }
 *   }
 * ]|
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/click-action.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/marshal.h>
#include <libesdashboard/actor.h>
#include <libesdashboard/stylable.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
struct _EsdashboardClickActionPrivate
{
	/* Properties related */
	guint					isHeld : 1;
	guint					isPressed : 1;

	gint					longPressThreshold;
	gint					longPressDuration;

	/* Instance related */
	ClutterActor			*stage;

	guint					eventID;
	guint					captureID;
	guint					longPressID;

	gint					dragThreshold;

	guint					pressButton;
	gint					pressDeviceID;
	ClutterEventSequence	*pressSequence;
	ClutterModifierType		modifierState;
	gfloat					pressX;
	gfloat					pressY;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardClickAction,
							esdashboard_click_action,
							CLUTTER_TYPE_ACTION);

/* Properties */
enum
{
	PROP_0,

	PROP_HELD,
	PROP_PRESSED,
	PROP_LONG_PRESS_THRESHOLD,
	PROP_LONG_PRESS_DURATION,

	PROP_LAST
};

GParamSpec* EsdashboardClickActionProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CLICKED,
	SIGNAL_LONG_PRESS,

	SIGNAL_LAST
};

guint EsdashboardClickActionSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Set press state */
static void _esdashboard_click_action_set_pressed(EsdashboardClickAction *self, gboolean isPressed)
{
	EsdashboardClickActionPrivate	*priv;
	ClutterActor					*actor;

	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;

	/* Set value if changed */
	isPressed=!!isPressed;

	if(priv->isPressed!=isPressed)
	{
		/* Set value */
		priv->isPressed=isPressed;

		/* Style state */
		actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
		if(ESDASHBOARD_IS_ACTOR(actor))
		{
			if(priv->isPressed) esdashboard_stylable_add_pseudo_class(ESDASHBOARD_STYLABLE(actor), "pressed");
				else esdashboard_stylable_remove_pseudo_class(ESDASHBOARD_STYLABLE(actor), "pressed");
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardClickActionProperties[PROP_PRESSED]);
	}
}

/* Set held state */
static void _esdashboard_click_action_set_held(EsdashboardClickAction *self, gboolean isHeld)
{
	EsdashboardClickActionPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;

	/* Set value if changed */
	isHeld=!!isHeld;

	if(priv->isHeld!=isHeld)
	{
		/* Set value */
		priv->isHeld=isHeld;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardClickActionProperties[PROP_HELD]);
	}
}

/* Emit "long-press" signal */
static gboolean _esdashboard_click_action_emit_long_press(gpointer inUserData)
{
	EsdashboardClickAction			*self;
	EsdashboardClickActionPrivate	*priv;
	ClutterActor					*actor;
	gboolean						result;

	g_return_val_if_fail(ESDASHBOARD_IS_CLICK_ACTION(inUserData), FALSE);

	self=ESDASHBOARD_CLICK_ACTION(inUserData);
	priv=self->priv;

	/* Reset variables */
	priv->longPressID=0;

	/* Emit signal */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(inUserData));
	g_signal_emit(self, EsdashboardClickActionSignals[SIGNAL_LONG_PRESS], 0, actor, CLUTTER_LONG_PRESS_ACTIVATE, &result);

	/* Disconnect signal handlers */
	if(priv->captureID!=0)
	{
		g_signal_handler_disconnect(priv->stage, priv->captureID);
		priv->captureID=0;
	}

	/* Reset state of this action */
	_esdashboard_click_action_set_pressed(self, FALSE);
	_esdashboard_click_action_set_held(self, FALSE);

	/* Event handled */
	return(FALSE);
}

/* Query if long-press events should be handled and signals emitted */
static void _esdashboard_click_action_query_long_press(EsdashboardClickAction *self)
{
	EsdashboardClickActionPrivate	*priv;
	ClutterActor					*actor;
	gboolean						result;
	gint							timeout;

	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;
	result=FALSE;

	/* If no duration was set get default one from settings */
	if(priv->longPressDuration<0)
	{
		ClutterSettings				*settings=clutter_settings_get_default();

		g_object_get(settings, "long-press-duration", &timeout, NULL);
	}
		else timeout=priv->longPressDuration;

	/* Emit signal to determine if long-press should be supported */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
	g_signal_emit(self, EsdashboardClickActionSignals[SIGNAL_LONG_PRESS], 0, actor, CLUTTER_LONG_PRESS_QUERY, &result);

	if(result)
	{
		priv->longPressID=clutter_threads_add_timeout(timeout,
														_esdashboard_click_action_emit_long_press,
														self);
	}
}

/* Cancel long-press handling */
static void _esdashboard_click_action_cancel_long_press(EsdashboardClickAction *self)
{
	EsdashboardClickActionPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;

	/* Remove signals/sources and emit cancel signal */
	if(priv->longPressID!=0)
	{
		ClutterActor				*actor;
		gboolean					result;

		/* Remove source */
		g_source_remove(priv->longPressID);
		priv->longPressID=0;

		/* Emit signal */
		actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
		g_signal_emit(self, EsdashboardClickActionSignals[SIGNAL_LONG_PRESS], 0, actor, CLUTTER_LONG_PRESS_CANCEL, &result);
	}
}

/* An event was captured */
static gboolean _esdashboard_click_action_on_captured_event(EsdashboardClickAction *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	EsdashboardClickActionPrivate	*priv;
	ClutterActor					*stage G_GNUC_UNUSED;
	ClutterActor					*actor;
	ClutterModifierType				modifierState;
	gboolean						hasButton;

	g_return_val_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;
	stage=CLUTTER_ACTOR(inUserData);
	hasButton=TRUE;

	/* Handle captured event */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_TOUCH_END:
		case CLUTTER_BUTTON_RELEASE:
			if(!priv->isHeld) return(CLUTTER_EVENT_STOP);

			hasButton=(clutter_event_type(inEvent)==CLUTTER_TOUCH_END ? FALSE : TRUE);

			if((hasButton && clutter_event_get_button(inEvent)!=priv->pressButton) ||
				(hasButton && clutter_event_get_click_count(inEvent)!=1) ||
				clutter_event_get_device_id(inEvent)!=priv->pressDeviceID ||
				clutter_event_get_event_sequence(inEvent)!=priv->pressSequence)
			{
				return(CLUTTER_EVENT_PROPAGATE);
			}

			_esdashboard_click_action_set_held(self, FALSE);
			_esdashboard_click_action_cancel_long_press(self);

			/* Disconnect the capture */
			if(priv->captureID!=0)
			{
				g_signal_handler_disconnect(priv->stage, priv->captureID);
				priv->captureID = 0;
			}

			if(priv->longPressID!=0)
			{
				g_source_remove(priv->longPressID);
				priv->longPressID=0;
			}

			if(!clutter_actor_contains(actor, clutter_event_get_source(inEvent)))
			{
				return(CLUTTER_EVENT_PROPAGATE);
			}

			/* Exclude any button-mask so that we can compare
			 * the press and release states properly
			 */
			modifierState=clutter_event_get_state(inEvent) &
							~(CLUTTER_BUTTON1_MASK |
								CLUTTER_BUTTON2_MASK |
								CLUTTER_BUTTON3_MASK |
								CLUTTER_BUTTON4_MASK |
								CLUTTER_BUTTON5_MASK);

			/* If press and release states don't match we simply ignore
			 * modifier keys. i.e. modifier keys are expected to be pressed
			 * throughout the whole click
			 */
			if(modifierState!=priv->modifierState) priv->modifierState=0;

			_esdashboard_click_action_set_pressed(self, FALSE);
			g_signal_emit(self, EsdashboardClickActionSignals[SIGNAL_CLICKED], 0, actor);
			break;

		case CLUTTER_MOTION:
		case CLUTTER_TOUCH_UPDATE:
			{
				gfloat				motionX, motionY;
				gfloat				deltaX, deltaY;

				if(!priv->isHeld) return(CLUTTER_EVENT_PROPAGATE);

				clutter_event_get_coords (inEvent, &motionX, &motionY);

				deltaX=ABS(motionX-priv->pressX);
				deltaY=ABS(motionY-priv->pressY);

				if(deltaX>priv->dragThreshold || deltaY>priv->dragThreshold)
				{
					_esdashboard_click_action_cancel_long_press(self);
				}
			}
			break;

		default:
			break;
	}

	/* This is line changed in returning CLUTTER_EVENT_PROPAGATE
	 * instead of CLUTTER_EVENT_STOP
	 */
	return(CLUTTER_EVENT_PROPAGATE);
}

/* An event was received */
static gboolean _esdashboard_click_action_on_event(EsdashboardClickAction *self, ClutterEvent *inEvent, gpointer inUserData)
{
	EsdashboardClickActionPrivate	*priv;
	gboolean						hasButton;
	ClutterActor					*actor;

	g_return_val_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;
	hasButton=TRUE;
	actor=CLUTTER_ACTOR(inUserData);

	/* Check if actor is enabled to handle events */
	if(!clutter_actor_meta_get_enabled(CLUTTER_ACTOR_META(self))) return(CLUTTER_EVENT_PROPAGATE);

	/* Handle event */
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_TOUCH_BEGIN:
		case CLUTTER_BUTTON_PRESS:
			hasButton=(clutter_event_type(inEvent)==CLUTTER_TOUCH_BEGIN ? FALSE : TRUE);

			/* We only handle single clicks if it is pointer device */
			if(hasButton && clutter_event_get_click_count(inEvent)!=1)
			{
				return(CLUTTER_EVENT_PROPAGATE);
			}

			/* Do we already held the press? */
			if(priv->isHeld) return(CLUTTER_EVENT_STOP);

			/* Is the source of event a child of this actor. If not do
			 * not handle this event but any other.
			 */
			if(!clutter_actor_contains(actor, clutter_event_get_source(inEvent)))
			{
				return(CLUTTER_EVENT_PROPAGATE);
			}

			/* Remember event data */
			priv->pressButton=hasButton ? clutter_event_get_button(inEvent) : 0;
			priv->pressDeviceID=clutter_event_get_device_id(inEvent);
			priv->pressSequence=clutter_event_get_event_sequence(inEvent);
			priv->modifierState=clutter_event_get_state(inEvent);
			clutter_event_get_coords(inEvent, &priv->pressX, &priv->pressY);

			if(priv->longPressThreshold<0)
			{
				ClutterSettings		*settings=clutter_settings_get_default();

				g_object_get(settings, "dnd-drag-threshold", &priv->dragThreshold, NULL);
			}
				else priv->dragThreshold=priv->longPressThreshold;

			if(priv->stage==NULL) priv->stage=clutter_actor_get_stage(actor);

			/* Connect signals */
			priv->captureID=g_signal_connect_object(priv->stage,
													"captured-event",
													G_CALLBACK(_esdashboard_click_action_on_captured_event),
													self,
													G_CONNECT_AFTER | G_CONNECT_SWAPPED);

			/* Set state of this action */
			_esdashboard_click_action_set_pressed(self, TRUE);
			_esdashboard_click_action_set_held(self, TRUE);
			_esdashboard_click_action_query_long_press(self);
			break;

		case CLUTTER_ENTER:
			_esdashboard_click_action_set_pressed(self, priv->isHeld);
			break;

		case CLUTTER_LEAVE:
			_esdashboard_click_action_set_pressed(self, priv->isHeld);
			_esdashboard_click_action_cancel_long_press(self);
			break;

		default:
			break;
	}

	return(CLUTTER_EVENT_PROPAGATE);
}

/* IMPLEMENTATION: ClutterActorMeta */

/* Called when attaching and detaching a ClutterActorMeta instance to a ClutterActor */
static void _esdashboard_click_action_set_actor(ClutterActorMeta *inActorMeta, ClutterActor *inActor)
{
	EsdashboardClickAction			*self;
	EsdashboardClickActionPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(inActorMeta));

	self=ESDASHBOARD_CLICK_ACTION(inActorMeta);
	priv=self->priv;

	/* Disconnect signals and remove sources */
	if(priv->eventID!=0)
	{
		ClutterActor				*oldActor=clutter_actor_meta_get_actor(inActorMeta);

		if(oldActor!=NULL) g_signal_handler_disconnect(oldActor, priv->eventID);
		priv->eventID=0;
	}

	if(priv->captureID!=0)
	{
		if(priv->stage!=NULL) g_signal_handler_disconnect(priv->stage, priv->captureID);
		priv->captureID=0;
		priv->stage=NULL;
	}

	if(priv->longPressID!=0)
	{
		g_source_remove(priv->longPressID);
		priv->longPressID=0;
	}

	/* Reset state of this action */
	_esdashboard_click_action_set_pressed(self, FALSE);
	_esdashboard_click_action_set_held(self, FALSE);

	/* Connect signals */
	if(inActor!=NULL)
	{
		priv->eventID=g_signal_connect_swapped(inActor,
													"event",
													G_CALLBACK(_esdashboard_click_action_on_event),
													self);
	}

	/* Call parent's class method */
	if(CLUTTER_ACTOR_META_CLASS(esdashboard_click_action_parent_class)->set_actor)
	{
		CLUTTER_ACTOR_META_CLASS(esdashboard_click_action_parent_class)->set_actor(inActorMeta, inActor);
	}
}

/* IMPLEMENTATION: GObject */

/* Set/get properties */
static void _esdashboard_click_action_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardClickAction			*self=ESDASHBOARD_CLICK_ACTION(inObject);
	EsdashboardClickActionPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_LONG_PRESS_DURATION:
			priv->longPressDuration=g_value_get_int(inValue);
			break;

		case PROP_LONG_PRESS_THRESHOLD:
			priv->longPressThreshold=g_value_get_int(inValue);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_click_action_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardClickAction			*self=ESDASHBOARD_CLICK_ACTION(inObject);
	EsdashboardClickActionPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_HELD:
			g_value_set_boolean(outValue, priv->isHeld);
			break;

		case PROP_PRESSED:
			g_value_set_boolean(outValue, priv->isPressed);
			break;

		case PROP_LONG_PRESS_DURATION:
			g_value_set_int(outValue, priv->longPressDuration);
			break;

		case PROP_LONG_PRESS_THRESHOLD:
			g_value_set_int(outValue, priv->longPressThreshold);
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
static void esdashboard_click_action_class_init(EsdashboardClickActionClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorMetaClass	*actorMetaClass=CLUTTER_ACTOR_META_CLASS(klass);

	/* Override functions */
	actorMetaClass->set_actor=_esdashboard_click_action_set_actor;

	gobjectClass->set_property=_esdashboard_click_action_set_property;
	gobjectClass->get_property=_esdashboard_click_action_get_property;

	/* Define properties */
	/**
	 * EsdashboardClickAction:pressed:
	 *
	 * Whether the clickable actor should be in "pressed" state
	 */
	EsdashboardClickActionProperties[PROP_PRESSED]=
		g_param_spec_boolean("pressed",
								"Pressed",
								"Whether the clickable should be in pressed state",
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardClickAction:held:
	 *
	 * Whether the clickable actor has the pointer grabbed
	 */
	EsdashboardClickActionProperties[PROP_HELD]=
		g_param_spec_boolean("held",
								"Held",
								"Whether the clickable has a grab",
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardClickAction:long-press-duration:
	 *
	 * The minimum duration of a press for it to be recognized as a long press
	 * gesture, in milliseconds.
	 *
	 * A value of -1 will make the #EsdashboardClickAction use the value of the
	 * #ClutterSettings:long-press-duration property.
	 */
	EsdashboardClickActionProperties[PROP_LONG_PRESS_DURATION]=
		g_param_spec_int("long-press-duration",
							"Long Press Duration",
							"The minimum duration of a long press to recognize the gesture",
							-1,
							G_MAXINT,
							-1,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardClickAction:long-press-threshold:
	 *
	 * The maximum allowed distance that can be covered (on both axes) before
	 * a long press gesture is cancelled, in pixels.
	 *
	 * A value of -1 will make the #EsdashboardClickAction use the value of the
	 * #ClutterSettings:dnd-drag-threshold property.
	 */
	EsdashboardClickActionProperties[PROP_LONG_PRESS_THRESHOLD]=
		g_param_spec_int("long-press-threshold",
							"Long Press Threshold",
							"The maximum threshold before a long press is cancelled",
							-1,
							G_MAXINT,
							-1,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardClickActionProperties);

	/* Define signals */
	/**
	 * EsdashboardClickAction::clicked:
	 * @self: The #EsdashboardClickAction that emitted the signal
	 * @inActor: The #ClutterActor attached to @self
	 *
	 * The ::clicked signal is emitted when the #ClutterActor to which
	 * a #EsdashboardClickAction has been applied should respond to a
	 * pointer button press and release events
	 */
	EsdashboardClickActionSignals[SIGNAL_CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardClickActionClass, clicked),
						NULL, NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						CLUTTER_TYPE_ACTOR);

	/**
	 * EsdashboardClickAction::long-press:
	 * @self: The #EsdashboardClickAction that emitted the signal
	 * @inActor: The #ClutterActor attached to @self
	 * @inState: The long press state
	 *
	 * The ::long-press signal is emitted during the long press gesture
	 * handling.
	 *
	 * This signal can be emitted multiple times with different states.
	 *
	 * The %CLUTTER_LONG_PRESS_QUERY state will be emitted on button presses,
	 * and its return value will determine whether the long press handling
	 * should be initiated. If the signal handlers will return %TRUE, the
	 * %CLUTTER_LONG_PRESS_QUERY state will be followed either by a signal
	 * emission with the %CLUTTER_LONG_PRESS_ACTIVATE state if the long press
	 * constraints were respected, or by a signal emission with the
	 * %CLUTTER_LONG_PRESS_CANCEL state if the long press was cancelled.
	 *
	 * It is possible to forcibly cancel a long press detection using
	 * esdashboard_click_action_release().
	 *
	 * Return value: Only the %CLUTTER_LONG_PRESS_QUERY state uses the
	 *   returned value of the handler; other states will ignore it
	 */
	EsdashboardClickActionSignals[SIGNAL_LONG_PRESS]=
		g_signal_new("long-press",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardClickActionClass, long_press),
						NULL, NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_ENUM,
						G_TYPE_BOOLEAN,
						2,
						CLUTTER_TYPE_ACTOR,
						CLUTTER_TYPE_LONG_PRESS_STATE);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_click_action_init(EsdashboardClickAction *self)
{
	EsdashboardClickActionPrivate	*priv;

	priv=self->priv=esdashboard_click_action_get_instance_private(self);

	/* Set up default values */
	priv->longPressThreshold=-1;
	priv->longPressDuration=-1;
}

/* IMPLEMENTATION: Public API */

/**
 * esdashboard_click_action_new:
 *
 * Creates a new #EsdashboardClickAction instance
 *
 * Return value: The newly created #EsdashboardClickAction
 */
ClutterAction* esdashboard_click_action_new(void)
{
	return(CLUTTER_ACTION(g_object_new(ESDASHBOARD_TYPE_CLICK_ACTION, NULL)));
}

/**
 * esdashboard_click_action_get_button:
 * @self: A #EsdashboardClickAction
 *
 * Retrieves the button that was pressed.
 *
 * Return value: The button value
 */
guint esdashboard_click_action_get_button(EsdashboardClickAction *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self), 0);

	return(self->priv->pressButton);
}

/**
 * esdashboard_click_action_get_state:
 * @self: A #EsdashboardClickAction
 *
 * Retrieves the modifier state of the click action.
 *
 * Return value: The modifier state parameter or 0
 */
ClutterModifierType esdashboard_click_action_get_state(EsdashboardClickAction *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self), 0);

	return(self->priv->modifierState);
}

/**
 * esdashboard_click_action_get_coords:
 * @self: A #EsdashboardClickAction
 * @outPressX: (out): Return location for the X coordinate or %NULL
 * @outPressY: (out): Return location for the Y coordinate or %NULL
 *
 * Retrieves the screen coordinates of the button press.
 */
void esdashboard_click_action_get_coords(EsdashboardClickAction *self, gfloat *outPressX, gfloat *outPressY)
{
	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self));

	if(outPressX!=NULL) *outPressX=self->priv->pressX;
	if(outPressY!=NULL) *outPressY=self->priv->pressY;
}

/**
 * esdashboard_click_action_release:
 * @self: A #EsdashboardClickAction
 *
 * Emulates a release of the pointer button, which ungrabs the pointer
 * and unsets the #EsdashboardClickAction:pressed state.
 *
 * This function will also cancel the long press gesture if one was
 * initiated.
 *
 * This function is useful to break a grab, for instance after a certain
 * amount of time has passed.
 */
void esdashboard_click_action_release(EsdashboardClickAction *self)
{
	EsdashboardClickActionPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;

	/* Only release pointer button if it is held by this action */
	if(!priv->isHeld) return;

	/* Disconnect signal handlers */
	if(priv->captureID!=0)
	{
		g_signal_handler_disconnect(priv->stage, priv->captureID);
		priv->captureID=0;
	}

	/* Reset state of this action */
	_esdashboard_click_action_cancel_long_press(self);
	_esdashboard_click_action_set_held(self, FALSE);
	_esdashboard_click_action_set_pressed(self, FALSE);
}

/**
 * esdashboard_click_action_is_left_button_or_touch
 * @self: A #EsdashboardClickAction
 *
 * Checks if the specified click action is either a left button press or a single touch 'tap'
 *
 * Return value: Returns %TRUE if the click action event is a left button press or a single 
 * touch tap, otherwise %FALSE
 */
gboolean esdashboard_click_action_is_left_button_or_tap(EsdashboardClickAction *self)
{
	EsdashboardClickActionPrivate *priv;
	g_return_val_if_fail(ESDASHBOARD_IS_CLICK_ACTION(self), FALSE);

	priv=self->priv;

	if(priv->pressButton==0 || priv->pressButton==ESDASHBOARD_CLICK_ACTION_LEFT_BUTTON)
	{
		return(TRUE);
	}

	return(FALSE);
}
