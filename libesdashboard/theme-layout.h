/*
 * theme-layout: A theme used for build and layout objects by XML files
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

#ifndef __LIBESDASHBOARD_THEME_LAYOUT__
#define __LIBESDASHBOARD_THEME_LAYOUT__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * EsdashboardThemeLayoutBuildGet:
 * @ESDASHBOARD_THEME_LAYOUT_BUILD_GET_FOCUSABLES: Get #GPtrArray of pointer to defined focusable actors. Caller must free returned #GPtrArray with g_ptr_array_unref().
 * @ESDASHBOARD_THEME_LAYOUT_BUILD_GET_SELECTED_FOCUS: Get #ClutterActor which should gain the focus. Caller must unref returned #ClutterActor with g_object_unref().
 *
 * The extra data to fetch when building an object from theme layout.
 */
typedef enum /*< prefix=ESDASHBOARD_THEME_LAYOUT_BUILD_GET >*/
{
	ESDASHBOARD_THEME_LAYOUT_BUILD_GET_FOCUSABLES=0,
	ESDASHBOARD_THEME_LAYOUT_BUILD_GET_SELECTED_FOCUS
} EsdashboardThemeLayoutBuildGet;

/* Object declaration */
#define ESDASHBOARD_TYPE_THEME_LAYOUT				(esdashboard_theme_layout_get_type())
#define ESDASHBOARD_THEME_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_THEME_LAYOUT, EsdashboardThemeLayout))
#define ESDASHBOARD_IS_THEME_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_THEME_LAYOUT))
#define ESDASHBOARD_THEME_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_THEME_LAYOUT, EsdashboardThemeLayoutClass))
#define ESDASHBOARD_IS_THEME_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_THEME_LAYOUT))
#define ESDASHBOARD_THEME_LAYOUT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_THEME_LAYOUT, EsdashboardThemeLayoutClass))

typedef struct _EsdashboardThemeLayout				EsdashboardThemeLayout;
typedef struct _EsdashboardThemeLayoutClass			EsdashboardThemeLayoutClass;
typedef struct _EsdashboardThemeLayoutPrivate		EsdashboardThemeLayoutPrivate;

/**
 * EsdashboardThemeLayout:
 *
 * The #EsdashboardThemeLayout structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardThemeLayout
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardThemeLayoutPrivate		*priv;
};

/**
 * EsdashboardThemeLayoutClass:
 *
 * The #EsdashboardThemeLayoutClass structure contains only private data
 */
struct _EsdashboardThemeLayoutClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define ESDASHBOARD_THEME_LAYOUT_ERROR				(esdashboard_theme_layout_error_quark())

GQuark esdashboard_theme_layout_error_quark(void);

typedef enum /*< prefix=ESDASHBOARD_THEME_LAYOUT_ERROR >*/
{
	ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
	ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
} EsdashboardThemeLayoutErrorEnum;

/* Public API */
GType esdashboard_theme_layout_get_type(void) G_GNUC_CONST;

EsdashboardThemeLayout* esdashboard_theme_layout_new(void);

gboolean esdashboard_theme_layout_add_file(EsdashboardThemeLayout *self,
											const gchar *inPath,
											GError **outError);

ClutterActor* esdashboard_theme_layout_build_interface(EsdashboardThemeLayout *self,
														const gchar *inID,
														...);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_THEME_LAYOUT__ */
