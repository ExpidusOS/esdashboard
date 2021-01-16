/*
 * theme-animation: A theme used for building animations by XML files
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

#ifndef __LIBESDASHBOARD_THEME_ANIMATION__
#define __LIBESDASHBOARD_THEME_ANIMATION__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/actor.h>
#include <libesdashboard/animation.h>

G_BEGIN_DECLS


/* Object declaration */
#define ESDASHBOARD_TYPE_THEME_ANIMATION				(esdashboard_theme_animation_get_type())
#define ESDASHBOARD_THEME_ANIMATION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_THEME_ANIMATION, EsdashboardThemeAnimation))
#define ESDASHBOARD_IS_THEME_ANIMATION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_THEME_ANIMATION))
#define ESDASHBOARD_THEME_ANIMATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_THEME_ANIMATION, EsdashboardThemeAnimationClass))
#define ESDASHBOARD_IS_THEME_ANIMATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_THEME_ANIMATION))
#define ESDASHBOARD_THEME_ANIMATION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_THEME_ANIMATION, EsdashboardThemeAnimationClass))

typedef struct _EsdashboardThemeAnimation				EsdashboardThemeAnimation;
typedef struct _EsdashboardThemeAnimationClass			EsdashboardThemeAnimationClass;
typedef struct _EsdashboardThemeAnimationPrivate		EsdashboardThemeAnimationPrivate;

/**
 * EsdashboardThemeAnimation:
 *
 * The #EsdashboardThemeAnimation structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardThemeAnimation
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	EsdashboardThemeAnimationPrivate		*priv;
};

/**
 * EsdashboardThemeAnimationClass:
 *
 * The #EsdashboardThemeAnimationClass structure contains only private data
 */
struct _EsdashboardThemeAnimationClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define ESDASHBOARD_THEME_ANIMATION_ERROR				(esdashboard_theme_animation_error_quark())

GQuark esdashboard_theme_animation_error_quark(void);

typedef enum /*< prefix=ESDASHBOARD_THEME_ANIMATION_ERROR >*/
{
	ESDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
	ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
} EsdashboardThemeAnimationErrorEnum;

/* Public API */
GType esdashboard_theme_animation_get_type(void) G_GNUC_CONST;

EsdashboardThemeAnimation* esdashboard_theme_animation_new(void);

gboolean esdashboard_theme_animation_add_file(EsdashboardThemeAnimation *self,
											const gchar *inPath,
											GError **outError);

EsdashboardAnimation* esdashboard_theme_animation_create(EsdashboardThemeAnimation *self,
															EsdashboardActor *inSender,
															const gchar *inSignal,
															EsdashboardAnimationValue **inDefaultInitialValues,
															EsdashboardAnimationValue **inDefaultFinalValuess);

EsdashboardAnimation* esdashboard_theme_animation_create_by_id(EsdashboardThemeAnimation *self,
																EsdashboardActor *inSender,
																const gchar *inID,
																EsdashboardAnimationValue **inDefaultInitialValues,
																EsdashboardAnimationValue **inDefaultFinalValues);

gchar* esdashboard_theme_animation_lookup_id(EsdashboardThemeAnimation *self,
															EsdashboardActor *inSender,
															const gchar *inSignal);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_THEME_ANIMATION__ */
