/*
 * action-button: A button representing an action to execute when clicked
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

/**
 * SECTION:action-button
 * @short_description: A button to perform a key binding action
 * @include: esdashboard/action-button.h
 *
 * This actor is a #EsdashboardButton and behaves exactly like a key binding which
 * performs a specified action on a specific actor when the associated key
 * combination is pressed. But instead of a key combination a button is displayed
 * and the action performed when this button is clicked.
 *
 * A #EsdashboardActionButton is usually created in the layout definition
 * of a theme but it can also be created with esdashboard_action_button_new()
 * followed by a call to esdashboard_action_button_set_target() and
 * esdashboard_action_button_set_action() to configure it.
 *
 * For example a #EsdashboardActionButton can be created which will quit the
 * application when clicked:
 *
 * |[<!-- language="C" -->
 *   ClutterActor       *actionButton;
 *
 *   actionButton=esdashboard_action_button_new();
 *   esdashboard_action_button_set_target(ESDASHBOARD_ACTION_BUTTON(actionButton), "EsdashboardApplication");
 *   esdashboard_action_button_set_action(ESDASHBOARD_ACTION_BUTTON(actionButton), "exit");
 * ]|
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/action-button.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/focusable.h>
#include <libesdashboard/focus-manager.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
static void _esdashboard_action_button_focusable_iface_init(EsdashboardFocusableInterface *iface);

struct _EsdashboardActionButtonPrivate
{
	/* Properties related */
	gchar								*target;
	gchar								*action;

	/* Instance related */
	EsdashboardFocusManager				*focusManager;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardActionButton,
						esdashboard_action_button,
						ESDASHBOARD_TYPE_BUTTON,
						G_ADD_PRIVATE(EsdashboardActionButton)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_FOCUSABLE, _esdashboard_action_button_focusable_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_TARGET,
	PROP_ACTION,

	PROP_LAST
};

static GParamSpec* EsdashboardActionButtonProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* This button was clicked */
static void _esdashboard_action_button_clicked(EsdashboardButton *inButton)
{
	EsdashboardActionButton				*self;
	EsdashboardActionButtonPrivate		*priv;
	GSList								*targets;
	GSList								*iter;

	g_return_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(inButton));

	self=ESDASHBOARD_ACTION_BUTTON(inButton);
	priv=self->priv;
	targets=NULL;

	/* Get target object to perform action at */
	targets=esdashboard_focus_manager_get_targets(priv->focusManager, priv->target);
	ESDASHBOARD_DEBUG(self, ACTOR, "Target list for '%s' has %d entries",
						priv->target,
						g_slist_length(targets));

	/* Emit action at each actor in target list */
	for(iter=targets; iter; iter=g_slist_next(iter))
	{
		GObject							*targetObject;
		guint							signalID;
		GSignalQuery					signalData={ 0, };
		const ClutterEvent				*event;
		gboolean						eventStatus;

		/* Get target to emit action signal at */
		targetObject=G_OBJECT(iter->data);

		/* Check if target provides action requested as signal */
		signalID=g_signal_lookup(priv->action, G_OBJECT_TYPE(targetObject));
		if(!signalID)
		{
			g_warning("Object type %s does not provide action '%s'",
						G_OBJECT_TYPE_NAME(targetObject),
						priv->action);
			continue;
		}

		/* Query signal for detailed data */
		g_signal_query(signalID, &signalData);

		/* Check if signal is an action signal */
		if(!(signalData.signal_flags & G_SIGNAL_ACTION))
		{
			g_warning("Action '%s' at object type %s is not an action signal.",
						priv->action,
						G_OBJECT_TYPE_NAME(targetObject));
			continue;
		}

		/* In debug mode also check if signal has right signature
		 * to be able to handle this action properly.
		 */
		if(signalID)
		{
			GType						returnValueType=G_TYPE_BOOLEAN;
			GType						parameterTypes[]={ ESDASHBOARD_TYPE_FOCUSABLE, G_TYPE_STRING, CLUTTER_TYPE_EVENT };
			guint						parameterCount;
			guint						i;

			/* Check if signal wants the right type of return value */
			if(signalData.return_type!=returnValueType)
			{
				g_critical("Action '%s' at object type %s wants return value of type %s but expected is %s.",
							priv->action,
							G_OBJECT_TYPE_NAME(targetObject),
							g_type_name(signalData.return_type),
							g_type_name(returnValueType));
			}

			/* Check if signals wants the right number and types of parameters */
			parameterCount=sizeof(parameterTypes)/sizeof(GType);
			if(signalData.n_params!=parameterCount)
			{
				g_critical("Action '%s' at object type %s wants %u parameters but expected are %u.",
							priv->action,
							G_OBJECT_TYPE_NAME(targetObject),
							signalData.n_params,
							parameterCount);
			}

			for(i=0; i<(parameterCount<signalData.n_params ? parameterCount : signalData.n_params); i++)
			{
				if(signalData.param_types[i]!=parameterTypes[i])
				{
					g_critical("Action '%s' at object type %s wants type %s at parameter %u but type %s is expected.",
								priv->action,
								G_OBJECT_TYPE_NAME(targetObject),
								g_type_name(signalData.param_types[i]),
								i+1,
								g_type_name(parameterTypes[i]));
				}
			}
		}

		/* Emit action signal at target */
		ESDASHBOARD_DEBUG(self, ACTOR, "Emitting action signal '%s' at actor %s",
							priv->action,
							G_OBJECT_TYPE_NAME(targetObject));

		event=clutter_get_current_event();
		eventStatus=CLUTTER_EVENT_PROPAGATE;
		g_signal_emit_by_name(targetObject,
								priv->action,
								ESDASHBOARD_FOCUSABLE(self),
								priv->action,
								event,
								&eventStatus);

		ESDASHBOARD_DEBUG(self, ACTOR, "Action signal '%s' was %s by actor %s",
							priv->action,
							eventStatus==CLUTTER_EVENT_STOP ? "handled" : "not handled",
							G_OBJECT_TYPE_NAME(targetObject));
	}

	/* Release allocated resources */
	if(targets) g_slist_free_full(targets, g_object_unref);
}

/* IMPLEMENTATION: Interface EsdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _esdashboard_action_button_focusable_can_focus(EsdashboardFocusable *inFocusable)
{
	EsdashboardFocusableInterface		*selfIface;
	EsdashboardFocusableInterface		*parentIface;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(inFocusable), FALSE);

	/* Call parent class interface function */
	selfIface=ESDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->can_focus)
	{
		if(!parentIface->can_focus(inFocusable)) return(FALSE);
	}

	/* If we get here this actor can be focused */
	return(TRUE);
}

/* Determine if this actor supports selection */
static gboolean _esdashboard_action_button_focusable_supports_selection(EsdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}

/* Get current selection */
static ClutterActor* _esdashboard_action_button_focusable_get_selection(EsdashboardFocusable *inFocusable)
{
	EsdashboardActionButton		*self;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(inFocusable), NULL);

	self=ESDASHBOARD_ACTION_BUTTON(inFocusable);

	/* Return the actor itself as current selection */
	return(CLUTTER_ACTOR(self));
}

/* Set new selection */
static gboolean _esdashboard_action_button_focusable_set_selection(EsdashboardFocusable *inFocusable,
																	ClutterActor *inSelection)
{
	EsdashboardActionButton				*self;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_ACTION_BUTTON(inFocusable);

	/* Setting new selection always fails if it is not this actor itself */
	if(inSelection!=CLUTTER_ACTOR(self)) return(FALSE);

	/* Otherwise setting selection was successful because nothing has changed */
	return(TRUE);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _esdashboard_action_button_focusable_find_selection(EsdashboardFocusable *inFocusable,
																			ClutterActor *inSelection,
																			EsdashboardSelectionTarget inDirection)
{
	EsdashboardActionButton				*self;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(inFocusable), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	self=ESDASHBOARD_ACTION_BUTTON(inFocusable);

	/* Regardless of "current" selection and direction requested for new selection
	 * we return this actor as new current selection resulting in no change of
	 * selection. It is and will be the actor itself.
	 */
	return(CLUTTER_ACTOR(self));
}

/* Activate selection */
static gboolean _esdashboard_action_button_focusable_activate_selection(EsdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	EsdashboardActionButton				*self;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(inFocusable), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_ACTION_BUTTON(inFocusable);

	/* Activate selection */
	_esdashboard_action_button_clicked(ESDASHBOARD_BUTTON(self));

	return(TRUE);
}

/* Interface initialization
 * Set up default functions
 */
void _esdashboard_action_button_focusable_iface_init(EsdashboardFocusableInterface *iface)
{
	iface->can_focus=_esdashboard_action_button_focusable_can_focus;

	iface->supports_selection=_esdashboard_action_button_focusable_supports_selection;
	iface->get_selection=_esdashboard_action_button_focusable_get_selection;
	iface->set_selection=_esdashboard_action_button_focusable_set_selection;
	iface->find_selection=_esdashboard_action_button_focusable_find_selection;
	iface->activate_selection=_esdashboard_action_button_focusable_activate_selection;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_action_button_dispose(GObject *inObject)
{
	EsdashboardActionButton				*self=ESDASHBOARD_ACTION_BUTTON(inObject);
	EsdashboardActionButtonPrivate		*priv=self->priv;

	/* Release our allocated variables */
	if(priv->focusManager)
	{
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	if(priv->target)
	{
		g_free(priv->target);
		priv->target=NULL;
	}

	if(priv->action)
	{
		g_free(priv->action);
		priv->action=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_action_button_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_action_button_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	EsdashboardActionButton			*self=ESDASHBOARD_ACTION_BUTTON(inObject);

	switch(inPropID)
	{
		case PROP_TARGET:
			esdashboard_action_button_set_target(self, g_value_get_string(inValue));
			break;

		case PROP_ACTION:
			esdashboard_action_button_set_action(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_action_button_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	EsdashboardActionButton			*self=ESDASHBOARD_ACTION_BUTTON(inObject);
	EsdashboardActionButtonPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_TARGET:
			g_value_set_string(outValue, priv->target);
			break;

		case PROP_ACTION:
			g_value_set_string(outValue, priv->action);
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
static void esdashboard_action_button_class_init(EsdashboardActionButtonClass *klass)
{
	EsdashboardButtonClass	*buttonClass=ESDASHBOARD_BUTTON_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	buttonClass->clicked=_esdashboard_action_button_clicked;

	gobjectClass->dispose=_esdashboard_action_button_dispose;
	gobjectClass->set_property=_esdashboard_action_button_set_property;
	gobjectClass->get_property=_esdashboard_action_button_get_property;

	/* Define properties */
	/**
	 * EsdashboardActionButton:target:
	 *
	 * A string with the class name of target at which the action should be
	 * performed.
	 */
	EsdashboardActionButtonProperties[PROP_TARGET]=
		g_param_spec_string("target",
								"Target",
								"The target actor's class name to lookup and to perform action at",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardActionButton:action:
	 *
	 * A string with the signal action name to perform at target.
	 */
	EsdashboardActionButtonProperties[PROP_ACTION]=
		g_param_spec_string("action",
								"Action",
								"The action signal to perform at target",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardActionButtonProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_action_button_init(EsdashboardActionButton *self)
{
	EsdashboardActionButtonPrivate		*priv;

	priv=self->priv=esdashboard_action_button_get_instance_private(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->target=NULL;
	priv->action=FALSE;
	priv->focusManager=esdashboard_focus_manager_get_default();
}

/* IMPLEMENTATION: Public API */

/**
 * esdashboard_action_button_new:
 *
 * Creates a new #EsdashboardActionButton actor
 *
 * Return value: The newly created #EsdashboardActionButton
 */
ClutterActor* esdashboard_action_button_new(void)
{
	return(g_object_new(ESDASHBOARD_TYPE_ACTION_BUTTON, NULL));
}

/**
 * esdashboard_action_button_get_target:
 * @self: A #EsdashboardActionButton
 *
 * Retrieves the target's class name of @self at which the action should be
 * performed.
 *
 * Return value: A string with target's class name
 */
const gchar* esdashboard_action_button_get_target(EsdashboardActionButton *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(self), NULL);

	return(self->priv->target);
}

/**
 * esdashboard_action_button_set_target:
 * @self: A #EsdashboardActionButton
 * @inTarget: The target's class name
 *
 * Sets the target's class name at @self at which the action should be
 * performed by this actor.
 */
void esdashboard_action_button_set_target(EsdashboardActionButton *self, const gchar *inTarget)
{
	EsdashboardActionButtonPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(self));
	g_return_if_fail(inTarget);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->target, inTarget)!=0)
	{
		/* Set value */
		if(priv->target) g_free(priv->target);
		priv->target=g_strdup(inTarget);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardActionButtonProperties[PROP_TARGET]);
	}
}

/**
 * esdashboard_action_button_get_action:
 * @self: A #EsdashboardActionButton
 *
 * Retrieves the action's signal name of @self which will be performed at target.
 *
 * Return value: A string with action's signal name
 */
const gchar* esdashboard_action_button_get_action(EsdashboardActionButton *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(self), NULL);

	return(self->priv->action);
}

/**
 * esdashboard_action_button_set_action:
 * @self: A #EsdashboardActionButton
 * @inAction: The action's signal name
 *
 * Sets the action's signal name at @self which will be performed at target.
 */
void esdashboard_action_button_set_action(EsdashboardActionButton *self, const gchar *inAction)
{
	EsdashboardActionButtonPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_ACTION_BUTTON(self));
	g_return_if_fail(inAction);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->action, inAction)!=0)
	{
		/* Set value */
		if(priv->action) g_free(priv->action);
		priv->action=g_strdup(inAction);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardActionButtonProperties[PROP_ACTION]);
	}
}
