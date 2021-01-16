/*
 * theme-css: A theme used for rendering esdashboard actors with CSS.
 *            The parser and the handling of CSS files is heavily based
 *            on mx-css, mx-style and mx-stylable of library mx
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

#ifndef __LIBESDASHBOARD_THEME_CSS__
#define __LIBESDASHBOARD_THEME_CSS__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libesdashboard/stylable.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_THEME_CSS					(esdashboard_theme_css_get_type())
#define ESDASHBOARD_THEME_CSS(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_THEME_CSS, EsdashboardThemeCSS))
#define ESDASHBOARD_IS_THEME_CSS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_THEME_CSS))
#define ESDASHBOARD_THEME_CSS_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_THEME_CSS, EsdashboardThemeCSSClass))
#define ESDASHBOARD_IS_THEME_CSS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_THEME_CSS))
#define ESDASHBOARD_THEME_CSS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_THEME_CSS, EsdashboardThemeCSSClass))

typedef struct _EsdashboardThemeCSS					EsdashboardThemeCSS;
typedef struct _EsdashboardThemeCSSClass			EsdashboardThemeCSSClass;
typedef struct _EsdashboardThemeCSSPrivate			EsdashboardThemeCSSPrivate;

struct _EsdashboardThemeCSS
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	EsdashboardThemeCSSPrivate		*priv;
};

struct _EsdashboardThemeCSSClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define ESDASHBOARD_THEME_CSS_ERROR					(esdashboard_theme_css_error_quark())

GQuark esdashboard_theme_css_error_quark(void);

typedef enum /*< prefix=ESDASHBOARD_THEME_CSS_ERROR >*/
{
	ESDASHBOARD_THEME_CSS_ERROR_INVALID_ARGUMENT,
	ESDASHBOARD_THEME_CSS_ERROR_UNSUPPORTED_STREAM,
	ESDASHBOARD_THEME_CSS_ERROR_PARSER_ERROR,
	ESDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR
} EsdashboardThemeCSSErrorEnum;

/* Public declarations */
typedef struct _EsdashboardThemeCSSValue			EsdashboardThemeCSSValue;
struct _EsdashboardThemeCSSValue
{
	const gchar						*string;
	const gchar						*source;
};

/* Public API */
GType esdashboard_theme_css_get_type(void) G_GNUC_CONST;

EsdashboardThemeCSS* esdashboard_theme_css_new(const gchar *inThemePath);

gboolean esdashboard_theme_css_add_file(EsdashboardThemeCSS *self,
											const gchar *inPath,
											gint inPriority,
											GError **outError);

GHashTable* esdashboard_theme_css_get_properties(EsdashboardThemeCSS *self,
													EsdashboardStylable *inStylable);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_THEME_CSS__ */
