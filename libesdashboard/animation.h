/*
 * animation: A animation for an actor
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

#ifndef __LIBESDASHBOARD_ANIMATION__
#define __LIBESDASHBOARD_ANIMATION__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/actor.h>
#include <libesdashboard/css-selector.h>


G_BEGIN_DECLS

/* Object declaration */
#define ESDASHBOARD_TYPE_ANIMATION				(esdashboard_animation_get_type())
#define ESDASHBOARD_ANIMATION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_ANIMATION, EsdashboardAnimation))
#define ESDASHBOARD_IS_ANIMATION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_ANIMATION))
#define ESDASHBOARD_ANIMATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_ANIMATION, EsdashboardAnimationClass))
#define ESDASHBOARD_IS_ANIMATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_ANIMATION))
#define ESDASHBOARD_ANIMATION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_ANIMATION, EsdashboardAnimationClass))

typedef struct _EsdashboardAnimation			EsdashboardAnimation;
typedef struct _EsdashboardAnimationClass		EsdashboardAnimationClass;
typedef struct _EsdashboardAnimationPrivate		EsdashboardAnimationPrivate;

/**
 * EsdashboardAnimation:
 *
 * The #EsdashboardAnimation structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardAnimation
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardAnimationPrivate			*priv;
};

/**
 * EsdashboardAnimationClass:
 *
 * The #EsdashboardAnimationClass structure contains only private data
 */
struct _EsdashboardAnimationClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*add_animation)(EsdashboardAnimation *self, ClutterActor *inActor, ClutterTransition *inTransition);
	void (*animation_done)(EsdashboardAnimation *self);
};

/**
 * EsdashboardAnimationValue:
 * @selector: A #EsdashboardCssSelector to find matchhing actors for the property's value in animation or %NULL to match sender
 * @property: A string containing the name of the property this value belongs to
 * @value: A #GValue containing the value for the property
 *
 */
struct _EsdashboardAnimationValue
{
	EsdashboardCssSelector				*selector;
	gchar								*property;
	GValue								*value;
};

typedef struct _EsdashboardAnimationValue		EsdashboardAnimationValue;

/* Public API */
GType esdashboard_animation_get_type(void) G_GNUC_CONST;

EsdashboardAnimation* esdashboard_animation_new(EsdashboardActor *inSender, const gchar *inSignal);
EsdashboardAnimation* esdashboard_animation_new_with_values(EsdashboardActor *inSender,
															const gchar *inSignal,
															EsdashboardAnimationValue **inDefaultInitialValues,
															EsdashboardAnimationValue **inDefaultFinalValues);

EsdashboardAnimation* esdashboard_animation_new_by_id(EsdashboardActor *inSender, const gchar *inID);
EsdashboardAnimation* esdashboard_animation_new_by_id_with_values(EsdashboardActor *inSender,
																	const gchar *inID,
																	EsdashboardAnimationValue **inDefaultInitialValues,
																	EsdashboardAnimationValue **inDefaultFinalValues);

gboolean esdashboard_animation_has_animation(EsdashboardActor *inSender, const gchar *inSignal);

const gchar* esdashboard_animation_get_id(EsdashboardAnimation *self);

gboolean esdashboard_animation_is_empty(EsdashboardAnimation *self);

void esdashboard_animation_run(EsdashboardAnimation *self);

void esdashboard_animation_ensure_complete(EsdashboardAnimation *self);

void esdashboard_animation_dump(EsdashboardAnimation *self);

EsdashboardAnimationValue** esdashboard_animation_defaults_new(gint inNumberValues, ...);
void esdashboard_animation_defaults_free(EsdashboardAnimationValue **inDefaultValues);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_ANIMATION__ */
