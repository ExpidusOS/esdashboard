/*
 * toggle-button: A button which can toggle its state between on and off
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
 * SECTION:toggle-button
 * @short_description: A button which can toggle its state between on and off
 * @include: esdashboard/toggle-button.h
 *
 * #EsdashboardToggleButton is a #EsdashboardButton which will remain in "pressed"
 * state when clicked. This is the "on" state. When it is clicked again it will
 * change its state back to normal state. This is the "off" state.
 *
 * A toggle button is created by calling either esdashboard_toggle_button_new() or
 * any other esdashboard_toggle_button_new_*(). These functions will create a toggle
 * button with state "off".
 *
 * The state of a #EsdashboardToggleButton can be set specifically using
 * esdashboard_toggle_button_set_toggle_state() and retrieved using
 * esdashboard_toggle_button_get_toggle_state().
 *
 * On creation the #EsdashboardToggleButton will be configured to change its state
 * automatically when clicked. This behaviour can be changed using
 * esdashboard_toggle_button_set_auto_toggle() and retrieved using
 * esdashboard_toggle_button_get_auto_toggle().
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/toggle-button.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/stylable.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
struct _EsdashboardToggleButtonPrivate
{
	/* Properties related */
	gboolean		toggleState;
	gboolean		autoToggleOnClick;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardToggleButton,
							esdashboard_toggle_button,
							ESDASHBOARD_TYPE_BUTTON)

/* Properties */
enum
{
	PROP_0,

	PROP_TOGGLE_STATE,
	PROP_AUTO_TOGGLE,

	PROP_LAST
};

static GParamSpec* EsdashboardToggleButtonProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_TOGGLED,

	SIGNAL_LAST
};

static guint EsdashboardToggleButtonSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Toggle button was clicked so toggle state */
static void _esdashboard_toggle_button_clicked(EsdashboardButton *inButton)
{
	EsdashboardToggleButton			*self;
	EsdashboardToggleButtonPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(inButton));

	self=ESDASHBOARD_TOGGLE_BUTTON(inButton);
	priv=self->priv;

	/* Call parent's class default click signal handler */
	if(ESDASHBOARD_BUTTON_CLASS(esdashboard_toggle_button_parent_class)->clicked)
	{
		ESDASHBOARD_BUTTON_CLASS(esdashboard_toggle_button_parent_class)->clicked(inButton);
	}

	/* Set new toggle state if "auto-toggle" (on click) is set */
	if(priv->autoToggleOnClick)
	{
		esdashboard_toggle_button_set_toggle_state(self, !priv->toggleState);
	}
}

/* IMPLEMENTATION: GObject */

/* Set/get properties */
static void _esdashboard_toggle_button_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardToggleButton			*self=ESDASHBOARD_TOGGLE_BUTTON(inObject);
	
	switch(inPropID)
	{
		case PROP_TOGGLE_STATE:
			esdashboard_toggle_button_set_toggle_state(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_toggle_button_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardToggleButton			*self=ESDASHBOARD_TOGGLE_BUTTON(inObject);
	EsdashboardToggleButtonPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_TOGGLE_STATE:
			g_value_set_boolean(outValue, priv->toggleState);
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
static void esdashboard_toggle_button_class_init(EsdashboardToggleButtonClass *klass)
{
	EsdashboardButtonClass			*buttonClass=ESDASHBOARD_BUTTON_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_esdashboard_toggle_button_set_property;
	gobjectClass->get_property=_esdashboard_toggle_button_get_property;

	buttonClass->clicked=_esdashboard_toggle_button_clicked;

	/* Define properties */
	/**
	 * EsdashboardToggleButton:toggle-state:
	 *
	 * A flag indicating if the state of toggle button. It is set to %TRUE if it
	 * is in "on" state that means it is pressed state and %FALSE if it is in
	 * "off" state that means it is not pressed.
	 */
	EsdashboardToggleButtonProperties[PROP_TOGGLE_STATE]=
		g_param_spec_boolean("toggle-state",
								"Toggle state",
								"State of toggle",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardToggleButton:auto-toggle:
	 *
	 * A flag indicating if the state of toggle button should be changed between
	 * "on" and "off" state automatically if it was clicked.
	 */
	EsdashboardToggleButtonProperties[PROP_AUTO_TOGGLE]=
		g_param_spec_boolean("auto-toggle",
								"Auto toggle",
								"If set the toggle state will be toggled on each click",
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardToggleButtonProperties);

	/* Define signals */
	/**
	 * EsdashboardToggleButton::toggled:
	 * @self: The #EsdashboardToggleButton which changed its state
	 *
	 * Should be connected if you wish to perform an action whenever the
	 * #EsdashboardToggleButton's state has changed. 
	 *
	 * The state has to be retrieved using esdashboard_toggle_button_get_toggle_state().
	 */
	EsdashboardToggleButtonSignals[SIGNAL_TOGGLED]=
		g_signal_new("toggled",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardToggleButtonClass, toggled),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_toggle_button_init(EsdashboardToggleButton *self)
{
	EsdashboardToggleButtonPrivate	*priv;

	priv=self->priv=esdashboard_toggle_button_get_instance_private(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->toggleState=FALSE;
	priv->autoToggleOnClick=TRUE;
}

/* IMPLEMENTATION: Public API */

/**
 * esdashboard_toggle_button_new:
 *
 * Creates a new #EsdashboardToggleButton actor
 *
 * Return value: The newly created #EsdashboardToggleButton
 */
ClutterActor* esdashboard_toggle_button_new(void)
{
	return(g_object_new(ESDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", N_(""),
						"label-style", ESDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}

/**
 * esdashboard_toggle_button_new_with_text:
 * @inText: A string containing the text to be placed in the toggle button
 *
 * Creates a new #EsdashboardToggleButton actor with a text label.
 *
 * Return value: The newly created #EsdashboardToggleButton
 */
ClutterActor* esdashboard_toggle_button_new_with_text(const gchar *inText)
{
	return(g_object_new(ESDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", inText,
						"label-style", ESDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}

/**
 * esdashboard_toggle_button_new_with_icon_name:
 * @inIconName: A string containing the stock icon name or file name for the icon
 *   to be place in the toogle button
 *
 * Creates a new #EsdashboardToggleButton actor with an icon.
 *
 * Return value: The newly created #EsdashboardToggleButton
 */
ClutterActor* esdashboard_toggle_button_new_with_icon_name(const gchar *inIconName)
{
	return(g_object_new(ESDASHBOARD_TYPE_TOGGLE_BUTTON,
						"icon-name", inIconName,
						"label-style", ESDASHBOARD_LABEL_STYLE_ICON,
						NULL));
}

/**
 * esdashboard_toggle_button_new_with_gicon:
 * @inIcon: A #GIcon containing the icon image
 *
 * Creates a new #EsdashboardToggleButton actor with an icon.
 *
 * Return value: The newly created #EsdashboardToggleButton
 */
ClutterActor* esdashboard_toggle_button_new_with_gicon(GIcon *inIcon)
{
	return(g_object_new(ESDASHBOARD_TYPE_TOGGLE_BUTTON,
						"icon-gicon", inIcon,
						"label-style", ESDASHBOARD_LABEL_STYLE_ICON,
						NULL));
}

/**
 * esdashboard_toggle_button_new_full_with_icon_name:
 * @inIconName: A string containing the stock icon name or file name for the icon
 *   to be place in the toogle button
 * @inText: A string containing the text to be placed in the toggle button
 *
 * Creates a new #EsdashboardToggleButton actor with a text label and an icon.
 *
 * Return value: The newly created #EsdashboardToggleButton
 */
ClutterActor* esdashboard_toggle_button_new_full_with_icon_name(const gchar *inIconName,
																const gchar *inText)
{
	return(g_object_new(ESDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", inText,
						"icon-name", inIconName,
						"label-style", ESDASHBOARD_LABEL_STYLE_BOTH,
						NULL));
}

/**
 * esdashboard_toggle_button_new_full_with_gicon:
 * @inIcon: A #GIcon containing the icon image
 * @inText: A string containing the text to be placed in the toggle button
 *
 * Creates a new #EsdashboardToggleButton actor with a text label and an icon.
 *
 * Return value: The newly created #EsdashboardToggleButton
 */
ClutterActor* esdashboard_toggle_button_new_full_with_gicon(GIcon *inIcon,
															const gchar *inText)
{
	return(g_object_new(ESDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", inText,
						"icon-gicon", inIcon,
						"label-style", ESDASHBOARD_LABEL_STYLE_BOTH,
						NULL));
}

/**
 * esdashboard_toggle_button_get_toggle_state:
 * @self: A #EsdashboardToggleButton
 *
 * Retrieves the current state of @self.
 *
 * Return value: Returns %TRUE if the toggle button is pressed in ("on" state) and
 *   %FALSE if it is raised ("off" state). 
 */
gboolean esdashboard_toggle_button_get_toggle_state(EsdashboardToggleButton *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(self), 0);

	return(self->priv->toggleState);
}

/**
 * esdashboard_toggle_button_set_toggle_state:
 * @self: A #EsdashboardToggleButton
 * @inToggleState: The state to set at @self
 *
 * Sets the state of @self. If @inToggleState is set to %TRUE then the toggle button
 * will set to and remain in pressed state ("on" state). If set to %FALSE then the
 * toggle button will raised ("off" state).
 */
void esdashboard_toggle_button_set_toggle_state(EsdashboardToggleButton *self, gboolean inToggleState)
{
	EsdashboardToggleButtonPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->toggleState!=inToggleState)
	{
		/* Set value */
		priv->toggleState=inToggleState;

		/* Set style classes */
		if(priv->toggleState) esdashboard_stylable_add_pseudo_class(ESDASHBOARD_STYLABLE(self), "toggled");
			else esdashboard_stylable_remove_pseudo_class(ESDASHBOARD_STYLABLE(self), "toggled");

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardToggleButtonProperties[PROP_TOGGLE_STATE]);

		/* Emit signal for change of toggle state */
		g_signal_emit(self, EsdashboardToggleButtonSignals[SIGNAL_TOGGLED], 0);
	}
}

/**
 * esdashboard_toggle_button_get_auto_toggle:
 * @self: A #EsdashboardToggleButton
 *
 * Retrieves the automatic toggle mode of @self. If automatic toggle mode is %TRUE
 * then it is active and the toggle button changes its state automatically when
 * clicked.
 *
 * Return value: Returns %TRUE if automatic toggle mode is active, otherwise %FALSE.
 */
gboolean esdashboard_toggle_button_get_auto_toggle(EsdashboardToggleButton *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(self), 0);

	return(self->priv->autoToggleOnClick);
}

/**
 * esdashboard_toggle_button_set_auto_toggle:
 * @self: A #EsdashboardToggleButton
 * @inAuto: The state to set at @self
 *
 * Sets the automatic toggle mode of @self. If @inAuto is set to %TRUE then the toggle
 * button will change its state automatically between pressed ("on") and raised ("off")
 * state when it is clicked. The "clicked" signal will be emitted before the toggle
 * changes its state. If @inAuto is set to %FALSE a signal handler for "clicked" signal
 * should be connected to handle the toggle state on your own.
 */
void esdashboard_toggle_button_set_auto_toggle(EsdashboardToggleButton *self, gboolean inAuto)
{
	EsdashboardToggleButtonPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->autoToggleOnClick!=inAuto)
	{
		/* Set value */
		priv->autoToggleOnClick=inAuto;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardToggleButtonProperties[PROP_AUTO_TOGGLE]);
	}
}

/**
 * esdashboard_toggle_button_toggle:
 * @self: A #EsdashboardToggleButton
 *
 * Toggles the state of @self. That means that the toggle button will change its
 * state to pressed ("on" state) if it is currently raised ("off" state) or vice
 * versa.
 */
void esdashboard_toggle_button_toggle(EsdashboardToggleButton *self)
{
	EsdashboardToggleButtonPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(self));

	priv=self->priv;

	/* Set opposite state of current one */
	esdashboard_toggle_button_set_toggle_state(self, !priv->toggleState);
}
