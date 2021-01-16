/*
 * themes: Theme settings of application
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

#ifndef __ESDASHBOARD_SETTINGS_THEMES__
#define __ESDASHBOARD_SETTINGS_THEMES__

#include <gtk/gtkx.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SETTINGS_THEMES				(esdashboard_settings_themes_get_type())
#define ESDASHBOARD_SETTINGS_THEMES(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SETTINGS_THEMES, EsdashboardSettingsThemes))
#define ESDASHBOARD_IS_SETTINGS_THEMES(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SETTINGS_THEMES))
#define ESDASHBOARD_SETTINGS_THEMES_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SETTINGS_THEMES, EsdashboardSettingsThemesClass))
#define ESDASHBOARD_IS_SETTINGS_THEMES_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SETTINGS_THEMES))
#define ESDASHBOARD_SETTINGS_THEMES_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SETTINGS_THEMES, EsdashboardSettingsThemesClass))

typedef struct _EsdashboardSettingsThemes				EsdashboardSettingsThemes;
typedef struct _EsdashboardSettingsThemesClass			EsdashboardSettingsThemesClass;
typedef struct _EsdashboardSettingsThemesPrivate		EsdashboardSettingsThemesPrivate;

struct _EsdashboardSettingsThemes
{
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	EsdashboardSettingsThemesPrivate	*priv;
};

struct _EsdashboardSettingsThemesClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_settings_themes_get_type(void) G_GNUC_CONST;

EsdashboardSettingsThemes* esdashboard_settings_themes_new(GtkBuilder *inBuilder);

G_END_DECLS

#endif	/* __ESDASHBOARD_SETTINGS_THEMES__ */
