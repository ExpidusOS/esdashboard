/*
 * collapse-box: A collapsable container for one actor
 *               with capability to expand
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

#include <libesdashboard/collapse-box.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/enums.h>
#include <libesdashboard/focus-manager.h>
#include <libesdashboard/animation.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
static void _esdashboard_collapse_box_container_iface_init(ClutterContainerIface *inInterface);

struct _EsdashboardCollapseBoxPrivate
{
	/* Properties related */
	gboolean				isCollapsed;
	gfloat					collapsedSize;
	EsdashboardOrientation	collapseOrientation;
	gfloat					collapseProgress;

	/* Instance related */
	ClutterActor			*child;
	guint					requestModeSignalID;

	EsdashboardFocusManager	*focusManager;
	guint					focusChangedSignalID;

	gboolean				expandedByPointer;
	gboolean				expandedByFocus;
	EsdashboardAnimation	*expandCollapseAnimation;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardCollapseBox,
						esdashboard_collapse_box,
						ESDASHBOARD_TYPE_ACTOR,
						G_ADD_PRIVATE(EsdashboardCollapseBox)
						G_IMPLEMENT_INTERFACE(CLUTTER_TYPE_CONTAINER, _esdashboard_collapse_box_container_iface_init));

/* Properties */
enum
{
	PROP_0,

	PROP_COLLAPSED,
	PROP_COLLAPSED_SIZE,
	PROP_COLLAPSE_ORIENTATION,
	PROP_COLLAPSE_PROGRESS,

	PROP_LAST
};

static GParamSpec* EsdashboardCollapseBoxProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_COLLAPSED_CHANGED,

	SIGNAL_LAST
};

static guint EsdashboardCollapseBoxSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Pointer device left this actor */
static gboolean _esdashboard_collapse_box_on_leave_event(EsdashboardCollapseBox *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	EsdashboardCollapseBoxPrivate	*priv;
	ClutterActor					*related;
	ClutterActor					*stage;
	gfloat							eventX, eventY;

	g_return_val_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Check if new target (related actor) is a direct or deeper child of
	 * this actor. This is an easy and inexpensive check and will likely
	 * succeed if pointer is still inside this actor.
	 */
	related=clutter_event_get_related(inEvent);
	if(clutter_actor_contains(CLUTTER_ACTOR(self), related)) return(CLUTTER_EVENT_PROPAGATE);

	/* There are few cases where above check will fail although pointer
	 * is still inside this actor. So do now a more expensive check
	 * by determining the actor under pointer first and then check if
	 * found actor under pointer is a direct or deeper child of this
	 * actor. This check should be very accurate but more expensive.
	 */
	stage=clutter_actor_get_stage(CLUTTER_ACTOR(self));
	clutter_event_get_coords(inEvent, &eventX, &eventY);
	related=clutter_stage_get_actor_at_pos(CLUTTER_STAGE(stage), CLUTTER_PICK_ALL, eventX, eventY);
	if(related && clutter_actor_contains(CLUTTER_ACTOR(self), related)) return(CLUTTER_EVENT_PROPAGATE);

	/* Pointer is not inside so check to collapse actor to minimum size */
	priv->expandedByPointer=FALSE;
	if(!priv->expandedByPointer && !priv->expandedByFocus)
	{
		esdashboard_collapse_box_set_collapsed(self, TRUE);
	}

	return(CLUTTER_EVENT_PROPAGATE);
}

/* Pointer device entered this actor */
static gboolean _esdashboard_collapse_box_on_enter_event(EsdashboardCollapseBox *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	EsdashboardCollapseBoxPrivate	*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Uncollapse (or better expand) to real size */
	priv->expandedByPointer=TRUE;
	esdashboard_collapse_box_set_collapsed(self, FALSE);
	return(CLUTTER_EVENT_PROPAGATE);
}

/* Request mode of child has changed so we need to apply this new
 * request mode to ourselve for get-preferred-{width,height} functions
 * and allocation function.
 */
static void _esdashboard_collapse_box_on_child_actor_request_mode_changed(EsdashboardCollapseBox *self,
																			GParamSpec *inSpec,
																			gpointer inUserData)
{
	EsdashboardCollapseBoxPrivate	*priv;
	ClutterActor					*child;
	ClutterRequestMode				requestMode;

	g_return_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	priv=self->priv;
	child=CLUTTER_ACTOR(inUserData);

	/* Check if property changed happened at child we remembered */
	g_return_if_fail(child==priv->child);

	/* Apply actor's request mode to us */
	requestMode=clutter_actor_get_request_mode(priv->child);
	clutter_actor_set_request_mode(CLUTTER_ACTOR(self), requestMode);
}

/* Focus has been changed. Check if old and new focusable actor
 * are a (deep) child of this collapse box.
 * If both are children then do nothing because nothing changes
 * and collapse box should be expanded already.
 * If none are children also do nothing and collapse box should
 * be collapsed already.
 * If only one is a child of this collapse box then collapse box
 * if the only child is the actor which lost the focus, otherwise
 * expand collapse box.
 */
static void _esdashboard_collapse_box_on_focus_changed(EsdashboardCollapseBox *self,
														EsdashboardFocusable *inOldActor,
														EsdashboardFocusable *inNewActor,
														gpointer inUserData)
{
	EsdashboardCollapseBoxPrivate	*priv;
	gboolean						oldActorIsChild;
	gboolean						newActorIsChild;

	g_return_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self));
	g_return_if_fail(ESDASHBOARD_IS_FOCUSABLE(inOldActor) || !inOldActor);
	g_return_if_fail(ESDASHBOARD_IS_FOCUSABLE(inNewActor) || !inNewActor);

	priv=self->priv;
	oldActorIsChild=FALSE;
	newActorIsChild=FALSE;

	/* Determine if old and new focusable actor are children of
	 * this collapse box.
	 */
	if(inOldActor)
	{
		oldActorIsChild=clutter_actor_contains(CLUTTER_ACTOR(self),
												CLUTTER_ACTOR(inOldActor));
	}

	if(inNewActor)
	{
		newActorIsChild=clutter_actor_contains(CLUTTER_ACTOR(self),
												CLUTTER_ACTOR(inNewActor));
	}

	/* Do nothing if both actors are children of this collapse box */
	if(oldActorIsChild==newActorIsChild) return;

	/* Collapse box if old actor is the only child of this collapse box ... */
	if(oldActorIsChild)
	{
		/* Check if a pointer is in this collapse box.
		 * If it is do not collapse.
		 */
		priv->expandedByFocus=FALSE;
		if(!priv->expandedByPointer && !priv->expandedByFocus)
		{
			esdashboard_collapse_box_set_collapsed(self, TRUE);
		}
	}
		/* ... otherwise expand box if new actor is the only child. */
		else
		{
			priv->expandedByFocus=TRUE;
			esdashboard_collapse_box_set_collapsed(self, FALSE);
		}
}

/* Collapse or expand animation has stopped and is done */
static void _esdashboard_collapse_box_animation_done(EsdashboardAnimation *inAnimation, gpointer inUserData)
{
	EsdashboardCollapseBox			*self;
	EsdashboardCollapseBoxPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_ANIMATION(inAnimation));
	g_return_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(inUserData));

	self=ESDASHBOARD_COLLAPSE_BOX(inUserData);
	priv=self->priv;

	/* Collapse or expand animation is done, so reset pointer to running animation */
	priv->expandCollapseAnimation=NULL;
}

/* IMPLEMENTATION: Interface ClutterContainer */

/* An actor was added to container */
static void _esdashboard_collapse_box_container_iface_actor_added(ClutterContainer *inContainer,
																	ClutterActor *inActor)
{
	EsdashboardCollapseBox			*self;
	EsdashboardCollapseBoxPrivate	*priv;
	ClutterContainerIface			*interfaceClass;
	ClutterContainerIface			*parentInterfaceClass;

	/* Get object implementing this interface */
	self=ESDASHBOARD_COLLAPSE_BOX(inContainer);
	priv=self->priv;

	/* Get parent interface class */
	interfaceClass=G_TYPE_INSTANCE_GET_INTERFACE(inContainer, CLUTTER_TYPE_CONTAINER, ClutterContainerIface);
	parentInterfaceClass=g_type_interface_peek_parent(interfaceClass);

	/* If it is the first actor added remember it otherwise print warning */
	if(!priv->child)
	{
		/* Remember child */
		priv->child=inActor;
		_esdashboard_collapse_box_on_child_actor_request_mode_changed(self, NULL, priv->child);

		/* Get notified about changed of request-mode of child */
		g_assert(priv->requestModeSignalID==0);
		priv->requestModeSignalID=g_signal_connect_swapped(priv->child,
															"notify::request-mode",
															G_CALLBACK(_esdashboard_collapse_box_on_child_actor_request_mode_changed),
															self);
	}
		else g_warning("More than one actor added to %s - results are unexpected", G_OBJECT_TYPE_NAME(self));

	/* Chain up old function */
	if(parentInterfaceClass->actor_added) parentInterfaceClass->actor_added(inContainer, inActor);
}

/* An actor was removed from container */
static void _esdashboard_collapse_box_container_iface_actor_remove(ClutterContainer *inContainer,
																	ClutterActor *inActor)
{
	EsdashboardCollapseBox			*self;
	EsdashboardCollapseBoxPrivate	*priv;
	ClutterContainerIface			*interfaceClass;
	ClutterContainerIface			*parentInterfaceClass;

	/* Get object implementing this interface */
	self=ESDASHBOARD_COLLAPSE_BOX(inContainer);
	priv=self->priv;

	/* Get parent interface class */
	interfaceClass=G_TYPE_INSTANCE_GET_INTERFACE(inContainer, CLUTTER_TYPE_CONTAINER, ClutterContainerIface);
	parentInterfaceClass=g_type_interface_peek_parent(interfaceClass);

	/* If actor removed is the one we remember
	 * "unremember" it and all information about it
	 */
	if(priv->child==inActor)
	{
		/* Disconnect signal */
		g_signal_handler_disconnect(priv->child, priv->requestModeSignalID);
		priv->requestModeSignalID=0;

		/* Unremember child */
		priv->child=NULL;
	}
	g_assert(priv->requestModeSignalID==0);

	/* Chain up old function */
	if(parentInterfaceClass->actor_removed) parentInterfaceClass->actor_removed(inContainer, inActor);
}

/* Override container interface */
static void _esdashboard_collapse_box_container_iface_init(ClutterContainerIface *inInterface)
{
	inInterface->actor_added=_esdashboard_collapse_box_container_iface_actor_added;
	inInterface->actor_removed=_esdashboard_collapse_box_container_iface_actor_remove;
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _esdashboard_collapse_box_get_preferred_height(ClutterActor *inActor,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	EsdashboardCollapseBox			*self=ESDASHBOARD_COLLAPSE_BOX(inActor);
	EsdashboardCollapseBoxPrivate	*priv=self->priv;
	gfloat							minHeight, naturalHeight;

	minHeight=naturalHeight=0.0f;

	/* Get size of child actor */
	if(priv->child)
	{
		clutter_actor_get_preferred_height(priv->child,
											inForWidth,
											&minHeight,
											&naturalHeight);
	}

	/* If actor is collapsed set minimum size */
	if(priv->collapseOrientation==ESDASHBOARD_ORIENTATION_TOP ||
			priv->collapseOrientation==ESDASHBOARD_ORIENTATION_BOTTOM)
	{
		if(minHeight>priv->collapsedSize)
		{
			minHeight=priv->collapsedSize+((minHeight-priv->collapsedSize)*priv->collapseProgress);
		}

		if(naturalHeight>priv->collapsedSize)
		{
			naturalHeight=priv->collapsedSize+((naturalHeight-priv->collapsedSize)*priv->collapseProgress);
		}
	}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _esdashboard_collapse_box_get_preferred_width(ClutterActor *inActor,
															gfloat inForHeight,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth)
{
	EsdashboardCollapseBox			*self=ESDASHBOARD_COLLAPSE_BOX(inActor);
	EsdashboardCollapseBoxPrivate	*priv=self->priv;
	gfloat							minWidth, naturalWidth;

	minWidth=naturalWidth=0.0f;

	/* Get size of child actor */
	if(priv->child)
	{
		clutter_actor_get_preferred_width(priv->child,
											inForHeight,
											&minWidth,
											&naturalWidth);
	}

	/* If actor is collapsed set minimum size */
	if(priv->collapseOrientation==ESDASHBOARD_ORIENTATION_LEFT ||
			priv->collapseOrientation==ESDASHBOARD_ORIENTATION_RIGHT)
	{
		if(minWidth>priv->collapsedSize)
		{
			minWidth=priv->collapsedSize+((minWidth-priv->collapsedSize)*priv->collapseProgress);
		}

		if(naturalWidth>priv->collapsedSize)
		{
			naturalWidth=priv->collapsedSize+((naturalWidth-priv->collapsedSize)*priv->collapseProgress);
		}
	}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _esdashboard_collapse_box_allocate(ClutterActor *inActor,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	EsdashboardCollapseBox			*self=ESDASHBOARD_COLLAPSE_BOX(inActor);
	EsdashboardCollapseBoxPrivate	*priv=self->priv;
	ClutterRequestMode				requestMode;
	gfloat							w, h;
	gfloat							childWidth, childHeight;
	ClutterActorBox					childBox={ 0, };

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(esdashboard_collapse_box_parent_class)->allocate(inActor, inBox, inFlags);

	/* Get size of this actor's allocation box */
	clutter_actor_box_get_size(inBox, &w, &h);

	/* Get allocation for child */
	if(priv->child && clutter_actor_is_visible(priv->child))
	{
		/* Set up allocation */
		requestMode=clutter_actor_get_request_mode(CLUTTER_ACTOR(self));
		if(requestMode==CLUTTER_REQUEST_WIDTH_FOR_HEIGHT)
		{
			childHeight=h;
			clutter_actor_get_preferred_width(priv->child, childHeight, NULL, &childWidth);
		}
			else
			{
				childWidth=w;
				clutter_actor_get_preferred_height(priv->child, childWidth, NULL, &childHeight);
			}
		clutter_actor_box_set_size(&childBox, childWidth, childHeight);

		clutter_actor_box_set_origin(&childBox, 0.0f, 0.0f);
		if(priv->isCollapsed)
		{
			switch(priv->collapseOrientation)
			{
				case ESDASHBOARD_ORIENTATION_LEFT:
				case ESDASHBOARD_ORIENTATION_TOP:
					/* Do nothing as origin is already set */
					break;

				case ESDASHBOARD_ORIENTATION_RIGHT:
					clutter_actor_box_set_origin(&childBox, -(childWidth-priv->collapsedSize), 0.0f);
					break;

				case ESDASHBOARD_ORIENTATION_BOTTOM:
					clutter_actor_box_set_origin(&childBox, 0.0f, -(childHeight-priv->collapsedSize));
					break;
			}
		}

		/* Set allocation at child */
		clutter_actor_allocate(priv->child, &childBox, inFlags);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_collapse_box_dispose(GObject *inObject)
{
	EsdashboardCollapseBox			*self=ESDASHBOARD_COLLAPSE_BOX(inObject);
	EsdashboardCollapseBoxPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->child)
	{
		if(priv->requestModeSignalID)
		{
			/* Disconnect signal */
			g_signal_handler_disconnect(priv->child, priv->requestModeSignalID);
			priv->requestModeSignalID=0;
		}

		/* Unremember child */
		priv->child=NULL;
	}
	g_assert(priv->requestModeSignalID==0);

	if(priv->focusManager)
	{
		if(priv->focusChangedSignalID)
		{
			/* Disconnect signal */
			g_signal_handler_disconnect(priv->focusManager, priv->focusChangedSignalID);
			priv->focusChangedSignalID=0;
		}

		/* Release reference to focus manager */
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}
	g_assert(priv->focusChangedSignalID==0);

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_collapse_box_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_collapse_box_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardCollapseBox			*self=ESDASHBOARD_COLLAPSE_BOX(inObject);
	
	switch(inPropID)
	{
		case PROP_COLLAPSED:
			esdashboard_collapse_box_set_collapsed(self, g_value_get_boolean(inValue));
			break;

		case PROP_COLLAPSED_SIZE:
			esdashboard_collapse_box_set_collapsed_size(self, g_value_get_float(inValue));
			break;

		case PROP_COLLAPSE_ORIENTATION:
			esdashboard_collapse_box_set_collapse_orientation(self, g_value_get_enum(inValue));
			break;

		case PROP_COLLAPSE_PROGRESS:
			esdashboard_collapse_box_set_collapse_progress(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_collapse_box_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardCollapseBox			*self=ESDASHBOARD_COLLAPSE_BOX(inObject);
	EsdashboardCollapseBoxPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_COLLAPSED:
			g_value_set_boolean(outValue, priv->isCollapsed);
			break;

		case PROP_COLLAPSED_SIZE:
			g_value_set_float(outValue, priv->collapsedSize);
			break;

		case PROP_COLLAPSE_ORIENTATION:
			g_value_set_enum(outValue, priv->collapseOrientation);
			break;

		case PROP_COLLAPSE_PROGRESS:
			g_value_set_float(outValue, priv->collapseProgress);
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
static void esdashboard_collapse_box_class_init(EsdashboardCollapseBoxClass *klass)
{
	EsdashboardActorClass	*actorClass=ESDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	clutterActorClass->get_preferred_width=_esdashboard_collapse_box_get_preferred_width;
	clutterActorClass->get_preferred_height=_esdashboard_collapse_box_get_preferred_height;
	clutterActorClass->allocate=_esdashboard_collapse_box_allocate;

	gobjectClass->set_property=_esdashboard_collapse_box_set_property;
	gobjectClass->get_property=_esdashboard_collapse_box_get_property;
	gobjectClass->dispose=_esdashboard_collapse_box_dispose;

	/* Define properties */
	EsdashboardCollapseBoxProperties[PROP_COLLAPSED]=
		g_param_spec_boolean("collapsed",
								"Collapsed",
								"If TRUE this actor is collapsed otherwise it is expanded to real size",
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardCollapseBoxProperties[PROP_COLLAPSED_SIZE]=
		g_param_spec_float("collapsed-size",
							"Collapsed size",
							"The size of actor when collapsed",
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardCollapseBoxProperties[PROP_COLLAPSE_ORIENTATION]=
		g_param_spec_enum("collapse-orientation",
							"Collapse orientation",
							"Orientation of area being visible in collapsed state",
							ESDASHBOARD_TYPE_ORIENTATION,
							ESDASHBOARD_ORIENTATION_LEFT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardCollapseBoxProperties[PROP_COLLAPSE_PROGRESS]=
		g_param_spec_float("collapse-progress",
							"Collapse progress",
							"The progress fraction used to animate collapse or expand. The fraction must be between 0.0 and 1.0 and "
								"defines the progress between collapsed size (fraction of 0.0) and preferred size (fraction of 1.0).",
							0.0f, 1.0f,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardCollapseBoxProperties);

	/* Define stylable properties */
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardCollapseBoxProperties[PROP_COLLAPSED_SIZE]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardCollapseBoxProperties[PROP_COLLAPSE_ORIENTATION]);

	/* Define signals */
	EsdashboardCollapseBoxSignals[SIGNAL_COLLAPSED_CHANGED]=
		g_signal_new("collapsed-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardCollapseBoxClass, collapse_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__BOOLEAN,
						G_TYPE_NONE,
						1,
						G_TYPE_BOOLEAN);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_collapse_box_init(EsdashboardCollapseBox *self)
{
	EsdashboardCollapseBoxPrivate		*priv;

	priv=self->priv=esdashboard_collapse_box_get_instance_private(self);

	/* Set up default values */
	priv->isCollapsed=TRUE;
	priv->collapsedSize=0.0f;
	priv->collapseOrientation=ESDASHBOARD_ORIENTATION_LEFT;
	priv->collapseProgress=0.0f;
	priv->child=NULL;
	priv->requestModeSignalID=0;
	priv->focusManager=esdashboard_focus_manager_get_default();
	priv->focusChangedSignalID=0;
	priv->expandedByPointer=FALSE;
	priv->expandedByFocus=FALSE;
	priv->expandCollapseAnimation=NULL;

	/* Set up actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_set_clip_to_allocation(CLUTTER_ACTOR(self), TRUE);

	/* Connect signals */
	g_signal_connect_swapped(self, "enter-event", G_CALLBACK(_esdashboard_collapse_box_on_enter_event), self);
	g_signal_connect_swapped(self, "leave-event", G_CALLBACK(_esdashboard_collapse_box_on_leave_event), self);

	if(priv->focusManager)
	{
		priv->focusChangedSignalID=g_signal_connect_swapped(priv->focusManager,
															"changed",
															G_CALLBACK(_esdashboard_collapse_box_on_focus_changed),
															self);
	}
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* esdashboard_collapse_box_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(ESDASHBOARD_TYPE_COLLAPSE_BOX, NULL)));
}

/* Get/set collapse state */
gboolean esdashboard_collapse_box_get_collapsed(EsdashboardCollapseBox *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self), FALSE);

	return(self->priv->isCollapsed);
}

void esdashboard_collapse_box_set_collapsed(EsdashboardCollapseBox *self, gboolean inCollapsed)
{
	EsdashboardCollapseBoxPrivate	*priv;
	EsdashboardAnimation			*animation;
	EsdashboardAnimationValue		**initials;
	EsdashboardAnimationValue		**finals;

	g_return_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self));

	priv=self->priv;

	/* Only set value if it changes */
	if(inCollapsed!=priv->isCollapsed)
	{
		/* Create animation for new expand/collapse state before stopping
		 * any possibly running animation for the old expand/collapse state,
		 * so that the new animation will take over any property value in
		 * the middle of currently running animation. If we stop the animation
		 * beforehand then the new animation would take the initial value in
		 * a state assuming that the current animation has completed.
		 */
		initials=esdashboard_animation_defaults_new(1, "collapse-progress", G_TYPE_FLOAT, inCollapsed ? 1.0 : 0.0);
		finals=esdashboard_animation_defaults_new(1, "collapse-progress", G_TYPE_FLOAT, inCollapsed ? 0.0 : 1.0);
		animation=esdashboard_animation_new_with_values(ESDASHBOARD_ACTOR(self),
														inCollapsed ? "collapse" : "expand",
														initials,
														finals);

		/* Expand/collapse state changed, so stop any running animation */
		if(priv->expandCollapseAnimation)
		{
			g_object_unref(priv->expandCollapseAnimation);
			priv->expandCollapseAnimation=NULL;
		}

		/* Store new animation */
		priv->expandCollapseAnimation=animation;

		/* Set new value */
		priv->isCollapsed=inCollapsed;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardCollapseBoxProperties[PROP_COLLAPSED]);

		/* Emit signal */
		g_signal_emit(self, EsdashboardCollapseBoxSignals[SIGNAL_COLLAPSED_CHANGED], 0, priv->isCollapsed);

		/* Start animation */
		g_signal_connect(priv->expandCollapseAnimation, "animation-done", G_CALLBACK(_esdashboard_collapse_box_animation_done), self);
		esdashboard_animation_run(priv->expandCollapseAnimation);

		/* Free default initial and final values */
		esdashboard_animation_defaults_free(initials);
		esdashboard_animation_defaults_free(finals);
	}
}

/* Get/set size for collapsed state */
gfloat esdashboard_collapse_box_get_collapsed_size(EsdashboardCollapseBox *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self), FALSE);

	return(self->priv->collapsedSize);
}

void esdashboard_collapse_box_set_collapsed_size(EsdashboardCollapseBox *self, gfloat inCollapsedSize)
{
	EsdashboardCollapseBoxPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self));
	g_return_if_fail(inCollapsedSize>=0.0f);

	priv=self->priv;

	/* Only set value if it changes */
	if(inCollapsedSize!=priv->collapsedSize)
	{
		/* Set new value */
		priv->collapsedSize=inCollapsedSize;
		if(priv->isCollapsed) clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardCollapseBoxProperties[PROP_COLLAPSED_SIZE]);
	}
}

/* Get/set orientation for collapsed state */
EsdashboardOrientation esdashboard_collapse_box_get_collapse_orientation(EsdashboardCollapseBox *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self), FALSE);

	return(self->priv->collapseOrientation);
}

void esdashboard_collapse_box_set_collapse_orientation(EsdashboardCollapseBox *self, EsdashboardOrientation inOrientation)
{
	EsdashboardCollapseBoxPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self));
	g_return_if_fail(inOrientation<=ESDASHBOARD_ORIENTATION_BOTTOM);

	priv=self->priv;

	/* Only set value if it changes */
	if(inOrientation!=priv->collapseOrientation)
	{
		/* Set new value */
		priv->collapseOrientation=inOrientation;
		if(priv->isCollapsed) clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardCollapseBoxProperties[PROP_COLLAPSE_ORIENTATION]);
	}
}

/* Get/set size for collapse/expand progress */
gfloat esdashboard_collapse_box_get_collapse_progress(EsdashboardCollapseBox *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self), FALSE);

	return(self->priv->collapseProgress);
}

void esdashboard_collapse_box_set_collapse_progress(EsdashboardCollapseBox *self, gfloat inProgress)
{
	EsdashboardCollapseBoxPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_COLLAPSE_BOX(self));
	g_return_if_fail(inProgress>=0.0f && inProgress<=1.0f);

	priv=self->priv;

	/* Only set value if it changes */
	if(inProgress!=priv->collapseProgress)
	{
		/* Set new value */
		priv->collapseProgress=inProgress;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardCollapseBoxProperties[PROP_COLLAPSE_PROGRESS]);
	}
}
