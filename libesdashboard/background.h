/*
 * background: An actor providing background rendering. Usually other
 *             actors are derived from this one.
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

#ifndef __LIBESDASHBOARD_BACKGROUND__
#define __LIBESDASHBOARD_BACKGROUND__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/actor.h>
#include <libesdashboard/types.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * EsdashboardBackgroundType:
 * @ESDASHBOARD_BACKGROUND_TYPE_NONE: The actor will be displayed unmodified.
 * @ESDASHBOARD_BACKGROUND_TYPE_FILL: The actor background will be filled with a color.
 * @ESDASHBOARD_BACKGROUND_TYPE_OUTLINE: The actor will get an outline.
 * @ESDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS: The edges of actor will be rounded.
 *
 * Determines how the background of an actor will be displayed and if it get an styled outline.
 */
typedef enum /*< flags,prefix=ESDASHBOARD_BACKGROUND_TYPE >*/
{
	ESDASHBOARD_BACKGROUND_TYPE_NONE=0,

	ESDASHBOARD_BACKGROUND_TYPE_FILL=1 << 0,
	ESDASHBOARD_BACKGROUND_TYPE_OUTLINE=1 << 1,
	ESDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS=1 << 2,
} EsdashboardBackgroundType;


/* Object declaration */
#define ESDASHBOARD_TYPE_BACKGROUND					(esdashboard_background_get_type())
#define ESDASHBOARD_BACKGROUND(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_BACKGROUND, EsdashboardBackground))
#define ESDASHBOARD_IS_BACKGROUND(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_BACKGROUND))
#define ESDASHBOARD_BACKGROUND_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_BACKGROUND, EsdashboardBackgroundClass))
#define ESDASHBOARD_IS_BACKGROUND_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_BACKGROUND))
#define ESDASHBOARD_BACKGROUND_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_BACKGROUND, EsdashboardBackgroundClass))

typedef struct _EsdashboardBackground				EsdashboardBackground;
typedef struct _EsdashboardBackgroundClass			EsdashboardBackgroundClass;
typedef struct _EsdashboardBackgroundPrivate		EsdashboardBackgroundPrivate;

/**
 * EsdashboardBackground:
 *
 * The #EsdashboardBackground structure contains only private data and
 * should be accessed using the provided API
 */struct _EsdashboardBackground
{
	/*< private >*/
	/* Parent instance */
	EsdashboardActor				parent_instance;

	/* Private structure */
	EsdashboardBackgroundPrivate	*priv;
};

/**
 * EsdashboardBackgroundClass:
 *
 * The #EsdashboardBackgroundClass structure contains only private data
 */
struct _EsdashboardBackgroundClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardActorClass			parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_background_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_background_new(void);

/* General functions */
EsdashboardBackgroundType esdashboard_background_get_background_type(EsdashboardBackground *self);
void esdashboard_background_set_background_type(EsdashboardBackground *self, const EsdashboardBackgroundType inType);

void esdashboard_background_set_corners(EsdashboardBackground *self, EsdashboardCorners inCorners);
void esdashboard_background_set_corner_radius(EsdashboardBackground *self, const gfloat inRadius);

/* Fill functions */
const ClutterColor* esdashboard_background_get_fill_color(EsdashboardBackground *self);
void esdashboard_background_set_fill_color(EsdashboardBackground *self, const ClutterColor *inColor);

EsdashboardCorners esdashboard_background_get_fill_corners(EsdashboardBackground *self);
void esdashboard_background_set_fill_corners(EsdashboardBackground *self, EsdashboardCorners inCorners);

gfloat esdashboard_background_get_fill_corner_radius(EsdashboardBackground *self);
void esdashboard_background_set_fill_corner_radius(EsdashboardBackground *self, const gfloat inRadius);

/* Outline functions */
const ClutterColor* esdashboard_background_get_outline_color(EsdashboardBackground *self);
void esdashboard_background_set_outline_color(EsdashboardBackground *self, const ClutterColor *inColor);

gfloat esdashboard_background_get_outline_width(EsdashboardBackground *self);
void esdashboard_background_set_outline_width(EsdashboardBackground *self, const gfloat inWidth);

EsdashboardBorders esdashboard_background_get_outline_borders(EsdashboardBackground *self);
void esdashboard_background_set_outline_borders(EsdashboardBackground *self, EsdashboardBorders inBorders);

EsdashboardCorners esdashboard_background_get_outline_corners(EsdashboardBackground *self);
void esdashboard_background_set_outline_corners(EsdashboardBackground *self, EsdashboardCorners inCorners);

gfloat esdashboard_background_get_outline_corner_radius(EsdashboardBackground *self);
void esdashboard_background_set_outline_corner_radius(EsdashboardBackground *self, const gfloat inRadius);

/* Image functions */
ClutterImage* esdashboard_background_get_image(EsdashboardBackground *self);
void esdashboard_background_set_image(EsdashboardBackground *self, ClutterImage *inImage);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_BACKGROUND__ */
