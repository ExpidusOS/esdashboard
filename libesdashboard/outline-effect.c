/*
 * outline-effect: Draws an outline on top of actor
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

#include <libesdashboard/outline-effect.h>

#include <glib/gi18n-lib.h>
#include <math.h>

#include <libesdashboard/enums.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
struct _EsdashboardOutlineEffectPrivate
{
	/* Properties related */
	ClutterColor				*color;
	gfloat						width;
	EsdashboardBorders			borders;
	EsdashboardCorners			corners;
	gfloat						cornersRadius;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardOutlineEffect,
							esdashboard_outline_effect,
							CLUTTER_TYPE_EFFECT)

/* Properties */
enum
{
	PROP_0,

	PROP_COLOR,
	PROP_WIDTH,
	PROP_BORDERS,
	PROP_CORNERS,
	PROP_CORNERS_RADIUS,

	PROP_LAST
};

static GParamSpec* EsdashboardOutlineEffectProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Draw effect after actor was drawn */
static void _esdashboard_outline_effect_paint(ClutterEffect *inEffect, ClutterEffectPaintFlags inFlags)
{
	EsdashboardOutlineEffect			*self;
	EsdashboardOutlineEffectPrivate		*priv;
	ClutterActor						*target;
	gfloat								width, height;
	gfloat								lineWidth;

	g_return_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(inEffect));

	self=ESDASHBOARD_OUTLINE_EFFECT(inEffect);
	priv=self->priv;

	/* Chain to the next item in the paint sequence */
	target=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
	clutter_actor_continue_paint(target);

	/* Get size of outline to draw */
	clutter_actor_get_size(target, &width, &height);

	/* Round line width for better looking and check if we can draw
	 * outline with configured line width. That means it needs to be
	 * greater or equal to 1.0
	 */
	lineWidth=floor(priv->width+0.5);
	if(lineWidth<1.0f) return;

	/* Draw outline */
	if(priv->color)
	{
		cogl_set_source_color4ub(priv->color->red,
									priv->color->green,
									priv->color->blue,
									priv->color->alpha);
	}

	if(priv->cornersRadius>0.0f)
	{
		gfloat							outerRadius, innerRadius;

		/* Determine radius for rounded corners of outer and inner lines */
		outerRadius=MIN(priv->cornersRadius+(lineWidth/2.0f), width/2.0f);
		outerRadius=MIN(outerRadius, width/2.0f);
		outerRadius=MAX(outerRadius, 0.0f);

		innerRadius=MIN(priv->cornersRadius-(lineWidth/2.0f), width/2.0f);
		innerRadius=MIN(innerRadius, width/2.0f);
		innerRadius=MAX(innerRadius, 0.0f);

		/* Top-left corner */
		if(priv->corners & ESDASHBOARD_CORNERS_TOP_LEFT &&
			priv->borders & ESDASHBOARD_BORDERS_LEFT &&
			priv->borders & ESDASHBOARD_BORDERS_TOP)
		{
			cogl_path_new();
			cogl_path_move_to(0, outerRadius);
			cogl_path_arc(outerRadius, outerRadius, outerRadius, outerRadius, 180, 270);
			cogl_path_line_to(outerRadius, outerRadius-innerRadius);
			cogl_path_arc(outerRadius, outerRadius, innerRadius, innerRadius, 270, 180);
			cogl_path_line_to(0, outerRadius);
			cogl_path_fill_preserve();
			cogl_path_close();
		}

		/* Top border */
		if(priv->borders & ESDASHBOARD_BORDERS_TOP)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & ESDASHBOARD_CORNERS_TOP_LEFT) offset1=outerRadius;
			if(priv->corners & ESDASHBOARD_CORNERS_TOP_RIGHT) offset2=outerRadius;

			cogl_path_new();
			cogl_path_move_to(offset1, 0);
			cogl_path_line_to(width-offset2, 0);
			cogl_path_line_to(width-offset2, outerRadius-innerRadius);
			cogl_path_line_to(offset1, outerRadius-innerRadius);
			cogl_path_line_to(offset1, 0);
			cogl_path_fill_preserve();
			cogl_path_close();
		}

		/* Top-right corner */
		if(priv->corners & ESDASHBOARD_CORNERS_TOP_RIGHT &&
			priv->borders & ESDASHBOARD_BORDERS_TOP &&
			priv->borders & ESDASHBOARD_BORDERS_RIGHT)
		{
			cogl_path_new();
			cogl_path_move_to(width-outerRadius, 0);
			cogl_path_arc(width-outerRadius, outerRadius, outerRadius, outerRadius, 270, 360);
			cogl_path_line_to(width-outerRadius+innerRadius, outerRadius);
			cogl_path_arc(width-outerRadius, outerRadius, innerRadius, innerRadius, 360, 270);
			cogl_path_line_to(width-outerRadius, 0);
			cogl_path_fill_preserve();
			cogl_path_close();
		}

		/* Right border */
		if(priv->borders & ESDASHBOARD_BORDERS_RIGHT)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & ESDASHBOARD_CORNERS_TOP_RIGHT) offset1=outerRadius;
			if(priv->corners & ESDASHBOARD_CORNERS_BOTTOM_RIGHT) offset2=outerRadius;

			cogl_path_new();
			cogl_path_move_to(width, offset1);
			cogl_path_line_to(width, height-offset2);
			cogl_path_line_to(width-outerRadius+innerRadius, height-offset2);
			cogl_path_line_to(width-outerRadius+innerRadius, offset1);
			cogl_path_line_to(width, offset1);
			cogl_path_fill_preserve();
			cogl_path_close();
		}

		/* Bottom-right corner */
		if(priv->corners & ESDASHBOARD_CORNERS_BOTTOM_RIGHT &&
			priv->borders & ESDASHBOARD_BORDERS_RIGHT &&
			priv->borders & ESDASHBOARD_BORDERS_BOTTOM)
		{
			cogl_path_new();
			cogl_path_move_to(width, height-outerRadius);
			cogl_path_arc(width-outerRadius, height-outerRadius, outerRadius, outerRadius, 0, 90);
			cogl_path_line_to(width-outerRadius, height-outerRadius+innerRadius);
			cogl_path_arc(width-outerRadius, height-outerRadius, innerRadius, innerRadius, 90, 0);
			cogl_path_line_to(width, height-outerRadius);
			cogl_path_fill_preserve();
			cogl_path_close();
		}

		/* Bottom border */
		if(priv->borders & ESDASHBOARD_BORDERS_BOTTOM)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & ESDASHBOARD_CORNERS_BOTTOM_LEFT) offset1=outerRadius;
			if(priv->corners & ESDASHBOARD_CORNERS_BOTTOM_RIGHT) offset2=outerRadius;

			cogl_path_new();
			cogl_path_move_to(offset1, height);
			cogl_path_line_to(width-offset2, height);
			cogl_path_line_to(width-offset2, height-outerRadius+innerRadius);
			cogl_path_line_to(offset1, height-outerRadius+innerRadius);
			cogl_path_line_to(offset1, height);
			cogl_path_fill_preserve();
			cogl_path_close();
		}

		/* Bottom-left corner */
		if(priv->corners & ESDASHBOARD_CORNERS_BOTTOM_LEFT &&
			priv->borders & ESDASHBOARD_BORDERS_BOTTOM &&
			priv->borders & ESDASHBOARD_BORDERS_LEFT)
		{
			cogl_path_new();
			cogl_path_move_to(outerRadius, height);
			cogl_path_arc(outerRadius, height-outerRadius, outerRadius, outerRadius, 90, 180);
			cogl_path_line_to(outerRadius-innerRadius, height-outerRadius);
			cogl_path_arc(outerRadius, height-outerRadius, innerRadius, innerRadius, 180, 90);
			cogl_path_line_to(outerRadius, height);
			cogl_path_fill_preserve();
			cogl_path_close();
		}

		/* Left border */
		if(priv->borders & ESDASHBOARD_BORDERS_LEFT)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & ESDASHBOARD_CORNERS_TOP_LEFT) offset1=outerRadius;
			if(priv->corners & ESDASHBOARD_CORNERS_BOTTOM_LEFT) offset2=outerRadius;

			cogl_path_new();
			cogl_path_move_to(0, offset1);
			cogl_path_line_to(0, height-offset2);
			cogl_path_line_to(outerRadius-innerRadius, height-offset2);
			cogl_path_line_to(outerRadius-innerRadius, offset1);
			cogl_path_line_to(0, offset1);
			cogl_path_fill_preserve();
			cogl_path_close();
		}
	}
		else
		{
			/* Top border */
			if(priv->borders & ESDASHBOARD_BORDERS_TOP)
			{
				cogl_path_new();
				cogl_path_move_to(0, 0);
				cogl_path_line_to(width, 0);
				cogl_path_line_to(width, lineWidth);
				cogl_path_line_to(0, lineWidth);
				cogl_path_line_to(0, 0);
				cogl_path_fill_preserve();
				cogl_path_close();
			}

			/* Right border */
			if(priv->borders & ESDASHBOARD_BORDERS_RIGHT)
			{
				cogl_path_new();
				cogl_path_move_to(width, 0);
				cogl_path_line_to(width, height);
				cogl_path_line_to(width-lineWidth, height);
				cogl_path_line_to(width-lineWidth, 0);
				cogl_path_line_to(width, 0);
				cogl_path_fill_preserve();
				cogl_path_close();
			}

			/* Bottom border */
			if(priv->borders & ESDASHBOARD_BORDERS_BOTTOM)
			{
				cogl_path_new();
				cogl_path_move_to(0, height);
				cogl_path_line_to(width, height);
				cogl_path_line_to(width, height-lineWidth);
				cogl_path_line_to(0, height-lineWidth);
				cogl_path_line_to(0, height);
				cogl_path_fill_preserve();
				cogl_path_close();
			}

			/* Left border */
			if(priv->borders & ESDASHBOARD_BORDERS_LEFT)
			{
				cogl_path_new();
				cogl_path_move_to(0, 0);
				cogl_path_line_to(0, height);
				cogl_path_line_to(lineWidth, height);
				cogl_path_line_to(lineWidth, 0);
				cogl_path_line_to(0, 0);
				cogl_path_fill_preserve();
				cogl_path_close();
			}
		}

}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_outline_effect_dispose(GObject *inObject)
{
	/* Release allocated variables */
	EsdashboardOutlineEffect			*self=ESDASHBOARD_OUTLINE_EFFECT(inObject);
	EsdashboardOutlineEffectPrivate		*priv=self->priv;

	if(priv->color)
	{
		clutter_color_free(priv->color);
		priv->color=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_outline_effect_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_outline_effect_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	EsdashboardOutlineEffect			*self=ESDASHBOARD_OUTLINE_EFFECT(inObject);

	switch(inPropID)
	{
		case PROP_COLOR:
			esdashboard_outline_effect_set_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_WIDTH:
			esdashboard_outline_effect_set_width(self, g_value_get_float(inValue));
			break;

		case PROP_BORDERS:
			esdashboard_outline_effect_set_borders(self, g_value_get_flags(inValue));
			break;

		case PROP_CORNERS:
			esdashboard_outline_effect_set_corners(self, g_value_get_flags(inValue));
			break;

		case PROP_CORNERS_RADIUS:
			esdashboard_outline_effect_set_corner_radius(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_outline_effect_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	EsdashboardOutlineEffect			*self=ESDASHBOARD_OUTLINE_EFFECT(inObject);
	EsdashboardOutlineEffectPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_COLOR:
			clutter_value_set_color(outValue, priv->color);
			break;

		case PROP_WIDTH:
			g_value_set_float(outValue, priv->width);
			break;

		case PROP_BORDERS:
			g_value_set_flags(outValue, priv->borders);
			break;

		case PROP_CORNERS:
			g_value_set_flags(outValue, priv->corners);
			break;

		case PROP_CORNERS_RADIUS:
			g_value_set_float(outValue, priv->cornersRadius);
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
static void esdashboard_outline_effect_class_init(EsdashboardOutlineEffectClass *klass)
{
	ClutterEffectClass				*effectClass=CLUTTER_EFFECT_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_outline_effect_dispose;
	gobjectClass->set_property=_esdashboard_outline_effect_set_property;
	gobjectClass->get_property=_esdashboard_outline_effect_get_property;

	effectClass->paint=_esdashboard_outline_effect_paint;

	/* Define properties */
	EsdashboardOutlineEffectProperties[PROP_COLOR]=
		clutter_param_spec_color("color",
									"Color",
									"Color to draw outline with",
									CLUTTER_COLOR_White,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardOutlineEffectProperties[PROP_WIDTH]=
		g_param_spec_float("width",
							"Width",
							"Width of line used to draw outline",
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardOutlineEffectProperties[PROP_BORDERS]=
		g_param_spec_flags("borders",
							"Borders",
							"Determines which sides of the border to draw",
							ESDASHBOARD_TYPE_BORDERS,
							ESDASHBOARD_BORDERS_ALL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardOutlineEffectProperties[PROP_CORNERS]=
		g_param_spec_flags("corners",
							"Corners",
							"Determines which corners are rounded",
							ESDASHBOARD_TYPE_CORNERS,
							ESDASHBOARD_CORNERS_ALL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardOutlineEffectProperties[PROP_CORNERS_RADIUS]=
		g_param_spec_float("corner-radius",
							"Corner radius",
							"Radius of rounded corners",
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardOutlineEffectProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_outline_effect_init(EsdashboardOutlineEffect *self)
{
	EsdashboardOutlineEffectPrivate	*priv;

	priv=self->priv=esdashboard_outline_effect_get_instance_private(self);

	/* Set up default values */
	priv->color=clutter_color_copy(CLUTTER_COLOR_White);
	priv->width=1.0f;
	priv->borders=ESDASHBOARD_BORDERS_ALL;
	priv->corners=ESDASHBOARD_CORNERS_ALL;
	priv->cornersRadius=0.0f;
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterEffect* esdashboard_outline_effect_new(void)
{
	return(g_object_new(ESDASHBOARD_TYPE_OUTLINE_EFFECT, NULL));
}

/* Get/set color to draw outline with */
const ClutterColor* esdashboard_outline_effect_get_color(EsdashboardOutlineEffect *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self), NULL);

	return(self->priv->color);
}

void esdashboard_outline_effect_set_color(EsdashboardOutlineEffect *self, const ClutterColor *inColor)
{
	EsdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->color==NULL || clutter_color_equal(inColor, priv->color)==FALSE)
	{
		/* Set value */
		if(priv->color) clutter_color_free(priv->color);
		priv->color=clutter_color_copy(inColor);

		/* Invalidate effect to get it redrawn */
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardOutlineEffectProperties[PROP_COLOR]);
	}
}

/* Get/set line width for outline */
gfloat esdashboard_outline_effect_get_width(EsdashboardOutlineEffect *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self), 0.0f);

	return(self->priv->width);
}

void esdashboard_outline_effect_set_width(EsdashboardOutlineEffect *self, const gfloat inWidth)
{
	EsdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self));
	g_return_if_fail(inWidth>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->width!=inWidth)
	{
		/* Set value */
		priv->width=inWidth;

		/* Invalidate effect to get it redrawn */
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardOutlineEffectProperties[PROP_WIDTH]);
	}
}

/* Get/set sides of border to draw  */
EsdashboardBorders esdashboard_outline_effect_get_borders(EsdashboardOutlineEffect *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self), ESDASHBOARD_BORDERS_NONE);

	return(self->priv->borders);
}

void esdashboard_outline_effect_set_borders(EsdashboardOutlineEffect *self, EsdashboardBorders inBorders)
{
	EsdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->borders!=inBorders)
	{
		/* Set value */
		priv->borders=inBorders;

		/* Invalidate effect to get it redrawn */
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardOutlineEffectProperties[PROP_BORDERS]);
	}
}

/* Get/set corners of rectangle to draw rounded */
EsdashboardCorners esdashboard_outline_effect_get_corners(EsdashboardOutlineEffect *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self), ESDASHBOARD_CORNERS_NONE);

	return(self->priv->corners);
}

void esdashboard_outline_effect_set_corners(EsdashboardOutlineEffect *self, EsdashboardCorners inCorners)
{
	EsdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->corners!=inCorners)
	{
		/* Set value */
		priv->corners=inCorners;

		/* Invalidate effect to get it redrawn */
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardOutlineEffectProperties[PROP_CORNERS]);
	}
}

/* Get/set radius for rounded corners */
gfloat esdashboard_outline_effect_get_corner_radius(EsdashboardOutlineEffect *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self), 0.0f);

	return(self->priv->cornersRadius);
}

void esdashboard_outline_effect_set_corner_radius(EsdashboardOutlineEffect *self, const gfloat inRadius)
{
	EsdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_OUTLINE_EFFECT(self));
	g_return_if_fail(inRadius>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->cornersRadius!=inRadius)
	{
		/* Set value */
		priv->cornersRadius=inRadius;

		/* Invalidate effect to get it redrawn */
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardOutlineEffectProperties[PROP_CORNERS_RADIUS]);
	}
}
