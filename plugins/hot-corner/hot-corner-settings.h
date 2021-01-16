/*
 * hot-corner-settings: Shared object instance holding settings for plugin
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

#ifndef __ESDASHBOARD_HOT_CORNER_SETTINGS__
#define __ESDASHBOARD_HOT_CORNER_SETTINGS__

#include <libesdashboard/libesdashboard.h>

G_BEGIN_DECLS

/* Public definitions */
typedef enum /*< prefix=ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER >*/
{
	ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT=0,
	ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_RIGHT,
	ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_LEFT,
	ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_RIGHT,
} EsdashboardHotCornerSettingsActivationCorner;

GType esdashboard_hot_corner_settings_activation_corner_get_type(void) G_GNUC_CONST;
#define ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS_ACTIVATION_CORNER	(esdashboard_hot_corner_settings_activation_corner_get_type())


/* Object declaration */
#define ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS			(esdashboard_hot_corner_settings_get_type())
#define ESDASHBOARD_HOT_CORNER_SETTINGS(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS, EsdashboardHotCornerSettings))
#define ESDASHBOARD_IS_HOT_CORNER_SETTINGS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS))
#define ESDASHBOARD_HOT_CORNER_SETTINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS, EsdashboardHotCornerSettingsClass))
#define ESDASHBOARD_IS_HOT_CORNER_SETTINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS))
#define ESDASHBOARD_HOT_CORNER_SETTINGS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS, EsdashboardHotCornerSettingsClass))

typedef struct _EsdashboardHotCornerSettings			EsdashboardHotCornerSettings; 
typedef struct _EsdashboardHotCornerSettingsPrivate		EsdashboardHotCornerSettingsPrivate;
typedef struct _EsdashboardHotCornerSettingsClass		EsdashboardHotCornerSettingsClass;

struct _EsdashboardHotCornerSettings
{
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardHotCornerSettingsPrivate	*priv;
};

struct _EsdashboardHotCornerSettingsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;
};

/* Public API */
GType esdashboard_hot_corner_settings_get_type(void) G_GNUC_CONST;

ESDASHBOARD_DECLARE_PLUGIN_TYPE(esdashboard_hot_corner_settings);

EsdashboardHotCornerSettings* esdashboard_hot_corner_settings_new(void);

EsdashboardHotCornerSettingsActivationCorner esdashboard_hot_corner_settings_get_activation_corner(EsdashboardHotCornerSettings *self);
void esdashboard_hot_corner_settings_set_activation_corner(EsdashboardHotCornerSettings *self, const EsdashboardHotCornerSettingsActivationCorner inCorner);

gint esdashboard_hot_corner_settings_get_activation_radius(EsdashboardHotCornerSettings *self);
void esdashboard_hot_corner_settings_set_activation_radius(EsdashboardHotCornerSettings *self, gint inRadius);

gint64 esdashboard_hot_corner_settings_get_activation_duration(EsdashboardHotCornerSettings *self);
void esdashboard_hot_corner_settings_set_activation_duration(EsdashboardHotCornerSettings *self, gint64 inDuration);

gboolean esdashboard_hot_corner_settings_get_primary_monitor_only(EsdashboardHotCornerSettings *self);
void esdashboard_hot_corner_settings_set_primary_monitor_only(EsdashboardHotCornerSettings *self, gboolean inPrimaryOnly);

G_END_DECLS

#endif
