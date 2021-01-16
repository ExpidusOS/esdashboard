/*
 * plugins: Plugin settings of application
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

#ifndef __ESDASHBOARD_SETTINGS_PLUGINS__
#define __ESDASHBOARD_SETTINGS_PLUGINS__

#include <gtk/gtkx.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SETTINGS_PLUGINS				(esdashboard_settings_plugins_get_type())
#define ESDASHBOARD_SETTINGS_PLUGINS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SETTINGS_PLUGINS, EsdashboardSettingsPlugins))
#define ESDASHBOARD_IS_SETTINGS_PLUGINS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SETTINGS_PLUGINS))
#define ESDASHBOARD_SETTINGS_PLUGINS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SETTINGS_PLUGINS, EsdashboardSettingsPluginsClass))
#define ESDASHBOARD_IS_SETTINGS_PLUGINS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SETTINGS_PLUGINS))
#define ESDASHBOARD_SETTINGS_PLUGINS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SETTINGS_PLUGINS, EsdashboardSettingsPluginsClass))

typedef struct _EsdashboardSettingsPlugins				EsdashboardSettingsPlugins;
typedef struct _EsdashboardSettingsPluginsClass			EsdashboardSettingsPluginsClass;
typedef struct _EsdashboardSettingsPluginsPrivate		EsdashboardSettingsPluginsPrivate;

struct _EsdashboardSettingsPlugins
{
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardSettingsPluginsPrivate	*priv;
};

struct _EsdashboardSettingsPluginsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_settings_plugins_get_type(void) G_GNUC_CONST;

EsdashboardSettingsPlugins* esdashboard_settings_plugins_new(GtkBuilder *inBuilder);

G_END_DECLS

#endif	/* __ESDASHBOARD_SETTINGS_PLUGINS__ */
