/*
 * settings: Settings of application
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

#ifndef __ESDASHBOARD_SETTINGS__
#define __ESDASHBOARD_SETTINGS__

#include <gtk/gtkx.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SETTINGS				(esdashboard_settings_get_type())
#define ESDASHBOARD_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SETTINGS, EsdashboardSettings))
#define ESDASHBOARD_IS_SETTINGS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SETTINGS))
#define ESDASHBOARD_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SETTINGS, EsdashboardSettingsClass))
#define ESDASHBOARD_IS_SETTINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SETTINGS))
#define ESDASHBOARD_SETTINGS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SETTINGS, EsdashboardSettingsClass))

typedef struct _EsdashboardSettings				EsdashboardSettings;
typedef struct _EsdashboardSettingsClass		EsdashboardSettingsClass;
typedef struct _EsdashboardSettingsPrivate		EsdashboardSettingsPrivate;

struct _EsdashboardSettings
{
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	EsdashboardSettingsPrivate	*priv;
};

struct _EsdashboardSettingsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_settings_get_type(void) G_GNUC_CONST;

EsdashboardSettings* esdashboard_settings_new(void);

GtkWidget* esdashboard_settings_create_dialog(EsdashboardSettings *self);
GtkWidget* esdashboard_settings_create_plug(EsdashboardSettings *self, Window inSocketID);

G_END_DECLS

#endif	/* __ESDASHBOARD_SETTINGS__ */
