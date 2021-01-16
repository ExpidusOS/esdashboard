/*
 * popup-menu-item-separator: A separator menu item
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
 * SECTION:popup-menu-item-separator
 * @short_description: A pop-up menu item separating menu items to group them
 *                     visually
 * @include: esdashboard/popup-menu-item-separator.h
 *
 * The #EsdashboardPopupMenuItemSeparator is a pop-up menu item to be used in a
 * #EsdashboardPopupMenu to group pop-up menu items within a menu. It displays a
 * horizontal line to separate the groups of pop-up menu items.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/popup-menu-item-separator.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/popup-menu-item.h>
#include <libesdashboard/click-action.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
static void _esdashboard_popup_menu_item_separator_popup_menu_item_iface_init(EsdashboardPopupMenuItemInterface *iface);

struct _EsdashboardPopupMenuItemSeparatorPrivate
{
	/* Properties related */
	gint						minHeight;

	gfloat						lineHorizontalAlign;
	gfloat						lineVerticalAlign;
	gfloat						lineLength;
	gfloat						lineWidth;
	ClutterColor				*lineColor;

	/* Instance related */
	ClutterContent				*lineCanvas;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardPopupMenuItemSeparator,
						esdashboard_popup_menu_item_separator,
						ESDASHBOARD_TYPE_BACKGROUND,
						G_ADD_PRIVATE(EsdashboardPopupMenuItemSeparator)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_POPUP_MENU_ITEM, _esdashboard_popup_menu_item_separator_popup_menu_item_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_MINIMUM_HEIGHT,

	PROP_LINE_HORIZONTAL_ALIGNMENT,
	PROP_LINE_VERTICAL_ALIGNMENT,
	PROP_LINE_LENGTH,
	PROP_LINE_WIDTH,
	PROP_LINE_COLOR,

	PROP_LAST
};

static GParamSpec* EsdashboardPopupMenuItemSeparatorProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Rectangle canvas should be redrawn */
static gboolean _esdashboard_popup_menu_item_separator_on_draw_line_canvas(EsdashboardPopupMenuItemSeparator *self,
																			cairo_t *inContext,
																			int inWidth,
																			int inHeight,
																			gpointer inUserData)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv;
	ClutterPoint								beginPosition, endPosition;
	gfloat										lineLength;

	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_CANVAS(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Clear current contents of the canvas */
	cairo_save(inContext);
	cairo_set_operator(inContext, CAIRO_OPERATOR_CLEAR);
	cairo_paint(inContext);
	cairo_restore(inContext);

	cairo_set_operator(inContext, CAIRO_OPERATOR_SOURCE);

	/* Do nothing if line width or line length is zero */
	if(priv->lineLength<=0.0f || priv->lineWidth<=0.0f) return(CLUTTER_EVENT_PROPAGATE);

	/* Determine line length for canvas size */
	lineLength=priv->lineLength*inWidth;

	/* Determine start and end position of line depending on alignment, line's length
	 * and width etc.
	 */
	beginPosition.x=MAX(0, (inWidth*priv->lineHorizontalAlign)-(lineLength/2.0f));
	beginPosition.y=MIN(inHeight, (inHeight*priv->lineVerticalAlign)+(priv->lineWidth/2.0f));

	endPosition.x=MIN(inWidth, inWidth*priv->lineHorizontalAlign)+(lineLength/2.0f);
	endPosition.y=beginPosition.y;

	/* Draw line */
	cairo_move_to(inContext, beginPosition.x, beginPosition.y);
	cairo_line_to(inContext, endPosition.x, endPosition.y);


	if(priv->lineColor) clutter_cairo_set_source_color(inContext, priv->lineColor);
	cairo_set_line_width(inContext, priv->lineWidth);

	cairo_stroke(inContext);

	/* Done drawing */
	cairo_close_path(inContext);

	return(CLUTTER_EVENT_PROPAGATE);
}

/* IMPLEMENTATION: Interface EsdashboardPopupMenuItem */

/* Get enable state of this pop-up menu item */
static gboolean _esdashboard_popup_menu_item_separator_popup_menu_item_get_enabled(EsdashboardPopupMenuItem *inMenuItem)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(inMenuItem), FALSE);

	/* Pop-up menu item separators are always disabled so return FALSE */
	return(FALSE);
}

/* Set enable state of this pop-up menu item */
static void _esdashboard_popup_menu_item_separator_popup_menu_item_set_enabled(EsdashboardPopupMenuItem *inMenuItem, gboolean inEnabled)
{
	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(inMenuItem));

	/* Pop-up menu item separators are always disabled so warn if someone tries
	 * to enabled it, otherwise ignore silently and do nothing.
	 */
	if(inEnabled)
	{
		g_warning("Object of type %s is always disabled and cannot be enabled.",
					G_OBJECT_TYPE_NAME(inMenuItem));
	}
}

/* Interface initialization
 * Set up default functions
 */
void _esdashboard_popup_menu_item_separator_popup_menu_item_iface_init(EsdashboardPopupMenuItemInterface *iface)
{
	iface->get_enabled=_esdashboard_popup_menu_item_separator_popup_menu_item_get_enabled;
	iface->set_enabled=_esdashboard_popup_menu_item_separator_popup_menu_item_set_enabled;
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _esdashboard_popup_menu_item_separator_get_preferred_height(ClutterActor *inActor,
																		gfloat inForWidth,
																		gfloat *outMinHeight,
																		gfloat *outNaturalHeight)
{
	EsdashboardPopupMenuItemSeparator			*self=ESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(inActor);
	EsdashboardPopupMenuItemSeparatorPrivate	*priv=self->priv;
	gfloat										minHeight, naturalHeight;

	minHeight=naturalHeight=priv->minHeight;

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

/* Paint actor */
static void _esdashboard_popup_menu_item_separator_paint_node(ClutterActor *self,
																ClutterPaintNode *inRootNode)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv=ESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(self)->priv;
	ClutterContentIface							*iface;

	/* Chain up to draw the actor */
	if(CLUTTER_ACTOR_CLASS(esdashboard_popup_menu_item_separator_parent_class)->paint_node)
	{
		CLUTTER_ACTOR_CLASS(esdashboard_popup_menu_item_separator_parent_class)->paint_node(self, inRootNode);
	}

	/* Now draw canvas for line */
	if(priv->lineCanvas)
	{
		iface=CLUTTER_CONTENT_GET_IFACE(priv->lineCanvas);
		if(iface->paint_content) iface->paint_content(priv->lineCanvas, self, inRootNode);
	}
}

/* Allocate position and size of actor and its children */
static void _esdashboard_popup_menu_item_separator_allocate(ClutterActor *self,
															const ClutterActorBox *inBox,
															ClutterAllocationFlags inFlags)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv=ESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(self)->priv;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(esdashboard_popup_menu_item_separator_parent_class)->allocate(self, inBox, inFlags);

	/* Set size of canvas */
	if(priv->lineCanvas)
	{
		clutter_canvas_set_size(CLUTTER_CANVAS(priv->lineCanvas),
									clutter_actor_box_get_width(inBox),
									clutter_actor_box_get_height(inBox));
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_popup_menu_item_separator_dispose(GObject *inObject)
{
	/* Release allocated variables */
	EsdashboardPopupMenuItemSeparatorPrivate	*priv=ESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(inObject)->priv;

	if(priv->lineCanvas)
	{
		g_object_unref(priv->lineCanvas);
		priv->lineCanvas=NULL;
	}

	if(priv->lineColor)
	{
		clutter_color_free(priv->lineColor);
		priv->lineColor=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_popup_menu_item_separator_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_popup_menu_item_separator_set_property(GObject *inObject,
																guint inPropID,
																const GValue *inValue,
																GParamSpec *inSpec)
{
	EsdashboardPopupMenuItemSeparator			*self=ESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(inObject);

	switch(inPropID)
	{
		case PROP_MINIMUM_HEIGHT:
			esdashboard_popup_menu_item_separator_set_minimum_height(self, g_value_get_float(inValue));
			break;

		case PROP_LINE_HORIZONTAL_ALIGNMENT:
			esdashboard_popup_menu_item_separator_set_line_horizontal_alignment(self, g_value_get_float(inValue));
			break;

		case PROP_LINE_VERTICAL_ALIGNMENT:
			esdashboard_popup_menu_item_separator_set_line_vertical_alignment(self, g_value_get_float(inValue));
			break;

		case PROP_LINE_LENGTH:
			esdashboard_popup_menu_item_separator_set_line_length(self, g_value_get_float(inValue));
			break;

		case PROP_LINE_WIDTH:
			esdashboard_popup_menu_item_separator_set_line_width(self, g_value_get_float(inValue));
			break;

		case PROP_LINE_COLOR:
			esdashboard_popup_menu_item_separator_set_line_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_popup_menu_item_separator_get_property(GObject *inObject,
																guint inPropID,
																GValue *outValue,
																GParamSpec *inSpec)
{
	EsdashboardPopupMenuItemSeparator			*self=ESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(inObject);
	EsdashboardPopupMenuItemSeparatorPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_MINIMUM_HEIGHT:
			g_value_set_float(outValue, priv->minHeight);
			break;

		case PROP_LINE_HORIZONTAL_ALIGNMENT:
			g_value_set_float(outValue, priv->lineHorizontalAlign);
			break;

		case PROP_LINE_VERTICAL_ALIGNMENT:
			g_value_set_float(outValue, priv->lineVerticalAlign);
			break;

		case PROP_LINE_LENGTH:
			g_value_set_float(outValue, priv->lineLength);
			break;

		case PROP_LINE_WIDTH:
			g_value_set_float(outValue, priv->lineWidth);
			break;

		case PROP_LINE_COLOR:
			clutter_value_set_color(outValue, priv->lineColor);
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
static void esdashboard_popup_menu_item_separator_class_init(EsdashboardPopupMenuItemSeparatorClass *klass)
{
	EsdashboardActorClass				*actorClass=ESDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass					*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass						*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_popup_menu_item_separator_dispose;
	gobjectClass->set_property=_esdashboard_popup_menu_item_separator_set_property;
	gobjectClass->get_property=_esdashboard_popup_menu_item_separator_get_property;

	clutterActorClass->get_preferred_height=_esdashboard_popup_menu_item_separator_get_preferred_height;
	clutterActorClass->paint_node=_esdashboard_popup_menu_item_separator_paint_node;
	clutterActorClass->allocate=_esdashboard_popup_menu_item_separator_allocate;

	/* Define properties */
	/**
	 * EsdashboardPopupMenuItemSeparator:minimum-height:
	 *
	 * A forced minimum height request for the pop-up menu item separator, in pixels
	 */
	EsdashboardPopupMenuItemSeparatorProperties[PROP_MINIMUM_HEIGHT]=
		g_param_spec_float("minimum-height",
							"Minimum height",
							"Forced minimum height request for the actor",
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenuItemSeparator:line-horizontal-alignment:
	 *
	 * The alignment of this pop-up menu item separator on the X axis given as
	 * fraction between 0.0 and 1.0
	 */
	EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_HORIZONTAL_ALIGNMENT]=
		g_param_spec_float("line-horizontal-alignment",
							"Line horizontal alignment",
							"The horizontal alignment of the actor within its allocation",
							0.0f, 1.0f,
							0.5f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenuItemSeparator:line-vertical-alignment:
	 *
	 * The alignment of this pop-up menu item separator on the Y axis given as
	 * fraction between 0.0 and 1.0
	 */
	EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_VERTICAL_ALIGNMENT]=
		g_param_spec_float("line-vertical-alignment",
							"Line vertical alignment",
							"The vertical alignment of the actor within its allocation",
							0.0f, 1.0f,
							0.5f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenuItemSeparator:line-length:
	 *
	 * The line's length at this pop-up menu item separator given as fraction
	 * between 0.0 and 1.0 (full actor's allocation width).
	 */
	EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_LENGTH]=
		g_param_spec_float("line-length",
							"Line length",
							"Fraction of length of line",
							0.0f, 1.0f,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenuItemSeparator:line-width:
	 *
	 * The width of line at the pop-up menu item separator, in pixels
	 */
	EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_WIDTH]=
		g_param_spec_float("line-width",
							"Line width",
							"Width of line",
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenuItemSeparator:line-color:
	 *
	 * The color of line at the pop-up menu item separator
	 */
	EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_COLOR]=
		clutter_param_spec_color("line-color",
									"Line color",
									"Color to draw line with",
									CLUTTER_COLOR_White,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardPopupMenuItemSeparatorProperties);

	/* Define stylable properties */
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardPopupMenuItemSeparatorProperties[PROP_MINIMUM_HEIGHT]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_HORIZONTAL_ALIGNMENT]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_VERTICAL_ALIGNMENT]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_LENGTH]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_WIDTH]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_COLOR]);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_popup_menu_item_separator_init(EsdashboardPopupMenuItemSeparator *self)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv;

	priv=self->priv=esdashboard_popup_menu_item_separator_get_instance_private(self);

	/* Set up default values */
	priv->minHeight=4.0f;

	priv->lineCanvas=clutter_canvas_new();
	priv->lineVerticalAlign=0.5f;
	priv->lineHorizontalAlign=0.5f;
	priv->lineLength=1.0f;
	priv->lineWidth=1.0f;
	priv->lineColor=clutter_color_copy(CLUTTER_COLOR_White);

	/* This actor cannot reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), FALSE);

	/* Connect signals */
	g_signal_connect_swapped(priv->lineCanvas,
								"draw",
								G_CALLBACK(_esdashboard_popup_menu_item_separator_on_draw_line_canvas),
								self);
}

/* IMPLEMENTATION: Public API */

/**
 * esdashboard_popup_menu_item_separator_new:
 *
 * Creates a new #EsdashboardPopupMenuItemSeparator pop-up menu item
 *
 * Return value: The newly created #EsdashboardPopupMenuItemSeparator
 */
ClutterActor* esdashboard_popup_menu_item_separator_new(void)
{
	return(g_object_new(ESDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR,
						NULL));
}

/**
 * esdashboard_popup_menu_item_separator_get_minimum_height:
 * @self: A #EsdashboardPopupMenuItemSeparator
 *
 * Retrieves the minimum height in pixels forced at pop-up menu item separator at @self.
 *
 * Return value: Returns float with forced minimum height in pixels.
 */
gfloat esdashboard_popup_menu_item_separator_get_minimum_height(EsdashboardPopupMenuItemSeparator *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self), 0.0f);

	return(self->priv->minHeight);
}

/**
 * esdashboard_popup_menu_item_separator_set_minimum_height:
 * @self: A #EsdashboardPopupMenuItemSeparator
 * @inMinimumHeight: The forced minimum height to be set at this pop-up menu item separator
 *
 * Forces a minimum height of @inMinimumHeight to be set at the pop-up menu item
 * separator at @self. The height at @inMinimumHeight must be zero or higher.
 */
void esdashboard_popup_menu_item_separator_set_minimum_height(EsdashboardPopupMenuItemSeparator *self, const gfloat inMinimumHeight)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self));
	g_return_if_fail(inMinimumHeight>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->minHeight!=inMinimumHeight)
	{
		/* Set value */
		priv->minHeight=inMinimumHeight;

		/* Relayout actor because its (minimum) size has changed */
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuItemSeparatorProperties[PROP_MINIMUM_HEIGHT]);
	}
}

/**
 * esdashboard_popup_menu_item_separator_get_line_horizontal_alignment:
 * @self: A #EsdashboardPopupMenuItemSeparator
 *
 * Retrieves the horizontal alignment of line of pop-up menu item separator at @self
 * as fraction between 0 and 1. A value of 0 means the line is aligned to the left
 * side of the actor's allocation box and a value of 1 means it is aligned to the
 * right side.
 *
 * Return value: Returns float with horizontal alignment as fraction.
 */
gfloat esdashboard_popup_menu_item_separator_get_line_horizontal_alignment(EsdashboardPopupMenuItemSeparator *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self), 0.0f);

	return(self->priv->lineHorizontalAlign);
}

/**
 * esdashboard_popup_menu_item_separator_set_line_horizontal_alignment:
 * @self: A #EsdashboardPopupMenuItemSeparator
 * @inAlignment: The horizontal alignment
 *
 * Sets the horizontal alignment of line of pop-up menu item separator at @self. The
 * alignment value at @inAlignment is a fraction between 0 and 1 whereby a value of 0
 * means the line should be aligned to the left side of the actor's allocation box
 * and a value of 1 aligns to the right side.
 */
void esdashboard_popup_menu_item_separator_set_line_horizontal_alignment(EsdashboardPopupMenuItemSeparator *self, gfloat inAlignment)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self));
	g_return_if_fail(inAlignment>=0.0f && inAlignment<=1.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->lineHorizontalAlign!=inAlignment)
	{
		/* Set value */
		priv->lineHorizontalAlign=inAlignment;

		/* Invalidate canvas to get it redrawn */
		if(priv->lineCanvas) clutter_content_invalidate(priv->lineCanvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_HORIZONTAL_ALIGNMENT]);
	}
}

/**
 * esdashboard_popup_menu_item_separator_get_line_vertical_alignment:
 * @self: A #EsdashboardPopupMenuItemSeparator
 *
 * Retrieves the vertical alignment of line of pop-up menu item separator at @self
 * as fraction between 0 and 1. A value of 0 means the line is aligned to the top
 * side of the actor's allocation box and a value of 1 means it is aligned to the
 * bottom side.
 *
 * Return value: Returns float with vertical alignment as fraction.
 */
gfloat esdashboard_popup_menu_item_separator_get_line_vertical_alignment(EsdashboardPopupMenuItemSeparator *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self), 0.0f);

	return(self->priv->lineVerticalAlign);
}

/**
 * esdashboard_popup_menu_item_separator_set_line_vertical_alignment:
 * @self: A #EsdashboardPopupMenuItemSeparator
 * @inAlignment: The vertical alignment
 *
 * Sets the vertical alignment of line of pop-up menu item separator at @self. The
 * alignment value at @inAlignment is a fraction between 0 and 1 whereby a value of 0
 * means the line should be aligned to the top side of the actor's allocation box
 * and a value of 1 aligns to the bottom side.
 */
void esdashboard_popup_menu_item_separator_set_line_vertical_alignment(EsdashboardPopupMenuItemSeparator *self, gfloat inAlignment)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self));
	g_return_if_fail(inAlignment>=0.0f && inAlignment<=1.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->lineVerticalAlign!=inAlignment)
	{
		/* Set value */
		priv->lineVerticalAlign=inAlignment;

		/* Invalidate canvas to get it redrawn */
		if(priv->lineCanvas) clutter_content_invalidate(priv->lineCanvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_VERTICAL_ALIGNMENT]);
	}
}

/**
 * esdashboard_popup_menu_item_separator_get_line_length:
 * @self: A #EsdashboardPopupMenuItemSeparator
 *
 * Retrieves the length of line of pop-up menu item separator at @self as a floating
 * point value between 0.0 and 1.0. This value can be interpreted as percentage
 * of available width which the line should cover, e.g. a value of 1.0 (100%) means
 * the line should cover the available width fully.
 *
 * Return value: Returns floating point value with lenght of line.
 */
gfloat esdashboard_popup_menu_item_separator_get_line_length(EsdashboardPopupMenuItemSeparator *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self), 0.0f);

	return(self->priv->lineLength);
}

/**
 * esdashboard_popup_menu_item_separator_set_line_length:
 * @self: A #EsdashboardPopupMenuItemSeparator
 * @inLength: The length of line at pop-up menu item separator
 *
 * Sets the lenght of line to @inLength which must a floating point value between
 * 0.0 and 1.0 and which is interpreted as a percentage value of the available width
 * the line should cover pop-up menu item separator at @self.
 */
void esdashboard_popup_menu_item_separator_set_line_length(EsdashboardPopupMenuItemSeparator *self, gfloat inLength)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self));
	g_return_if_fail(inLength>=0.0f && inLength<=1.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->lineLength!=inLength)
	{
		/* Set value */
		priv->lineLength=inLength;

		/* Invalidate canvas to get it redrawn */
		if(priv->lineCanvas) clutter_content_invalidate(priv->lineCanvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_LENGTH]);
	}
}

/**
 * esdashboard_popup_menu_item_separator_get_line_width:
 * @self: A #EsdashboardPopupMenuItemSeparator
 *
 * Retrieves the line's width of pop-up menu item separator at @self.
 *
 * Return value: Returns float with width of line in pixels.
 */
gfloat esdashboard_popup_menu_item_separator_get_line_width(EsdashboardPopupMenuItemSeparator *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self), 0.0f);

	return(self->priv->lineWidth);
}

/**
 * esdashboard_popup_menu_item_separator_set_minimum_height:
 * @self: A #EsdashboardPopupMenuItemSeparator
 * @inWidth: The width of line at pop-up menu item separator
 *
 * Set the width of line to @inWidth at the pop-up menu item separator at @self.
 */
void esdashboard_popup_menu_item_separator_set_line_width(EsdashboardPopupMenuItemSeparator *self, gfloat inWidth)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self));
	g_return_if_fail(inWidth>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->lineWidth!=inWidth)
	{
		/* Set value */
		priv->lineWidth=inWidth;

		/* Invalidate canvas to get it redrawn */
		if(priv->lineCanvas) clutter_content_invalidate(priv->lineCanvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_WIDTH]);
	}
}

/**
 * esdashboard_popup_menu_item_separator_get_line_color:
 * @self: A #EsdashboardPopupMenuItemSeparator
 *
 * Retrieves the color used when the line of pop-up menu item separator at @self
 * is drawn.
 *
 * Return value: Returns #ClutterColor containing the color of line
 */
const ClutterColor* esdashboard_popup_menu_item_separator_get_line_color(EsdashboardPopupMenuItemSeparator *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self), NULL);

	return(self->priv->lineColor);
}

/**
 * esdashboard_popup_menu_item_separator_set_line_color:
 * @self: A #EsdashboardPopupMenuItemSeparator
 * @inColor: The #ClutterColor for line of separator
 *
 * Sets the color to @inColor to be used when the pop-up menu item separator at
 * @self is drawn.
 */
void esdashboard_popup_menu_item_separator_set_line_color(EsdashboardPopupMenuItemSeparator *self, const ClutterColor *inColor)
{
	EsdashboardPopupMenuItemSeparatorPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->lineColor==NULL || clutter_color_equal(inColor, priv->lineColor)==FALSE)
	{
		/* Set value */
		if(priv->lineColor) clutter_color_free(priv->lineColor);
		priv->lineColor=clutter_color_copy(inColor);

		/* Invalidate canvas to get it redrawn */
		if(priv->lineCanvas) clutter_content_invalidate(priv->lineCanvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuItemSeparatorProperties[PROP_LINE_COLOR]);
	}
}
