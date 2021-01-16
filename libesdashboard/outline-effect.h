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

#ifndef __LIBESDASHBOARD_OUTLINE_EFFECT__
#define __LIBESDASHBOARD_OUTLINE_EFFECT__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/types.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_OUTLINE_EFFECT				(esdashboard_outline_effect_get_type())
#define ESDASHBOARD_OUTLINE_EFFECT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_OUTLINE_EFFECT, EsdashboardOutlineEffect))
#define ESDASHBOARD_IS_OUTLINE_EFFECT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_OUTLINE_EFFECT))
#define ESDASHBOARD_OUTLINE_EFFECT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_OUTLINE_EFFECT, EsdashboardOutlineEffectClass))
#define ESDASHBOARD_IS_OUTLINE_EFFECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_OUTLINE_EFFECT))
#define ESDASHBOARD_OUTLINE_EFFECT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_OUTLINE_EFFECT, EsdashboardOutlineEffectClass))

typedef struct _EsdashboardOutlineEffect			EsdashboardOutlineEffect;
typedef struct _EsdashboardOutlineEffectClass		EsdashboardOutlineEffectClass;
typedef struct _EsdashboardOutlineEffectPrivate		EsdashboardOutlineEffectPrivate;

struct _EsdashboardOutlineEffect
{
	/*< private >*/
	/* Parent instance */
	ClutterEffect						parent_instance;

	/* Private structure */
	EsdashboardOutlineEffectPrivate		*priv;
};

struct _EsdashboardOutlineEffectClass
{
	/*< private >*/
	/* Parent class */
	ClutterEffectClass					parent_class;
};

/* Public API */
GType esdashboard_outline_effect_get_type(void) G_GNUC_CONST;

ClutterEffect* esdashboard_outline_effect_new(void);

const ClutterColor* esdashboard_outline_effect_get_color(EsdashboardOutlineEffect *self);
void esdashboard_outline_effect_set_color(EsdashboardOutlineEffect *self, const ClutterColor *inColor);

gfloat esdashboard_outline_effect_get_width(EsdashboardOutlineEffect *self);
void esdashboard_outline_effect_set_width(EsdashboardOutlineEffect *self, const gfloat inWidth);

EsdashboardBorders esdashboard_outline_effect_get_borders(EsdashboardOutlineEffect *self);
void esdashboard_outline_effect_set_borders(EsdashboardOutlineEffect *self, EsdashboardBorders inBorders);

EsdashboardCorners esdashboard_outline_effect_get_corners(EsdashboardOutlineEffect *self);
void esdashboard_outline_effect_set_corners(EsdashboardOutlineEffect *self, EsdashboardCorners inCorners);

gfloat esdashboard_outline_effect_get_corner_radius(EsdashboardOutlineEffect *self);
void esdashboard_outline_effect_set_corner_radius(EsdashboardOutlineEffect *self, const gfloat inRadius);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_OUTLINE_EFFECT__ */
