/*
 * emblem-effect: Draws an emblem on top of an actor
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

#ifndef __LIBESDASHBOARD_EMBLEM_EFFECT__
#define __LIBESDASHBOARD_EMBLEM_EFFECT__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/types.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_EMBLEM_EFFECT				(esdashboard_emblem_effect_get_type())
#define ESDASHBOARD_EMBLEM_EFFECT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_EMBLEM_EFFECT, EsdashboardEmblemEffect))
#define ESDASHBOARD_IS_EMBLEM_EFFECT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_EMBLEM_EFFECT))
#define ESDASHBOARD_EMBLEM_EFFECT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_EMBLEM_EFFECT, EsdashboardEmblemEffectClass))
#define ESDASHBOARD_IS_EMBLEM_EFFECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_EMBLEM_EFFECT))
#define ESDASHBOARD_EMBLEM_EFFECT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_EMBLEM_EFFECT, EsdashboardEmblemEffectClass))

typedef struct _EsdashboardEmblemEffect			EsdashboardEmblemEffect;
typedef struct _EsdashboardEmblemEffectClass		EsdashboardEmblemEffectClass;
typedef struct _EsdashboardEmblemEffectPrivate		EsdashboardEmblemEffectPrivate;

struct _EsdashboardEmblemEffect
{
	/*< private >*/
	/* Parent instance */
	ClutterEffect						parent_instance;

	/* Private structure */
	EsdashboardEmblemEffectPrivate		*priv;
};

struct _EsdashboardEmblemEffectClass
{
	/*< private >*/
	/* Parent class */
	ClutterEffectClass					parent_class;
};

/* Public API */
GType esdashboard_emblem_effect_get_type(void) G_GNUC_CONST;

ClutterEffect* esdashboard_emblem_effect_new(void);

const gchar* esdashboard_emblem_effect_get_icon_name(EsdashboardEmblemEffect *self);
void esdashboard_emblem_effect_set_icon_name(EsdashboardEmblemEffect *self, const gchar *inIconName);

gint esdashboard_emblem_effect_get_icon_size(EsdashboardEmblemEffect *self);
void esdashboard_emblem_effect_set_icon_size(EsdashboardEmblemEffect *self, const gint inSize);

gfloat esdashboard_emblem_effect_get_padding(EsdashboardEmblemEffect *self);
void esdashboard_emblem_effect_set_padding(EsdashboardEmblemEffect *self, const gfloat inPadding);

gfloat esdashboard_emblem_effect_get_x_align(EsdashboardEmblemEffect *self);
void esdashboard_emblem_effect_set_x_align(EsdashboardEmblemEffect *self, const gfloat inAlign);

gfloat esdashboard_emblem_effect_get_y_align(EsdashboardEmblemEffect *self);
void esdashboard_emblem_effect_set_y_align(EsdashboardEmblemEffect *self, const gfloat inAlign);

EsdashboardAnchorPoint esdashboard_emblem_effect_get_anchor_point(EsdashboardEmblemEffect *self);
void esdashboard_emblem_effect_set_anchor_point(EsdashboardEmblemEffect *self, const EsdashboardAnchorPoint inAnchorPoint);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_EMBLEM_EFFECT__ */
