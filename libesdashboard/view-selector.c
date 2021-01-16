/*
 * view-selector: A selector for registered views
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
 * SECTION:view-selector
 * @short_description: An actor to choose between views
 * @include: esdashboard/view-selector.h
 *
 * This actor displays a #EsdashboardToggleButton for each view added to a
 * #EsdashboardViewpad. It behave like a radio button group where you can choose
 * one among many others. That means when one of them is selected, all other
 * will be deselected. Only one #EsdashboardToggleButton can be active at one time.
 *
 * A #EsdashboardViewSelector is usually created in the layout definition
 * of a theme but it can also be created with esdashboard_view_selector_new()
 * followed by a call to esdashboard_view_selector_set_viewpad() or with
 * esdashboard_view_selector_new_for_viewpad() for short.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/view-selector.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/view.h>
#include <libesdashboard/tooltip-action.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
struct _EsdashboardViewSelectorPrivate
{
	/* Properties related */
	gfloat					spacing;
	EsdashboardViewpad		*viewpad;
	ClutterOrientation		orientation;

	/* Instance related */
	ClutterLayoutManager	*layout;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardViewSelector,
							esdashboard_view_selector,
							ESDASHBOARD_TYPE_ACTOR)

/* Properties */
enum
{
	PROP_0,

	PROP_VIEWPAD,
	PROP_SPACING,
	PROP_ORIENTATION,

	PROP_LAST
};

static GParamSpec* EsdashboardViewSelectorProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_STATE_CHANGED,

	SIGNAL_LAST
};

static guint EsdashboardViewSelectorSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* A view button changed its toggle state */
static void _esdashboard_view_selector_on_toggle_button_state_changed(EsdashboardViewSelector *self, gpointer inUserData)
{
	EsdashboardToggleButton				*button;

	g_return_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	button=ESDASHBOARD_TOGGLE_BUTTON(inUserData);
	g_signal_emit(self, EsdashboardViewSelectorSignals[SIGNAL_STATE_CHANGED], 0, button);
}

/* A view button was clicked to activate it */
static void _esdashboard_view_selector_on_view_button_clicked(EsdashboardViewSelector *self, gpointer inUserData)
{
	EsdashboardViewSelectorPrivate		*priv;
	EsdashboardToggleButton				*button;
	EsdashboardView						*view;

	g_return_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(ESDASHBOARD_IS_BUTTON(inUserData));

	priv=self->priv;
	button=ESDASHBOARD_TOGGLE_BUTTON(inUserData);

	view=ESDASHBOARD_VIEW(g_object_get_data(G_OBJECT(button), "view"));

	esdashboard_viewpad_set_active_view(priv->viewpad, view);
}

/* Called when a view was enabled or will be disabled */
static void _esdashboard_view_selector_on_view_enable_state_changed(EsdashboardView *inView, gpointer inUserData)
{
	ClutterActor						*button;

	g_return_if_fail(ESDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	button=CLUTTER_ACTOR(inUserData);

	if(!esdashboard_view_get_enabled(inView)) clutter_actor_hide(button);
		else clutter_actor_show(button);
}

/* Called when a view was activated or deactivated */
static void _esdashboard_view_selector_on_view_activated(EsdashboardView *inView, gpointer inUserData)
{
	EsdashboardToggleButton				*button;

	g_return_if_fail(ESDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	button=ESDASHBOARD_TOGGLE_BUTTON(inUserData);
	esdashboard_toggle_button_set_toggle_state(button, TRUE);
}

static void _esdashboard_view_selector_on_view_deactivated(EsdashboardView *inView, gpointer inUserData)
{
	EsdashboardToggleButton				*button;

	g_return_if_fail(ESDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	button=ESDASHBOARD_TOGGLE_BUTTON(inUserData);
	esdashboard_toggle_button_set_toggle_state(button, FALSE);
}

/* Called when an icon of a view has changed */
static void _esdashboard_view_selector_on_view_icon_changed(EsdashboardView *inView, ClutterImage *inImage, gpointer inUserData)
{
	EsdashboardButton					*button;

	g_return_if_fail(ESDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(ESDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	button=ESDASHBOARD_BUTTON(inUserData);
	esdashboard_label_set_icon_name(ESDASHBOARD_LABEL(button), esdashboard_view_get_icon(inView));
}

/* Called when the name of a view has changed */
static void _esdashboard_view_selector_on_view_name_changed(EsdashboardView *inView, const gchar *inName, gpointer inUserData)
{
	EsdashboardTooltipAction			*action;

	g_return_if_fail(ESDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(inUserData));

	action=ESDASHBOARD_TOOLTIP_ACTION(inUserData);
	esdashboard_tooltip_action_set_text(action, inName);
}

/* Called when a new view was added to viewpad */
static void _esdashboard_view_selector_on_view_added(EsdashboardViewSelector *self,
														EsdashboardView *inView,
														gpointer inUserData)
{
	EsdashboardViewSelectorPrivate		*priv;
	ClutterActor						*button;
	gchar								*viewName;
	const gchar							*viewIcon;
	gboolean							isActive;
	ClutterAction						*action;

	g_return_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(ESDASHBOARD_IS_VIEW(inView));

	priv=self->priv;

	/* Create button for newly added view */
	viewName=g_markup_printf_escaped("%s", esdashboard_view_get_name(inView));
	viewIcon=esdashboard_view_get_icon(inView);

	button=esdashboard_toggle_button_new_full_with_icon_name(viewIcon, viewName);
	esdashboard_toggle_button_set_auto_toggle(ESDASHBOARD_TOGGLE_BUTTON(button), FALSE);
	g_object_set_data(G_OBJECT(button), "view", inView);
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(_esdashboard_view_selector_on_view_button_clicked), self);

	/* Set toggle state depending of if view is active or not and connect
	 * signal to get notified if toggle state changes to proxy signal
	 */
	g_signal_connect_swapped(button, "toggled", G_CALLBACK(_esdashboard_view_selector_on_toggle_button_state_changed), self);

	isActive=(esdashboard_viewpad_get_active_view(priv->viewpad)==inView);
	esdashboard_toggle_button_set_toggle_state(ESDASHBOARD_TOGGLE_BUTTON(button), isActive);

	/* Add tooltip */
	action=esdashboard_tooltip_action_new();
	esdashboard_tooltip_action_set_text(ESDASHBOARD_TOOLTIP_ACTION(action), viewName);
	clutter_actor_add_action(button, action);

	/* If view is disabled hide button otherwise show and connect signals
	 * to get notified if enabled state has changed
	 */
	if(!esdashboard_view_get_enabled(inView)) clutter_actor_hide(button);
		else clutter_actor_show(button);

	g_signal_connect(inView, "disabled", G_CALLBACK(_esdashboard_view_selector_on_view_enable_state_changed), button);
	g_signal_connect(inView, "enabled", G_CALLBACK(_esdashboard_view_selector_on_view_enable_state_changed), button);
	g_signal_connect(inView, "activated", G_CALLBACK(_esdashboard_view_selector_on_view_activated), button);
	g_signal_connect(inView, "deactivated", G_CALLBACK(_esdashboard_view_selector_on_view_deactivated), button);
	g_signal_connect(inView, "icon-changed", G_CALLBACK(_esdashboard_view_selector_on_view_icon_changed), button);
	g_signal_connect(inView, "name-changed", G_CALLBACK(_esdashboard_view_selector_on_view_name_changed), action);

	/* Add button as child actor */
	clutter_actor_add_child(CLUTTER_ACTOR(self), button);

	/* Release allocated resources */
	g_free(viewName);
}

/* Called when a view was removed to viewpad */
static void _esdashboard_view_selector_on_view_removed(EsdashboardViewSelector *self,
														EsdashboardView *inView,
														gpointer inUserData)
{
	ClutterActorIter					iter;
	ClutterActor						*child;
	gpointer							view;

	g_return_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self));

	/* Iterate through create views and lookup view of given type */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a button otherwise continue iterating */
		if(!ESDASHBOARD_IS_TOGGLE_BUTTON(child)) continue;

		/* If button has reference to view destroy it */
		view=g_object_get_data(G_OBJECT(child), "view");
		if(ESDASHBOARD_IS_VIEW(view) && ESDASHBOARD_VIEW(view)==inView)
		{
			esdashboard_actor_destroy(child);
		}
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_view_selector_dispose(GObject *inObject)
{
	EsdashboardViewSelector			*self=ESDASHBOARD_VIEW_SELECTOR(inObject);
	EsdashboardViewSelectorPrivate	*priv=self->priv;
	ClutterActorIter					iter;
	ClutterActor						*child;
	gpointer							view;

	/* Release allocated resources */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a button otherwise continue iterating */
		if(!ESDASHBOARD_IS_TOGGLE_BUTTON(child)) continue;

		/* If button has reference to a view remove signal handlers it */
		view=g_object_get_data(G_OBJECT(child), "view");
		if(view) g_signal_handlers_disconnect_by_data(view, child);
	}

	if(priv->viewpad)
	{
		g_signal_handlers_disconnect_by_data(priv->viewpad, self);
		g_object_unref(priv->viewpad);
		priv->viewpad=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_view_selector_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_view_selector_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardViewSelector		*self=ESDASHBOARD_VIEW_SELECTOR(inObject);
	
	switch(inPropID)
	{
		case PROP_VIEWPAD:
			esdashboard_view_selector_set_viewpad(self, ESDASHBOARD_VIEWPAD(g_value_get_object(inValue)));
			break;

		case PROP_SPACING:
			esdashboard_view_selector_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_ORIENTATION:
			esdashboard_view_selector_set_orientation(self, g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_view_selector_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardViewSelector		*self=ESDASHBOARD_VIEW_SELECTOR(inObject);

	switch(inPropID)
	{
		case PROP_VIEWPAD:
			g_value_set_object(outValue, self->priv->viewpad);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, self->priv->spacing);
			break;

		case PROP_ORIENTATION:
			g_value_set_enum(outValue, self->priv->orientation);
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
static void esdashboard_view_selector_class_init(EsdashboardViewSelectorClass *klass)
{
	EsdashboardActorClass	*actorClass=ESDASHBOARD_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_esdashboard_view_selector_set_property;
	gobjectClass->get_property=_esdashboard_view_selector_get_property;
	gobjectClass->dispose=_esdashboard_view_selector_dispose;

	/* Define properties */

	/**
	 * EsdashboardViewSelector:viewpad:
	 *
	 * Sets the #EsdashboardViewpad whose views to show
	 */
	EsdashboardViewSelectorProperties[PROP_VIEWPAD]=
		g_param_spec_object("viewpad",
								"Viewpad",
								"The viewpad this selector belongs to",
								ESDASHBOARD_TYPE_VIEWPAD,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardViewSelector:spacing:
	 *
	 * Sets the spacing in pixels between each actor representing a view selection
	 */
	EsdashboardViewSelectorProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							"Spacing",
							"The spacing between views and scrollbars",
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardViewSelector:orientation:
	 *
	 * Sets the orientation of the #EsdashboardViewSelector
	 */
	EsdashboardViewSelectorProperties[PROP_ORIENTATION]=
		g_param_spec_enum("orientation",
							"Orientation",
							"Orientation of view selector",
							CLUTTER_TYPE_ORIENTATION,
							CLUTTER_ORIENTATION_HORIZONTAL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardViewSelectorProperties);

	/* Define stylable properties */
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardViewSelectorProperties[PROP_SPACING]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardViewSelectorProperties[PROP_ORIENTATION]);

	/* Define signals */
 	/**
	 * EsdashboardViewSelector::state-changed:
	 * @self: The object which received the signal
	 * @inButton: The #EsdashboardToggleButton which changed its state
	 *
	 * Should be connected if you wish to perform an action whenever the
	 * #EsdashboardViewSelector has changed the state of @inButton.
	 */
	EsdashboardViewSelectorSignals[SIGNAL_STATE_CHANGED]=
		g_signal_new("state-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardViewSelectorClass, state_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						ESDASHBOARD_TYPE_TOGGLE_BUTTON);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_view_selector_init(EsdashboardViewSelector *self)
{
	EsdashboardViewSelectorPrivate		*priv;

	priv=self->priv=esdashboard_view_selector_get_instance_private(self);

	/* Set up default values */
	priv->viewpad=NULL;
	priv->spacing=0.0f;
	priv->orientation=CLUTTER_ORIENTATION_HORIZONTAL;

	priv->layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(priv->layout), priv->orientation);

	/* Set up actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
}

/* IMPLEMENTATION: Public API */

/**
 * esdashboard_view_selector_new:
 *
 * Creates a new #EsdashboardViewSelector actor
 *
 * Return value: The newly created #EsdashboardViewSelector
 */
ClutterActor* esdashboard_view_selector_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(ESDASHBOARD_TYPE_VIEW_SELECTOR, NULL)));
}

/**
 * esdashboard_view_selector_new_for_viewpad:
 * @inViewpad: The #EsdashboardViewpad whose views to show in this actor
 *
 * Creates a new #EsdashboardViewSelector actor, using @inViewpad as viewpad
 * whose views to show as #EsdashboardToggleButton<!-- -->s.
 *
 * This function is more or less equivalent to calling esdashboard_view_selector_new()
 * and esdashboard_view_selector_set_viewpad().
 *
 * |[<!-- language="C" -->
 *   ClutterActor       *viewSelector;
 *   EsdashboardViewpad *viewpad;
 *
 *   viewpad=ESDASHBOARD_VIEWPAD(...Get viewpad somehow...);
 *
 *   viewSelector=esdashboard_view_selector_new();
 *   esdashboard_view_selector_set_viewpad(ESDASHBOARD_VIEW_SELECTOR(viewSelector), viewpad);
 * ]|
 *
 * Return value: The newly created #EsdashboardViewSelector
 */
ClutterActor* esdashboard_view_selector_new_for_viewpad(EsdashboardViewpad *inViewpad)
{
	return(CLUTTER_ACTOR(g_object_new(ESDASHBOARD_TYPE_VIEW_SELECTOR,
										"viewpad", inViewpad,
										NULL)));
}

/**
 * esdashboard_view_selector_get_viewpad:
 * @self: A #EsdashboardViewSelector
 *
 * Retrieves the #EsdashboardViewpad whose views are shown at @self.
 *
 * Return value: The #EsdashboardViewpad whose views are shown
 */
EsdashboardViewpad* esdashboard_view_selector_get_viewpad(EsdashboardViewSelector *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self), NULL);

	return(self->priv->viewpad);
}

/**
 * esdashboard_view_selector_set_viewpad:
 * @self: A #EsdashboardViewSelector
 * @inViewpad: The #EsdashboardViewpad whose views to show at this actor
 *
 * Sets the #EsdashboardViewpad whose views to show as a choice of views
 * at @self.
 */
void esdashboard_view_selector_set_viewpad(EsdashboardViewSelector *self, EsdashboardViewpad *inViewpad)
{
	EsdashboardViewSelectorPrivate		*priv;
	GList								*views, *entry;

	g_return_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(ESDASHBOARD_IS_VIEWPAD(inViewpad));

	priv=self->priv;

	/* Only set new value if it differs from current value */
	if(priv->viewpad==inViewpad) return;

	/* Release old viewpad if available */
	if(priv->viewpad)
	{
		/* Destroy all children */
		esdashboard_actor_destroy_all_children(CLUTTER_ACTOR(self));

		/* Release old viewpad */
		g_signal_handlers_disconnect_by_data(priv->viewpad, self);
		g_object_unref(priv->viewpad);
		priv->viewpad=NULL;
	}

	/* Set new value */
	priv->viewpad=ESDASHBOARD_VIEWPAD(g_object_ref(inViewpad));
	g_signal_connect_swapped(priv->viewpad, "view-added", G_CALLBACK(_esdashboard_view_selector_on_view_added), self);
	g_signal_connect_swapped(priv->viewpad, "view-removed", G_CALLBACK(_esdashboard_view_selector_on_view_removed), self);

	/* Create instance of each registered view type and add it to this actor
	 * and connect signals
	 */
	views=esdashboard_viewpad_get_views(priv->viewpad);
	for(entry=views; entry; entry=g_list_next(entry))
	{
		_esdashboard_view_selector_on_view_added(self, ESDASHBOARD_VIEW(entry->data), NULL);
	}
	g_list_free(views);

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardViewSelectorProperties[PROP_VIEWPAD]);
}

/**
 * esdashboard_view_selector_get_spacing:
 * @self: A #EsdashboardViewSelector
 *
 * Retrieves the spacing between the actors of @self representing a choice of
 * all views.
 *
 * Return value: The spacing
 */
gfloat esdashboard_view_selector_get_spacing(EsdashboardViewSelector *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self), 0.0f);

	return(self->priv->spacing);
}

/**
 * esdashboard_view_selector_set_spacing:
 * @self: A #EsdashboardViewSelector
 * @inSpacing: The spacing between the actors of @self representing a view.
 *
 * Sets the spacing between the actors representing a view selection.
 */
void esdashboard_view_selector_set_spacing(EsdashboardViewSelector *self, gfloat inSpacing)
{
	EsdashboardViewSelectorPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Only set value if it changes */
	if(inSpacing==priv->spacing) return;

	/* Set new value */
	priv->spacing=inSpacing;
	if(priv->layout) clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), priv->spacing);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardViewSelectorProperties[PROP_SPACING]);
}

/**
 * esdashboard_view_selector_get_orientation:
 * @self: A #EsdashboardViewSelector
 *
 * Retrieves the orientation of the @self.
 *
 * Return value: The orientation of the actor
 */

ClutterOrientation esdashboard_view_selector_get_orientation(EsdashboardViewSelector *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self), CLUTTER_ORIENTATION_HORIZONTAL);

	return(self->priv->orientation);
}

/**
 * esdashboard_view_selector_set_orientation:
 * @self: A #EsdashboardViewSelector
 * @inOrientation: The orientation of #EsdashboardViewSelector
 *
 * Sets the orientation at @self.
 */
void esdashboard_view_selector_set_orientation(EsdashboardViewSelector *self, ClutterOrientation inOrientation)
{
	EsdashboardViewSelectorPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(inOrientation!=CLUTTER_ORIENTATION_HORIZONTAL || inOrientation!=CLUTTER_ORIENTATION_VERTICAL);

	priv=self->priv;

	/* Only set value if it changes */
	if(inOrientation==priv->orientation) return;

	/* Set new value */
	priv->orientation=inOrientation;
	if(priv->layout) clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(priv->layout), priv->orientation);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardViewSelectorProperties[PROP_ORIENTATION]);
}
