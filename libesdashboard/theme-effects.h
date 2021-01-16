/*
 * theme-effects: A theme used for build effects by XML files
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

#ifndef __LIBESDASHBOARD_THEME_EFFECTS__
#define __LIBESDASHBOARD_THEME_EFFECTS__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_THEME_EFFECTS				(esdashboard_theme_effects_get_type())
#define ESDASHBOARD_THEME_EFFECTS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_THEME_EFFECTS, EsdashboardThemeEffects))
#define ESDASHBOARD_IS_THEME_EFFECTS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_THEME_EFFECTS))
#define ESDASHBOARD_THEME_EFFECTS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_THEME_EFFECTS, EsdashboardThemeEffectsClass))
#define ESDASHBOARD_IS_THEME_EFFECTS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_THEME_EFFECTS))
#define ESDASHBOARD_THEME_EFFECTS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_THEME_EFFECTS, EsdashboardThemeEffectsClass))

typedef struct _EsdashboardThemeEffects				EsdashboardThemeEffects;
typedef struct _EsdashboardThemeEffectsClass		EsdashboardThemeEffectsClass;
typedef struct _EsdashboardThemeEffectsPrivate		EsdashboardThemeEffectsPrivate;

struct _EsdashboardThemeEffects
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardThemeEffectsPrivate		*priv;
};

struct _EsdashboardThemeEffectsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define ESDASHBOARD_THEME_EFFECTS_ERROR				(esdashboard_theme_effects_error_quark())

GQuark esdashboard_theme_effects_error_quark(void);

typedef enum /*< prefix=ESDASHBOARD_THEME_EFFECTS_ERROR >*/
{
	ESDASHBOARD_THEME_EFFECTS_ERROR_ERROR,
	ESDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
} EsdashboardThemeEffectsErrorEnum;

/* Public API */
GType esdashboard_theme_effects_get_type(void) G_GNUC_CONST;

EsdashboardThemeEffects* esdashboard_theme_effects_new(void);

gboolean esdashboard_theme_effects_add_file(EsdashboardThemeEffects *self,
											const gchar *inPath,
											GError **outError);

ClutterEffect* esdashboard_theme_effects_create_effect(EsdashboardThemeEffects *self,
														const gchar *inID);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_THEME_EFFECTS__ */
