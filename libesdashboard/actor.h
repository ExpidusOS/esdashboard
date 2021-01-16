/*
 * actor: Abstract base actor
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

#ifndef __LIBESDASHBOARD_ACTOR__
#define __LIBESDASHBOARD_ACTOR__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_ACTOR					(esdashboard_actor_get_type())
#define ESDASHBOARD_ACTOR(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_ACTOR, EsdashboardActor))
#define ESDASHBOARD_IS_ACTOR(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_ACTOR))
#define ESDASHBOARD_ACTOR_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_ACTOR, EsdashboardActorClass))
#define ESDASHBOARD_IS_ACTOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_ACTOR))
#define ESDASHBOARD_ACTOR_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_ACTOR, EsdashboardActorClass))

typedef struct _EsdashboardActor				EsdashboardActor;
typedef struct _EsdashboardActorClass			EsdashboardActorClass;
typedef struct _EsdashboardActorPrivate			EsdashboardActorPrivate;

struct _EsdashboardActor
{
	/*< private >*/
	/* Parent instance */
	ClutterActor				parent_instance;

	/* Private structure */
	EsdashboardActorPrivate		*priv;
};

struct _EsdashboardActorClass
{
	/*< private >*/
	/* Parent class */
	ClutterActorClass			parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_actor_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_actor_new(void);

gboolean esdashboard_actor_get_can_focus(EsdashboardActor *self);
void esdashboard_actor_set_can_focus(EsdashboardActor *self, gboolean inCanFous);

const gchar* esdashboard_actor_get_effects(EsdashboardActor *self);
void esdashboard_actor_set_effects(EsdashboardActor *self, const gchar *inEffects);

void esdashboard_actor_install_stylable_property(EsdashboardActorClass *klass, GParamSpec *inParamSpec);
void esdashboard_actor_install_stylable_property_by_name(EsdashboardActorClass *klass, const gchar *inParamName);
GHashTable* esdashboard_actor_get_stylable_properties(EsdashboardActorClass *klass);
GHashTable* esdashboard_actor_get_stylable_properties_full(EsdashboardActorClass *klass);

void esdashboard_actor_invalidate(EsdashboardActor *self);

void esdashboard_actor_enable_allocation_animation_once(EsdashboardActor *self);
void esdashboard_actor_get_allocation_box(EsdashboardActor *self, ClutterActorBox *outAllocationBox);

gboolean esdashboard_actor_destroy(ClutterActor *self);
void esdashboard_actor_destroy_all_children(ClutterActor *self);
gboolean esdashboard_actor_iter_destroy(ClutterActorIter *self);

G_END_DECLS

#endif
