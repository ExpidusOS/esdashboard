/*
 * clock-view-settings: Shared object instance holding settings for plugin
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

#ifndef __ESDASHBOARD_CLOCK_VIEW_SETTINGS__
#define __ESDASHBOARD_CLOCK_VIEW_SETTINGS__

#include <libesdashboard/libesdashboard.h>
#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS				(esdashboard_clock_view_settings_get_type())
#define ESDASHBOARD_CLOCK_VIEW_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS, EsdashboardClockViewSettings))
#define ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS))
#define ESDASHBOARD_CLOCK_VIEW_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS, EsdashboardClockViewSettingsClass))
#define ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS))
#define ESDASHBOARD_CLOCK_VIEW_SETTINGS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS, EsdashboardClockViewSettingsClass))

typedef struct _EsdashboardClockViewSettings				EsdashboardClockViewSettings; 
typedef struct _EsdashboardClockViewSettingsPrivate			EsdashboardClockViewSettingsPrivate;
typedef struct _EsdashboardClockViewSettingsClass			EsdashboardClockViewSettingsClass;

struct _EsdashboardClockViewSettings
{
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	EsdashboardClockViewSettingsPrivate		*priv;
};

struct _EsdashboardClockViewSettingsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;
};

/* Public API */
GType esdashboard_clock_view_settings_get_type(void) G_GNUC_CONST;

EsdashboardClockViewSettings* esdashboard_clock_view_settings_new(void);

const ClutterColor* esdashboard_clock_view_settings_get_hour_color(EsdashboardClockViewSettings *self);
void esdashboard_clock_view_settings_set_hour_color(EsdashboardClockViewSettings *self, const ClutterColor *inColor);

const ClutterColor* esdashboard_clock_view_settings_get_minute_color(EsdashboardClockViewSettings *self);
void esdashboard_clock_view_settings_set_minute_color(EsdashboardClockViewSettings *self, const ClutterColor *inColor);

const ClutterColor* esdashboard_clock_view_settings_get_second_color(EsdashboardClockViewSettings *self);
void esdashboard_clock_view_settings_set_second_color(EsdashboardClockViewSettings *self, const ClutterColor *inColor);

const ClutterColor* esdashboard_clock_view_settings_get_background_color(EsdashboardClockViewSettings *self);
void esdashboard_clock_view_settings_set_background_color(EsdashboardClockViewSettings *self, const ClutterColor *inColor);

ESDASHBOARD_DECLARE_PLUGIN_TYPE(esdashboard_clock_view_settings);

G_END_DECLS

#endif
