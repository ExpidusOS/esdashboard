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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clock-view-settings.h"

#include <libesdashboard/libesdashboard.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>


/* Define this class in GObject system */
struct _EsdashboardClockViewSettingsPrivate
{
	/* Properties related */
	ClutterColor			*hourColor;
	ClutterColor			*minuteColor;
	ClutterColor			*secondColor;
	ClutterColor			*backgroundColor;

	/* Instance related */
	EsconfChannel			*esconfChannel;
	guint					esconfHourColorBindingID;
	guint					esconfMinuteColorBindingID;
	guint					esconfSecondColorBindingID;
	guint					esconfBackgroundColorBindingID;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(EsdashboardClockViewSettings,
								esdashboard_clock_view_settings,
								G_TYPE_OBJECT,
								0,
								G_ADD_PRIVATE_DYNAMIC(EsdashboardClockViewSettings))

/* Define this class in this plugin */
ESDASHBOARD_DEFINE_PLUGIN_TYPE(esdashboard_clock_view_settings);

/* Properties */
enum
{
	PROP_0,

	PROP_HOUR_COLOR,
	PROP_MINUTE_COLOR,
	PROP_SECOND_COLOR,
	PROP_BACKGROUOND_COLOR,

	PROP_LAST
};

static GParamSpec* EsdashboardClockViewSettingsProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

#define ESDASHBOARD_ESCONF_CHANNEL		"esdashboard"

#define COLOR_HOUR_ESCONF_PROP			"/plugins/"PLUGIN_ID"/hour-color"
#define COLOR_MINUTE_ESCONF_PROP		"/plugins/"PLUGIN_ID"/minute-color"
#define COLOR_SECOND_ESCONF_PROP		"/plugins/"PLUGIN_ID"/second-color"
#define COLOR_BACKGROUND_ESCONF_PROP	"/plugins/"PLUGIN_ID"/background-color"


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_clock_view_settings_dispose(GObject *inObject)
{
	EsdashboardClockViewSettings			*self=ESDASHBOARD_CLOCK_VIEW_SETTINGS(inObject);
	EsdashboardClockViewSettingsPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->esconfHourColorBindingID)
	{
		esconf_g_property_unbind(priv->esconfHourColorBindingID);
		priv->esconfHourColorBindingID=0;
	}

	if(priv->esconfMinuteColorBindingID)
	{
		esconf_g_property_unbind(priv->esconfMinuteColorBindingID);
		priv->esconfMinuteColorBindingID=0;
	}

	if(priv->esconfSecondColorBindingID)
	{
		esconf_g_property_unbind(priv->esconfSecondColorBindingID);
		priv->esconfSecondColorBindingID=0;
	}

	if(priv->esconfBackgroundColorBindingID)
	{
		esconf_g_property_unbind(priv->esconfBackgroundColorBindingID);
		priv->esconfBackgroundColorBindingID=0;
	}

	if(priv->esconfChannel)
	{
		priv->esconfChannel=NULL;
	}

	if(priv->hourColor)
	{
		clutter_color_free(priv->hourColor);
		priv->hourColor=NULL;
	}

	if(priv->minuteColor)
	{
		clutter_color_free(priv->minuteColor);
		priv->minuteColor=NULL;
	}

	if(priv->secondColor)
	{
		clutter_color_free(priv->secondColor);
		priv->secondColor=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_clock_view_settings_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_clock_view_settings_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	EsdashboardClockViewSettings			*self=ESDASHBOARD_CLOCK_VIEW_SETTINGS(inObject);

	switch(inPropID)
	{
		case PROP_HOUR_COLOR:
			esdashboard_clock_view_settings_set_hour_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_MINUTE_COLOR:
			esdashboard_clock_view_settings_set_minute_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_SECOND_COLOR:
			esdashboard_clock_view_settings_set_second_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_BACKGROUOND_COLOR:
			esdashboard_clock_view_settings_set_background_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_clock_view_settings_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	EsdashboardClockViewSettings			*self=ESDASHBOARD_CLOCK_VIEW_SETTINGS(inObject);
	EsdashboardClockViewSettingsPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_HOUR_COLOR:
			clutter_value_set_color(outValue, priv->hourColor);
			break;

		case PROP_MINUTE_COLOR:
			clutter_value_set_color(outValue, priv->minuteColor);
			break;

		case PROP_SECOND_COLOR:
			clutter_value_set_color(outValue, priv->secondColor);
			break;

		case PROP_BACKGROUOND_COLOR:
			clutter_value_set_color(outValue, priv->backgroundColor);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_clock_view_settings_class_init(EsdashboardClockViewSettingsClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_clock_view_settings_dispose;
	gobjectClass->set_property=_esdashboard_clock_view_settings_set_property;
	gobjectClass->get_property=_esdashboard_clock_view_settings_get_property;

	/* Define properties */
	EsdashboardClockViewSettingsProperties[PROP_HOUR_COLOR]=
		clutter_param_spec_color("hour-color",
									"Hour color",
									"Color to draw the hour hand with",
									CLUTTER_COLOR_LightChameleon,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardClockViewSettingsProperties[PROP_MINUTE_COLOR]=
		clutter_param_spec_color("minute-color",
									"Minute color",
									"Color to draw the minute hand with",
									CLUTTER_COLOR_LightChameleon,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardClockViewSettingsProperties[PROP_SECOND_COLOR]=
		clutter_param_spec_color("second-color",
									"Second color",
									"Color to draw the second hand with",
									CLUTTER_COLOR_White,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardClockViewSettingsProperties[PROP_BACKGROUOND_COLOR]=
		clutter_param_spec_color("background-color",
									"Background color",
									"Color to draw the circle with that holds the second hand",
									CLUTTER_COLOR_Blue,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardClockViewSettingsProperties);
}

/* Class finalization */
void esdashboard_clock_view_settings_class_finalize(EsdashboardClockViewSettingsClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_clock_view_settings_init(EsdashboardClockViewSettings *self)
{
	EsdashboardClockViewSettingsPrivate		*priv;

	self->priv=priv=esdashboard_clock_view_settings_get_instance_private(self);

	/* Set up default values */
	priv->hourColor=clutter_color_copy(CLUTTER_COLOR_LightChameleon);
	priv->minuteColor=clutter_color_copy(CLUTTER_COLOR_LightChameleon);
	priv->secondColor=clutter_color_copy(CLUTTER_COLOR_White);
	priv->backgroundColor=clutter_color_copy(CLUTTER_COLOR_Blue);
	priv->esconfChannel=esconf_channel_get(ESDASHBOARD_ESCONF_CHANNEL);

	/* Bind to esconf to react on changes */
	priv->esconfHourColorBindingID=
		esconf_g_property_bind(priv->esconfChannel,
								COLOR_HOUR_ESCONF_PROP,
								G_TYPE_STRING,
								self,
								"hour-color");

	priv->esconfMinuteColorBindingID=
		esconf_g_property_bind(priv->esconfChannel,
								COLOR_MINUTE_ESCONF_PROP,
								G_TYPE_STRING,
								self,
								"minute-color");

	priv->esconfSecondColorBindingID=
		esconf_g_property_bind(priv->esconfChannel,
								COLOR_SECOND_ESCONF_PROP,
								G_TYPE_STRING,
								self,
								"second-color");

	priv->esconfBackgroundColorBindingID=
		esconf_g_property_bind(priv->esconfChannel,
								COLOR_BACKGROUND_ESCONF_PROP,
								G_TYPE_STRING,
								self,
								"background-color");
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
EsdashboardClockViewSettings* esdashboard_clock_view_settings_new(void)
{
	return(ESDASHBOARD_CLOCK_VIEW_SETTINGS(g_object_new(ESDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS, NULL)));	
}

/* Get/set color to draw hour hand with */
const ClutterColor* esdashboard_clock_view_settings_get_hour_color(EsdashboardClockViewSettings *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self), NULL);

	return(self->priv->hourColor);
}

void esdashboard_clock_view_settings_set_hour_color(EsdashboardClockViewSettings *self, const ClutterColor *inColor)
{
	EsdashboardClockViewSettingsPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->hourColor==NULL ||
		!clutter_color_equal(inColor, priv->hourColor))
	{
		/* Set value */
		if(priv->hourColor) clutter_color_free(priv->hourColor);
		priv->hourColor=clutter_color_copy(inColor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardClockViewSettingsProperties[PROP_HOUR_COLOR]);
	}
}

/* Get/set color to draw minute hand with */
const ClutterColor* esdashboard_clock_view_settings_get_minute_color(EsdashboardClockViewSettings *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self), NULL);

	return(self->priv->minuteColor);
}

void esdashboard_clock_view_settings_set_minute_color(EsdashboardClockViewSettings *self, const ClutterColor *inColor)
{
	EsdashboardClockViewSettingsPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->minuteColor==NULL ||
		!clutter_color_equal(inColor, priv->minuteColor))
	{
		/* Set value */
		if(priv->minuteColor) clutter_color_free(priv->minuteColor);
		priv->minuteColor=clutter_color_copy(inColor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardClockViewSettingsProperties[PROP_MINUTE_COLOR]);
	}
}

/* Get/set color to draw second hand with */
const ClutterColor* esdashboard_clock_view_settings_get_second_color(EsdashboardClockViewSettings *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self), NULL);

	return(self->priv->secondColor);
}

void esdashboard_clock_view_settings_set_second_color(EsdashboardClockViewSettings *self, const ClutterColor *inColor)
{
	EsdashboardClockViewSettingsPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->secondColor==NULL ||
		!clutter_color_equal(inColor, priv->secondColor))
	{
		/* Set value */
		if(priv->secondColor) clutter_color_free(priv->secondColor);
		priv->secondColor=clutter_color_copy(inColor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardClockViewSettingsProperties[PROP_SECOND_COLOR]);
	}
}

/* Get/set color to draw background with that holds second hand */
const ClutterColor* esdashboard_clock_view_settings_get_background_color(EsdashboardClockViewSettings *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self), NULL);

	return(self->priv->backgroundColor);
}

void esdashboard_clock_view_settings_set_background_color(EsdashboardClockViewSettings *self, const ClutterColor *inColor)
{
	EsdashboardClockViewSettingsPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->backgroundColor==NULL ||
		!clutter_color_equal(inColor, priv->backgroundColor))
	{
		/* Set value */
		if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=clutter_color_copy(inColor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardClockViewSettingsProperties[PROP_BACKGROUOND_COLOR]);
	}
}
