/*
 * fill-box-layout: A box layout expanding actors in one direction
 *                  (fill to fit parent's size) and using natural
 *                  size in other direction
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

#include <libesdashboard/fill-box-layout.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <math.h>

#include <libesdashboard/compat.h>


/* Define this class in GObject system */
struct _EsdashboardFillBoxLayoutPrivate
{
	/* Properties related */
	ClutterOrientation	orientation;
	gfloat				spacing;
	gboolean			isHomogeneous;
	gboolean			keepAspect;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardFillBoxLayout,
							esdashboard_fill_box_layout,
							CLUTTER_TYPE_LAYOUT_MANAGER)

/* Properties */
enum
{
	PROP_0,

	PROP_ORIENTATION,
	PROP_SPACING,
	PROP_HOMOGENEOUS,
	PROP_KEEP_ASPECT,

	PROP_LAST
};

static GParamSpec* EsdashboardFillBoxLayoutProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Get largest minimum and natural size of all visible children
 * for calculation of one child and returns the number of visible ones
 */
static gint _esdashboard_fill_box_layout_get_largest_sizes(EsdashboardFillBoxLayout *self,
															ClutterContainer *inContainer,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	EsdashboardFillBoxLayoutPrivate		*priv;
	ClutterActor						*child;
	ClutterActorIter					iter;
	gint								numberChildren;
	gfloat								largestMinWidth, largestNaturalWidth;
	gfloat								largestMinHeight, largestNaturalHeight;
	gfloat								childMinWidth, childNaturalWidth;
	gfloat								childMinHeight, childNaturalHeight;
	ClutterActor						*parent;
	gfloat								parentWidth, parentHeight;
	gfloat								aspectRatio;

	g_return_val_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self), 0);
	g_return_val_if_fail(CLUTTER_IS_CONTAINER(inContainer), 0);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inContainer), 0);

	priv=self->priv;

	/* Iterate through all children and determine sizes */
	numberChildren=0;
	largestMinWidth=largestNaturalWidth=largestMinHeight=largestNaturalHeight=0.0f;

	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!clutter_actor_is_visible(child)) continue;

		/* Check for largest size */
		clutter_actor_get_preferred_size(child,
											&childMinWidth, &childNaturalWidth,
											&childMinHeight, &childNaturalHeight);

		if(childMinWidth>largestMinWidth) largestMinWidth=childMinWidth;
		if(childNaturalWidth>largestNaturalWidth) largestNaturalWidth=childNaturalWidth;
		if(childMinHeight>largestMinHeight) largestMinHeight=childMinHeight;
		if(childNaturalHeight>largestNaturalHeight) largestNaturalHeight=childNaturalHeight;

		/* Count visible children */
		numberChildren++;
	}

	/* Depending on orientation set sizes to fit into parent actor */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(inContainer));
	if(parent)
	{
		aspectRatio=1.0f;

		clutter_actor_get_size(CLUTTER_ACTOR(parent), &parentWidth, &parentHeight);
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			if(priv->keepAspect==TRUE)
			{
				aspectRatio=largestMinWidth/largestMinHeight;
				largestMinHeight=parentHeight;
				largestMinWidth=largestMinHeight*aspectRatio;

				aspectRatio=largestNaturalWidth/largestNaturalHeight;
				largestNaturalHeight=parentHeight;
				largestNaturalWidth=largestNaturalHeight*aspectRatio;
			}
				else
				{
					largestMinHeight=parentHeight;
					largestNaturalHeight=parentHeight;
				}
		}
			else
			{
				if(priv->keepAspect==TRUE)
				{
					aspectRatio=largestMinHeight/largestMinWidth;
					largestMinWidth=parentWidth;
					largestMinHeight=largestMinWidth*aspectRatio;

					aspectRatio=largestNaturalHeight/largestNaturalWidth;
					largestNaturalWidth=parentWidth;
					largestNaturalHeight=largestNaturalWidth*aspectRatio;
				}
					else
					{
						largestMinWidth=parentWidth;
						largestNaturalWidth=parentWidth;
					}
			}
	}

	/* Set return values */
	if(outMinWidth) *outMinWidth=largestMinWidth;
	if(outNaturalWidth) *outNaturalWidth=largestNaturalWidth;
	if(outMinHeight) *outMinHeight=largestMinHeight;
	if(outNaturalHeight) *outNaturalHeight=largestNaturalHeight;

	/* Return number of visible children */
	return(numberChildren);
}

/* Get minimum and natural size of all visible children */
static void _esdashboard_fill_box_layout_get_sizes_for_all(EsdashboardFillBoxLayout *self,
															ClutterContainer *inContainer,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	EsdashboardFillBoxLayoutPrivate		*priv;
	ClutterActor						*child;
	ClutterActorIter					iter;
	gint								numberChildren;
	gfloat								minWidth, naturalWidth;
	gfloat								minHeight, naturalHeight;
	gfloat								childMinWidth, childNaturalWidth;
	gfloat								childMinHeight, childNaturalHeight;
	ClutterActor						*parent;
	gfloat								parentWidth, parentHeight;
	gfloat								aspectRatio;

	g_return_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));
	g_return_if_fail(CLUTTER_IS_ACTOR(inContainer));

	priv=self->priv;

	/* Initialize return values */
	numberChildren=0;
	minWidth=naturalWidth=minHeight=naturalHeight=0.0f;

	/* If not homogeneous then iterate through all children and determine sizes ... */
	if(priv->isHomogeneous==FALSE)
	{
		/* Iterate through children and calculate sizes */
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only get sizes of visible children */
			if(!clutter_actor_is_visible(child)) continue;

			/* Count visible children */
			numberChildren++;

			/* Determine sizes of visible child */
			clutter_actor_get_preferred_size(child,
												&childMinWidth, &childNaturalWidth,
												&childMinHeight, &childNaturalHeight);

			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				minWidth+=childMinWidth;
				naturalWidth+=childNaturalWidth;
				if(childMinHeight>minHeight) minHeight=childMinHeight;
				if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
			}
				else
				{
					minHeight+=childMinHeight;
					naturalHeight+=childNaturalHeight;
					if(childMinWidth>naturalWidth) minWidth=naturalWidth;
					if(childNaturalWidth>naturalHeight) naturalHeight=childNaturalWidth;
				}
		}
	}
		/* ... otherwise get largest minimum and natural size and add spacing */
		else
		{
			/* Get number of visible children and also largest minimum
			 * and natural size
			 */
			numberChildren=_esdashboard_fill_box_layout_get_largest_sizes(self,
																			inContainer,
																			&childMinWidth, &childNaturalWidth,
																			&childMinHeight, &childNaturalHeight);

			/* Multiply largest sizes with number visible children */
			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				minWidth=(numberChildren*childMinWidth);
				naturalWidth=(numberChildren*childNaturalWidth);

				minHeight=childMinHeight;
				naturalHeight=childNaturalHeight;
			}
				else
				{
					minWidth=childMinWidth;
					naturalWidth=childNaturalWidth;

					minHeight=(numberChildren*childMinHeight);
					naturalHeight=(numberChildren*childNaturalHeight);
				}
		}

	/* Add spacing */
	if(numberChildren>0)
	{
		numberChildren--;
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			minWidth+=numberChildren*priv->spacing;
			naturalWidth+=numberChildren*priv->spacing;
		}
			else
			{
				minHeight+=numberChildren*priv->spacing;
				naturalHeight+=numberChildren*priv->spacing;
			}
	}

	/* Depending on orientation set sizes to fit into parent actor */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(inContainer));
	if(parent)
	{
		aspectRatio=1.0f;

		clutter_actor_get_size(CLUTTER_ACTOR(parent), &parentWidth, &parentHeight);
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			if(priv->keepAspect==TRUE)
			{
				aspectRatio=minWidth/minHeight;
				minHeight=parentHeight;
				minWidth=minHeight*aspectRatio;

				aspectRatio=naturalWidth/naturalHeight;
				naturalHeight=parentHeight;
				naturalWidth=naturalHeight*aspectRatio;
			}
				else
				{
					minHeight=parentHeight;
					naturalHeight=parentHeight;
				}
		}
			else
			{
				if(priv->keepAspect==TRUE)
				{
					aspectRatio=minHeight/minWidth;
					minWidth=parentWidth;
					minHeight=minWidth*aspectRatio;

					aspectRatio=naturalHeight/naturalWidth;
					naturalWidth=parentWidth;
					naturalHeight=naturalWidth*aspectRatio;
				}
					else
					{
						minWidth=parentWidth;
						naturalWidth=parentWidth;
					}
			}
	}

	/* Set return values */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

/* IMPLEMENTATION: ClutterLayoutManager */

/* Get preferred width/height */
static void _esdashboard_fill_box_layout_get_preferred_width(ClutterLayoutManager *inLayoutManager,
																ClutterContainer *inContainer,
																gfloat inForHeight,
																gfloat *outMinWidth,
																gfloat *outNaturalWidth)
{
	EsdashboardFillBoxLayout			*self;
	gfloat								maxMinWidth, maxNaturalWidth;

	g_return_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(inLayoutManager));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	self=ESDASHBOARD_FILL_BOX_LAYOUT(inLayoutManager);

	/* Set up default values */
	maxMinWidth=0.0f;
	maxNaturalWidth=0.0f;

	/* Get sizes */
	_esdashboard_fill_box_layout_get_sizes_for_all(self, inContainer, &maxMinWidth, &maxNaturalWidth, NULL, NULL);

	/* Set return values */
	if(outMinWidth) *outMinWidth=maxMinWidth;
	if(outNaturalWidth) *outNaturalWidth=maxNaturalWidth;
}

static void _esdashboard_fill_box_layout_get_preferred_height(ClutterLayoutManager *inLayoutManager,
																ClutterContainer *inContainer,
																gfloat inForWidth,
																gfloat *outMinHeight,
																gfloat *outNaturalHeight)
{
	EsdashboardFillBoxLayout			*self;
	gfloat								maxMinHeight, maxNaturalHeight;

	g_return_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(inLayoutManager));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	self=ESDASHBOARD_FILL_BOX_LAYOUT(inLayoutManager);

	/* Set up default values */
	maxMinHeight=0.0f;
	maxNaturalHeight=0.0f;

	/* Get sizes */
	_esdashboard_fill_box_layout_get_sizes_for_all(self, inContainer, NULL, NULL, &maxMinHeight, &maxNaturalHeight);

	/* Set return values */
	if(outMinHeight) *outMinHeight=maxMinHeight;
	if(outNaturalHeight) *outNaturalHeight=maxNaturalHeight;
}

/* Re-layout and allocate children of container we manage */
static void _esdashboard_fill_box_layout_allocate(ClutterLayoutManager *inLayoutManager,
													ClutterContainer *inContainer,
													const ClutterActorBox *inAllocation,
													ClutterAllocationFlags inFlags)
{
	EsdashboardFillBoxLayout			*self;
	EsdashboardFillBoxLayoutPrivate		*priv;
	ClutterActor						*child;
	ClutterActorIter					iter;
	gfloat								parentWidth, parentHeight;
	gfloat								homogeneousSize;
	gfloat								x, y, w, h;
	gfloat								aspectRatio;
	ClutterActorBox						childAllocation;

	g_return_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(inLayoutManager));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	self=ESDASHBOARD_FILL_BOX_LAYOUT(inLayoutManager);
	priv=self->priv;

	/* Get dimension of allocation */
	parentWidth=clutter_actor_box_get_width(inAllocation);
	parentHeight=clutter_actor_box_get_height(inAllocation);

	/* If homogeneous determine sizes for all children */
	if(priv->isHomogeneous==TRUE)
	{
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			_esdashboard_fill_box_layout_get_largest_sizes(self, inContainer, NULL, &homogeneousSize, NULL, NULL);
		}
			else
			{
				_esdashboard_fill_box_layout_get_largest_sizes(self, inContainer, NULL, NULL, NULL, &homogeneousSize);
			}
	}

	/* Iterate through children and set their sizes */
	x=y=0.0f;

	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		gboolean						fixedPosition;
		gfloat							fixedX, fixedY;

		/* Only set sizes on visible children */
		if(!clutter_actor_is_visible(child)) continue;

		/* Calculate and set new allocation of child */
		if(priv->isHomogeneous==FALSE)
		{
			clutter_actor_get_size(CLUTTER_ACTOR(inContainer), &w, &h);
			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				aspectRatio=w/h;
				h=parentHeight;
				w=h*aspectRatio;
			}
				else
				{
					aspectRatio=h/w;
					w=parentWidth;
					h=w*aspectRatio;
				}
		}
			else
			{
				if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
				{
					w=homogeneousSize;
					h=parentHeight;
				}
					else
					{
						w=parentWidth;
						h=homogeneousSize;
					}
			}

		/* Set new allocation of child but respect fixed position of actor */
		g_object_get(child,
						"fixed-position-set", &fixedPosition,
						"fixed-x", &fixedX,
						"fixed-y", &fixedY,
						NULL);

		if(fixedPosition)
		{
			childAllocation.x1=ceil(fixedX);
			childAllocation.y1=ceil(fixedY);
		}
			else
			{
				childAllocation.x1=ceil(x);
				childAllocation.y1=ceil(y);
			}

		childAllocation.x2=ceil(childAllocation.x1+w);
		childAllocation.y2=ceil(childAllocation.y1+h);
		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) x+=(w+priv->spacing);
			else y+=(h+priv->spacing);
	}
}

/* IMPLEMENTATION: GObject */

/* Set/get properties */
static void _esdashboard_fill_box_layout_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	EsdashboardFillBoxLayout			*self=ESDASHBOARD_FILL_BOX_LAYOUT(inObject);
	
	switch(inPropID)
	{
		case PROP_ORIENTATION:
			esdashboard_fill_box_layout_set_orientation(self, g_value_get_enum(inValue));
			break;

		case PROP_SPACING:
			esdashboard_fill_box_layout_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_HOMOGENEOUS:
			esdashboard_fill_box_layout_set_homogeneous(self, g_value_get_boolean(inValue));
			break;

		case PROP_KEEP_ASPECT:
			esdashboard_fill_box_layout_set_keep_aspect(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_fill_box_layout_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	EsdashboardFillBoxLayout	*self=ESDASHBOARD_FILL_BOX_LAYOUT(inObject);

	switch(inPropID)
	{
		case PROP_ORIENTATION:
			g_value_set_enum(outValue, self->priv->orientation);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, self->priv->spacing);
			break;

		case PROP_HOMOGENEOUS:
			g_value_set_boolean(outValue, self->priv->isHomogeneous);
			break;

		case PROP_KEEP_ASPECT:
			g_value_set_boolean(outValue, self->priv->keepAspect);
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
static void esdashboard_fill_box_layout_class_init(EsdashboardFillBoxLayoutClass *klass)
{
	ClutterLayoutManagerClass	*layoutClass=CLUTTER_LAYOUT_MANAGER_CLASS(klass);
	GObjectClass				*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	layoutClass->get_preferred_width=_esdashboard_fill_box_layout_get_preferred_width;
	layoutClass->get_preferred_height=_esdashboard_fill_box_layout_get_preferred_height;
	layoutClass->allocate=_esdashboard_fill_box_layout_allocate;

	gobjectClass->set_property=_esdashboard_fill_box_layout_set_property;
	gobjectClass->get_property=_esdashboard_fill_box_layout_get_property;

	/* Define properties */
	EsdashboardFillBoxLayoutProperties[PROP_ORIENTATION]=
		g_param_spec_enum("orientation",
							"Orientation",
							"The orientation to layout children",
							CLUTTER_TYPE_ORIENTATION,
							CLUTTER_ORIENTATION_HORIZONTAL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardFillBoxLayoutProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
								"spacing",
								"The spacing between children",
								0.0f,
								G_MAXFLOAT,
								0.0f,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardFillBoxLayoutProperties[PROP_HOMOGENEOUS]=
		g_param_spec_boolean("homogeneous",
								"Homogeneous",
								"Whether the layout should be homogeneous, i.e. all children get the same size",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	EsdashboardFillBoxLayoutProperties[PROP_KEEP_ASPECT]=
		g_param_spec_boolean("keep-aspect",
								"Keep aspect",
								"Whether all children should keep their aspect",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardFillBoxLayoutProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_fill_box_layout_init(EsdashboardFillBoxLayout *self)
{
	EsdashboardFillBoxLayoutPrivate	*priv;

	priv=self->priv=esdashboard_fill_box_layout_get_instance_private(self);

	/* Set default values */
	priv->orientation=CLUTTER_ORIENTATION_HORIZONTAL;
	priv->spacing=0.0f;
	priv->isHomogeneous=FALSE;
	priv->keepAspect=FALSE;
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterLayoutManager* esdashboard_fill_box_layout_new(void)
{
	return(CLUTTER_LAYOUT_MANAGER(g_object_new(ESDASHBOARD_TYPE_FILL_BOX_LAYOUT, NULL)));
}

ClutterLayoutManager* esdashboard_fill_box_layout_new_with_orientation(ClutterOrientation inOrientation)
{
	return(CLUTTER_LAYOUT_MANAGER(g_object_new(ESDASHBOARD_TYPE_FILL_BOX_LAYOUT,
												"orientation", inOrientation,
												NULL)));
}

/* Get/set orientation */
ClutterOrientation esdashboard_fill_box_layout_get_orientation(EsdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self), CLUTTER_ORIENTATION_HORIZONTAL);

	return(self->priv->orientation);
}

void esdashboard_fill_box_layout_set_orientation(EsdashboardFillBoxLayout *self, ClutterOrientation inOrientation)
{
	EsdashboardFillBoxLayoutPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self));
	g_return_if_fail(inOrientation==CLUTTER_ORIENTATION_HORIZONTAL ||
						inOrientation==CLUTTER_ORIENTATION_VERTICAL);

	priv=self->priv;

	/* Set new values if changed */
	if(priv->orientation!=inOrientation)
	{
		/* Set new values and notify about properties changes */
		priv->orientation=inOrientation;
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardFillBoxLayoutProperties[PROP_ORIENTATION]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set spacing */
gfloat esdashboard_fill_box_layout_get_spacing(EsdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self), 0.0f);

	return(self->priv->spacing);
}

void esdashboard_fill_box_layout_set_spacing(EsdashboardFillBoxLayout *self, gfloat inSpacing)
{
	EsdashboardFillBoxLayoutPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set new values if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set new values and notify about properties changes */
		priv->spacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardFillBoxLayoutProperties[PROP_SPACING]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set homogenous */
gboolean esdashboard_fill_box_layout_get_homogeneous(EsdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self), FALSE);

	return(self->priv->isHomogeneous);
}

void esdashboard_fill_box_layout_set_homogeneous(EsdashboardFillBoxLayout *self, gboolean inIsHomogeneous)
{
	EsdashboardFillBoxLayoutPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self));

	priv=self->priv;

	/* Set new values if changed */
	if(priv->isHomogeneous!=inIsHomogeneous)
	{
		/* Set new values and notify about properties changes */
		priv->isHomogeneous=inIsHomogeneous;
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardFillBoxLayoutProperties[PROP_HOMOGENEOUS]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set keep aspect ratio */
gboolean esdashboard_fill_box_layout_get_keep_aspect(EsdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self), FALSE);

	return(self->priv->keepAspect);
}

void esdashboard_fill_box_layout_set_keep_aspect(EsdashboardFillBoxLayout *self, gboolean inKeepAspect)
{
	EsdashboardFillBoxLayoutPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_FILL_BOX_LAYOUT(self));

	priv=self->priv;

	/* Set new values if changed */
	if(priv->keepAspect!=inKeepAspect)
	{
		/* Set new values and notify about properties changes */
		priv->keepAspect=inKeepAspect;
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardFillBoxLayoutProperties[PROP_KEEP_ASPECT]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}
