/*
 * theme: Top-level theme object (parses key file and manages loading
 *        resources like css style files, xml layout files etc.)
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

#ifndef __LIBESDASHBOARD_THEME__
#define __LIBESDASHBOARD_THEME__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/theme-css.h>
#include <libesdashboard/theme-layout.h>
#include <libesdashboard/theme-effects.h>
#include <libesdashboard/theme-animation.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_THEME					(esdashboard_theme_get_type())
#define ESDASHBOARD_THEME(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_THEME, EsdashboardTheme))
#define ESDASHBOARD_IS_THEME(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_THEME))
#define ESDASHBOARD_THEME_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_THEME, EsdashboardThemeClass))
#define ESDASHBOARD_IS_THEME_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_THEME))
#define ESDASHBOARD_THEME_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_THEME, EsdashboardThemeClass))

typedef struct _EsdashboardTheme				EsdashboardTheme;
typedef struct _EsdashboardThemeClass			EsdashboardThemeClass;
typedef struct _EsdashboardThemePrivate			EsdashboardThemePrivate;

struct _EsdashboardTheme
{
	/*< private >*/
	/* Parent instance */
	GObject						parent_instance;

	/* Private structure */
	EsdashboardThemePrivate		*priv;
};

struct _EsdashboardThemeClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass				parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define ESDASHBOARD_THEME_ERROR					(esdashboard_theme_error_quark())

GQuark esdashboard_theme_error_quark(void);

typedef enum /*< prefix=ESDASHBOARD_THEME_ERROR >*/
{
	ESDASHBOARD_THEME_ERROR_THEME_NOT_FOUND,
	ESDASHBOARD_THEME_ERROR_ALREADY_LOADED
} EsdashboardThemeErrorEnum;

/* Public API */
GType esdashboard_theme_get_type(void) G_GNUC_CONST;

EsdashboardTheme* esdashboard_theme_new(const gchar *inThemeName);

const gchar* esdashboard_theme_get_path(EsdashboardTheme *self);

const gchar* esdashboard_theme_get_theme_name(EsdashboardTheme *self);
const gchar* esdashboard_theme_get_display_name(EsdashboardTheme *self);
const gchar* esdashboard_theme_get_comment(EsdashboardTheme *self);

gboolean esdashboard_theme_load(EsdashboardTheme *self,
								GError **outError);

EsdashboardThemeCSS* esdashboard_theme_get_css(EsdashboardTheme *self);
EsdashboardThemeLayout* esdashboard_theme_get_layout(EsdashboardTheme *self);
EsdashboardThemeEffects* esdashboard_theme_get_effects(EsdashboardTheme *self);
EsdashboardThemeAnimation* esdashboard_theme_get_animation(EsdashboardTheme *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_THEME__ */
