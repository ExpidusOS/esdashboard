/*
 * binding: A keyboard or pointer binding
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

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>

#include <libesdashboard/binding.h>
#include <libesdashboard/enums.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
struct _EsdashboardBindingPrivate
{
	/* Instance related */
	ClutterEventType		eventType;
	gchar					*className;
	guint					key;
	ClutterModifierType		modifiers;
	gchar					*target;
	gchar					*action;
	EsdashboardBindingFlags	flags;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardBinding,
							esdashboard_binding,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_EVENT_TYPE,
	PROP_CLASS_NAME,
	PROP_KEY,
	PROP_MODIFIERS,
	PROP_TARGET,
	PROP_ACTION,
	PROP_FLAGS,

	PROP_LAST
};

static GParamSpec* EsdashboardBindingProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_binding_dispose(GObject *inObject)
{
	/* Release allocated variables */
	EsdashboardBinding			*self=ESDASHBOARD_BINDING(inObject);
	EsdashboardBindingPrivate	*priv=self->priv;

	if(priv->className)
	{
		g_free(priv->className);
		priv->className=NULL;
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
	G_OBJECT_CLASS(esdashboard_binding_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_binding_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	EsdashboardBinding			*self=ESDASHBOARD_BINDING(inObject);

	switch(inPropID)
	{
		case PROP_EVENT_TYPE:
			esdashboard_binding_set_event_type(self, g_value_get_enum(inValue));
			break;

		case PROP_CLASS_NAME:
			esdashboard_binding_set_class_name(self, g_value_get_string(inValue));
			break;

		case PROP_KEY:
			esdashboard_binding_set_key(self, g_value_get_uint(inValue));
			break;

		case PROP_MODIFIERS:
			esdashboard_binding_set_modifiers(self, g_value_get_flags(inValue));
			break;

		case PROP_TARGET:
			esdashboard_binding_set_target(self, g_value_get_string(inValue));
			break;

		case PROP_ACTION:
			esdashboard_binding_set_action(self, g_value_get_string(inValue));
			break;

		case PROP_FLAGS:
			esdashboard_binding_set_flags(self, g_value_get_flags(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_binding_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	EsdashboardBinding			*self=ESDASHBOARD_BINDING(inObject);
	EsdashboardBindingPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_EVENT_TYPE:
			g_value_set_enum(outValue, priv->eventType);
			break;

		case PROP_CLASS_NAME:
			g_value_set_string(outValue, priv->className);
			break;

		case PROP_KEY:
			g_value_set_uint(outValue, priv->key);
			break;

		case PROP_MODIFIERS:
			g_value_set_flags(outValue, priv->modifiers);
			break;

		case PROP_TARGET:
			g_value_set_string(outValue, priv->target);
			break;

		case PROP_ACTION:
			g_value_set_string(outValue, priv->action);
			break;

		case PROP_FLAGS:
			g_value_set_flags(outValue, priv->flags);
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
static void esdashboard_binding_class_init(EsdashboardBindingClass *klass)
{
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_binding_dispose;
	gobjectClass->set_property=_esdashboard_binding_set_property;
	gobjectClass->get_property=_esdashboard_binding_get_property;

	/* Define properties */
	EsdashboardBindingProperties[PROP_EVENT_TYPE]=
		g_param_spec_enum("event-type",
							"Event type",
							"The event type this binding is bound to",
							CLUTTER_TYPE_EVENT_TYPE,
							CLUTTER_NOTHING,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardBindingProperties[PROP_CLASS_NAME]=
		g_param_spec_string("class-name",
								"Class name",
								"Class name of object this binding is bound to",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardBindingProperties[PROP_KEY]=
		g_param_spec_uint("key",
							"Key",
							"Key code of a keyboard event this binding is bound to",
							0, G_MAXUINT,
							0,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardBindingProperties[PROP_MODIFIERS]=
		g_param_spec_flags("modifiers",
							"Modifiers",
							"Modifiers this binding is bound to",
							CLUTTER_TYPE_MODIFIER_TYPE,
							0,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardBindingProperties[PROP_TARGET]=
		g_param_spec_string("target",
								"Target",
								"Class name of target of this binding",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardBindingProperties[PROP_ACTION]=
		g_param_spec_string("action",
								"Action",
								"Action assigned to this binding",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardBindingProperties[PROP_FLAGS]=
		g_param_spec_flags("flags",
								"Flags",
								"Flags assigned to this binding",
								ESDASHBOARD_TYPE_BINDING_FLAGS,
								0,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardBindingProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_binding_init(EsdashboardBinding *self)
{
	EsdashboardBindingPrivate	*priv;

	priv=self->priv=esdashboard_binding_get_instance_private(self);

	/* Set up default values */
	priv->eventType=CLUTTER_NOTHING;
	priv->className=NULL;
	priv->key=0;
	priv->modifiers=0;
	priv->target=NULL;
	priv->action=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
EsdashboardBinding* esdashboard_binding_new(void)
{
	return(ESDASHBOARD_BINDING(g_object_new(ESDASHBOARD_TYPE_BINDING, NULL)));
}

EsdashboardBinding* esdashboard_binding_new_for_event(const ClutterEvent *inEvent)
{
	EsdashboardBinding		*binding;
	ClutterEventType		eventType;

	g_return_val_if_fail(inEvent, NULL);

	/* Create instance */
	binding=ESDASHBOARD_BINDING(g_object_new(ESDASHBOARD_TYPE_BINDING, NULL));
	if(!binding)
	{
		g_warning("Failed to create binding instance");
		return(NULL);
	}

	/* Set up instance */
	eventType=clutter_event_type(inEvent);
	switch(eventType)
	{
		case CLUTTER_KEY_PRESS:
			esdashboard_binding_set_event_type(binding, eventType);
			esdashboard_binding_set_key(binding, ((ClutterKeyEvent*)inEvent)->keyval);
			esdashboard_binding_set_modifiers(binding, ((ClutterKeyEvent*)inEvent)->modifier_state);
			break;

		case CLUTTER_KEY_RELEASE:
			/* We assume that a key event with a key value and a modifier state but no unicode
			 * value is the release of a single key which is a modifier. In this case do not
			 * set the modifier state in this binding which is created for this event.
			 * This means: Only set modifier state in this binding if key valule, modifier state
			 * and a unicode value is set.
			 */
			esdashboard_binding_set_event_type(binding, eventType);
			esdashboard_binding_set_key(binding, ((ClutterKeyEvent*)inEvent)->keyval);
			if(((ClutterKeyEvent*)inEvent)->keyval &&
				((ClutterKeyEvent*)inEvent)->modifier_state &&
				((ClutterKeyEvent*)inEvent)->unicode_value)
			{
				esdashboard_binding_set_modifiers(binding, ((ClutterKeyEvent*)inEvent)->modifier_state);
			}
			break;

		default:
			ESDASHBOARD_DEBUG(binding, MISC,
								"Cannot create binding instance for unsupported or invalid event type %d",
								eventType);

			/* Release allocated resources */
			g_object_unref(binding);

			return(NULL);
	}

	/* Return create binding instance */
	return(binding);
}

/* Get hash value for binding */
guint esdashboard_binding_hash(gconstpointer inValue)
{
	EsdashboardBindingPrivate		*priv;
	guint							hash;

	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(inValue), 0);

	priv=ESDASHBOARD_BINDING(inValue)->priv;
	hash=0;

	/* Create hash value */
	if(priv->className) hash=g_str_hash(priv->className);

	switch(priv->eventType)
	{
		case CLUTTER_KEY_PRESS:
		case CLUTTER_KEY_RELEASE:
			hash^=priv->key;
			hash^=priv->modifiers;
			break;

		default:
			g_assert_not_reached();
			break;
	}

	return(hash);
}

/* Check if two bindings are equal */
gboolean esdashboard_binding_compare(gconstpointer inLeft, gconstpointer inRight)
{
	EsdashboardBindingPrivate		*leftPriv;
	EsdashboardBindingPrivate		*rightPriv;

	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(inLeft), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(inRight), FALSE);

	leftPriv=ESDASHBOARD_BINDING(inLeft)->priv;
	rightPriv=ESDASHBOARD_BINDING(inRight)->priv;

	/* Check if event type of bindings are equal */
	if(leftPriv->eventType!=rightPriv->eventType) return(FALSE);

	/* Check if class of bindings are equal */
	if(g_strcmp0(leftPriv->className, rightPriv->className)) return(FALSE);

	/* Check if other values of bindings are equal - depending on their type */
	switch(leftPriv->eventType)
	{
		case CLUTTER_KEY_PRESS:
		case CLUTTER_KEY_RELEASE:
			if(leftPriv->key!=rightPriv->key ||
				leftPriv->modifiers!=rightPriv->modifiers)
			{
				return(FALSE);
			}
			break;

		default:
			/* We should never get here but if we do return FALSE
			 * to indicate that both binding are not equal.
			 */
			g_assert_not_reached();
			return(FALSE);
	}

	/* If we get here all check succeeded so return TRUE */
	return(TRUE);
}

/* Get/set event type of binding */
ClutterEventType esdashboard_binding_get_event_type(const EsdashboardBinding *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(self), CLUTTER_NOTHING);

	return(self->priv->eventType);
}

void esdashboard_binding_set_event_type(EsdashboardBinding *self, ClutterEventType inType)
{
	EsdashboardBindingPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_BINDING(self));

	priv=self->priv;

	/* Only key or pointer events can be handled by binding */
	if(inType!=CLUTTER_KEY_PRESS &&
		inType!=CLUTTER_KEY_RELEASE)
	{
		GEnumClass				*eventEnumClass;
		GEnumValue				*eventEnumValue;

		eventEnumClass=g_type_class_ref(CLUTTER_TYPE_EVENT);

		eventEnumValue=g_enum_get_value(eventEnumClass, inType);
		if(eventEnumValue)
		{
			g_warning("Cannot set unsupported event type %s at binding",
						eventEnumValue->value_name);
		}
			else
			{
				g_warning("Cannot set invalid event type at binding");
			}

		g_type_class_unref(eventEnumClass);

		return;
	}

	/* Set value if changed */
	if(priv->eventType!=inType)
	{
		/* Set value */
		priv->eventType=inType;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardBindingProperties[PROP_EVENT_TYPE]);
	}
}

/* Get/set class name of binding */
const gchar* esdashboard_binding_get_class_name(const EsdashboardBinding *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(self), NULL);

	return(self->priv->className);
}

void esdashboard_binding_set_class_name(EsdashboardBinding *self, const gchar *inClassName)
{
	EsdashboardBindingPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inClassName && *inClassName);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->className, inClassName)!=0)
	{
		/* Set value */
		if(priv->className)
		{
			g_free(priv->className);
			priv->className=NULL;
		}

		if(inClassName) priv->className=g_strdup(inClassName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardBindingProperties[PROP_CLASS_NAME]);
	}
}

/* Get/set key code of binding */
guint esdashboard_binding_get_key(const EsdashboardBinding *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(self), 0);

	return(self->priv->key);
}

void esdashboard_binding_set_key(EsdashboardBinding *self, guint inKey)
{
	EsdashboardBindingPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inKey>0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->key!=inKey)
	{
		/* Set value */
		priv->key=inKey;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardBindingProperties[PROP_KEY]);
	}
}

/* Get/set modifiers of binding */
ClutterModifierType esdashboard_binding_get_modifiers(const EsdashboardBinding *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(self), 0);

	return(self->priv->modifiers);
}

void esdashboard_binding_set_modifiers(EsdashboardBinding *self, ClutterModifierType inModifiers)
{
	EsdashboardBindingPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_BINDING(self));

	priv=self->priv;

	/* Reduce modifiers to supported ones */
	inModifiers=inModifiers & ESDASHBOARD_BINDING_MODIFIERS_MASK;

	/* Set value if changed */
	if(priv->modifiers!=inModifiers)
	{
		/* Set value */
		priv->modifiers=inModifiers;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardBindingProperties[PROP_MODIFIERS]);
	}
}

/* Get/set target of binding */
const gchar* esdashboard_binding_get_target(const EsdashboardBinding *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(self), NULL);

	return(self->priv->target);
}

void esdashboard_binding_set_target(EsdashboardBinding *self, const gchar *inTarget)
{
	EsdashboardBindingPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inTarget && *inTarget);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->target, inTarget)!=0)
	{
		/* Set value */
		if(priv->target)
		{
			g_free(priv->target);
			priv->target=NULL;
		}

		if(inTarget) priv->target=g_strdup(inTarget);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardBindingProperties[PROP_TARGET]);
	}
}

/* Get/set action of binding */
const gchar* esdashboard_binding_get_action(const EsdashboardBinding *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(self), NULL);

	return(self->priv->action);
}

void esdashboard_binding_set_action(EsdashboardBinding *self, const gchar *inAction)
{
	EsdashboardBindingPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inAction && *inAction);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->action, inAction)!=0)
	{
		/* Set value */
		if(priv->action)
		{
			g_free(priv->action);
			priv->action=NULL;
		}

		if(inAction) priv->action=g_strdup(inAction);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardBindingProperties[PROP_ACTION]);
	}
}

/* Get/set flags of binding */
EsdashboardBindingFlags esdashboard_binding_get_flags(const EsdashboardBinding *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_BINDING(self), 0);

	return(self->priv->flags);
}

void esdashboard_binding_set_flags(EsdashboardBinding *self, EsdashboardBindingFlags inFlags)
{
	EsdashboardBindingPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inFlags<=ESDASHBOARD_BINDING_FLAGS_ALLOW_UNFOCUSABLE_TARGET);

	priv=self->priv;

	/* Set value if changed */
	if(priv->flags!=inFlags)
	{
		/* Set value */
		priv->flags=inFlags;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardBindingProperties[PROP_FLAGS]);
	}
}
