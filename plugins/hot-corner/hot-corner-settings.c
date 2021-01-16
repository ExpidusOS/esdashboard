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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hot-corner-settings.h"

#include <libesdashboard/libesdashboard.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>


/* Define this class in GObject system */
struct _EsdashboardHotCornerSettingsPrivate
{
	/* Properties related */
	EsdashboardHotCornerSettingsActivationCorner	activationCorner;
	gint											activationRadius;
	gint64											activationDuration;
	gboolean										primaryMonitorOnly;

	/* Instance related */
	EsconfChannel									*esconfChannel;
	guint											esconfActivationCornerBindingID;
	guint											esconfActivationRadiusBindingID;
	guint											esconfActivationDurationBindingID;
	guint											esconfPrimaryMonitorOnlyBindingID;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(EsdashboardHotCornerSettings,
								esdashboard_hot_corner_settings,
								G_TYPE_OBJECT,
								0,
								G_ADD_PRIVATE_DYNAMIC(EsdashboardHotCornerSettings))

/* Define this class in this plugin */
ESDASHBOARD_DEFINE_PLUGIN_TYPE(esdashboard_hot_corner_settings);

/* Properties */
enum
{
	PROP_0,

	PROP_ACTIVATION_CORNER,
	PROP_ACTIVATION_RADIUS,
	PROP_ACTIVATION_DURATION,
	PROP_PRIMARY_MONITOR_ONLY,

	PROP_LAST
};

static GParamSpec* EsdashboardHotCornerSettingsProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Enum ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS_ACTIVATION_CORNER */

GType esdashboard_hot_corner_settings_activation_corner_get_type(void)
{
	static volatile gsize	g_define_type_id__volatile=0;

	if(g_once_init_enter(&g_define_type_id__volatile))
	{
		static const GEnumValue values[]=
		{
			{ ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT, "ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT", "top-left" },
			{ ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_RIGHT, "ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_RIGHT", "top-right" },
			{ ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_LEFT, "ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_LEFT", "bottom-left" },
			{ ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_RIGHT, "ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_RIGHT", "bottom-right" },
			{ 0, NULL, NULL }
		};

		GType	g_define_type_id=g_enum_register_static(g_intern_static_string("EsdashboardHotCornerSettingsActivationCorner"), values);
		g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
	}

	return(g_define_type_id__volatile);
}


/* IMPLEMENTATION: Private variables and methods */
#define POLL_POINTER_POSITION_INTERVAL			100

#define ESDASHBOARD_ESCONF_CHANNEL				"esdashboard"

#define ACTIVATION_CORNER_ESCONF_PROP			"/plugins/"PLUGIN_ID"/activation-corner"
#define DEFAULT_ACTIVATION_CORNER				ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT

#define ACTIVATION_RADIUS_ESCONF_PROP			"/plugins/"PLUGIN_ID"/activation-radius"
#define DEFAULT_ACTIVATION_RADIUS				20

#define ACTIVATION_DURATION_ESCONF_PROP			"/plugins/"PLUGIN_ID"/activation-duration"
#define DEFAULT_ACTIVATION_DURATION				300

#define PRIMARY_MONITOR_ONLY_ESCONF_PROP		"/plugins/"PLUGIN_ID"/primary-monitor-only"
#define DEFAULT_PRIMARY_MONITOR_ONLY			TRUE


typedef struct _EsdashboardHotCornerSettingsBox		EsdashboardHotCornerSettingsBox;
struct _EsdashboardHotCornerSettingsBox
{
	gint		x1, y1;
	gint		x2, y2;
};


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_hot_corner_settings_dispose(GObject *inObject)
{
	EsdashboardHotCornerSettings			*self=ESDASHBOARD_HOT_CORNER_SETTINGS(inObject);
	EsdashboardHotCornerSettingsPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->esconfActivationCornerBindingID)
	{
		esconf_g_property_unbind(priv->esconfActivationCornerBindingID);
		priv->esconfActivationCornerBindingID=0;
	}

	if(priv->esconfActivationRadiusBindingID)
	{
		esconf_g_property_unbind(priv->esconfActivationRadiusBindingID);
		priv->esconfActivationRadiusBindingID=0;
	}

	if(priv->esconfActivationDurationBindingID)
	{
		esconf_g_property_unbind(priv->esconfActivationDurationBindingID);
		priv->esconfActivationDurationBindingID=0;
	}

	if(priv->esconfPrimaryMonitorOnlyBindingID)
	{
		esconf_g_property_unbind(priv->esconfPrimaryMonitorOnlyBindingID);
		priv->esconfPrimaryMonitorOnlyBindingID=0;
	}

	if(priv->esconfChannel)
	{
		priv->esconfChannel=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_hot_corner_settings_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_hot_corner_settings_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardHotCornerSettings			*self=ESDASHBOARD_HOT_CORNER_SETTINGS(inObject);

	switch(inPropID)
	{
		case PROP_ACTIVATION_CORNER:
			esdashboard_hot_corner_settings_set_activation_corner(self, g_value_get_enum(inValue));
			break;

		case PROP_ACTIVATION_RADIUS:
			esdashboard_hot_corner_settings_set_activation_radius(self, g_value_get_int(inValue));
			break;

		case PROP_ACTIVATION_DURATION:
			esdashboard_hot_corner_settings_set_activation_duration(self, g_value_get_uint64(inValue));
			break;

		case PROP_PRIMARY_MONITOR_ONLY:
			esdashboard_hot_corner_settings_set_primary_monitor_only(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_hot_corner_settings_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardHotCornerSettings			*self=ESDASHBOARD_HOT_CORNER_SETTINGS(inObject);
	EsdashboardHotCornerSettingsPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_ACTIVATION_CORNER:
			g_value_set_enum(outValue, priv->activationCorner);
			break;

		case PROP_ACTIVATION_RADIUS:
			g_value_set_int(outValue, priv->activationRadius);
			break;

		case PROP_ACTIVATION_DURATION:
			g_value_set_uint64(outValue, priv->activationDuration);
			break;
		case PROP_PRIMARY_MONITOR_ONLY:
			g_value_set_boolean(outValue, priv->primaryMonitorOnly);
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
void esdashboard_hot_corner_settings_class_init(EsdashboardHotCornerSettingsClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_hot_corner_settings_dispose;
	gobjectClass->set_property=_esdashboard_hot_corner_settings_set_property;
	gobjectClass->get_property=_esdashboard_hot_corner_settings_get_property;

	/* Define properties */
	EsdashboardHotCornerSettingsProperties[PROP_ACTIVATION_CORNER]=
		g_param_spec_enum("activation-corner",
							"Activation corner",
							"The hot corner where to trigger the application to suspend or to resume",
							ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS_ACTIVATION_CORNER,
							DEFAULT_ACTIVATION_CORNER,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardHotCornerSettingsProperties[PROP_ACTIVATION_RADIUS]=
		g_param_spec_int("activation-radius",
							"Activation radius",
							"The radius around hot corner where the pointer must be inside",
							0, G_MAXINT,
							DEFAULT_ACTIVATION_RADIUS,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardHotCornerSettingsProperties[PROP_ACTIVATION_DURATION]=
		g_param_spec_uint64("activation-duration",
							"Activation duration",
							"The time in milliseconds the pointer must stay inside the radius at hot corner to trigger",
							0, G_MAXUINT64,
							DEFAULT_ACTIVATION_DURATION,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardHotCornerSettingsProperties[PROP_PRIMARY_MONITOR_ONLY]=
		g_param_spec_boolean("primary-monitor-only",
								"Primary monitor only",
								"A flag indicating if all monitors or only the primary one should be check for hot corner",
								DEFAULT_PRIMARY_MONITOR_ONLY,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardHotCornerSettingsProperties);
}

/* Class finalization */
void esdashboard_hot_corner_settings_class_finalize(EsdashboardHotCornerSettingsClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_hot_corner_settings_init(EsdashboardHotCornerSettings *self)
{
	EsdashboardHotCornerSettingsPrivate		*priv;

	self->priv=priv=esdashboard_hot_corner_settings_get_instance_private(self);

	/* Set up default values */
	priv->activationCorner=DEFAULT_ACTIVATION_CORNER;
	priv->activationRadius=DEFAULT_ACTIVATION_RADIUS;
	priv->activationDuration=DEFAULT_ACTIVATION_DURATION;
	priv->primaryMonitorOnly=DEFAULT_PRIMARY_MONITOR_ONLY;
	priv->esconfChannel=esconf_channel_get(ESDASHBOARD_ESCONF_CHANNEL);

	/* Bind to esconf to react on changes */
	priv->esconfActivationCornerBindingID=
		esconf_g_property_bind(priv->esconfChannel,
								ACTIVATION_CORNER_ESCONF_PROP,
								G_TYPE_STRING,
								self,
								"activation-corner");

	priv->esconfActivationRadiusBindingID=
		esconf_g_property_bind(priv->esconfChannel,
								ACTIVATION_RADIUS_ESCONF_PROP,
								G_TYPE_INT,
								self,
								"activation-radius");

	priv->esconfActivationDurationBindingID=
		esconf_g_property_bind(priv->esconfChannel,
								ACTIVATION_DURATION_ESCONF_PROP,
								G_TYPE_INT64,
								self,
								"activation-duration");

	priv->esconfPrimaryMonitorOnlyBindingID=
		esconf_g_property_bind(priv->esconfChannel,
								PRIMARY_MONITOR_ONLY_ESCONF_PROP,
								G_TYPE_BOOLEAN,
								self,
								"primary-monitor-only");
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
EsdashboardHotCornerSettings* esdashboard_hot_corner_settings_new(void)
{
	GObject		*hotCorner;

	hotCorner=g_object_new(ESDASHBOARD_TYPE_HOT_CORNER_SETTINGS, NULL);
	if(!hotCorner) return(NULL);

	return(ESDASHBOARD_HOT_CORNER_SETTINGS(hotCorner));
}

/* Get/set hot corner */
EsdashboardHotCornerSettingsActivationCorner esdashboard_hot_corner_settings_get_activation_corner(EsdashboardHotCornerSettings *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_HOT_CORNER_SETTINGS(self), ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT);

	return(self->priv->activationCorner);
}

void esdashboard_hot_corner_settings_set_activation_corner(EsdashboardHotCornerSettings *self, EsdashboardHotCornerSettingsActivationCorner inCorner)
{
	EsdashboardHotCornerSettingsPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_HOT_CORNER_SETTINGS(self));
	g_return_if_fail(inCorner<=ESDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_RIGHT);

	priv=self->priv;

	/* Set value if changed */
	if(priv->activationCorner!=inCorner)
	{
		/* Set value */
		priv->activationCorner=inCorner;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardHotCornerSettingsProperties[PROP_ACTIVATION_CORNER]);
	}
}

/* Get/set radius around hot corner */
gint esdashboard_hot_corner_settings_get_activation_radius(EsdashboardHotCornerSettings *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_HOT_CORNER_SETTINGS(self), 0);

	return(self->priv->activationRadius);
}

void esdashboard_hot_corner_settings_set_activation_radius(EsdashboardHotCornerSettings *self, gint inRadius)
{
	EsdashboardHotCornerSettingsPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_HOT_CORNER_SETTINGS(self));
	g_return_if_fail(inRadius>0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->activationRadius!=inRadius)
	{
		/* Set value */
		priv->activationRadius=inRadius;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardHotCornerSettingsProperties[PROP_ACTIVATION_RADIUS]);
	}
}

/* Get/set duration when to trigger hot corner */
gint64 esdashboard_hot_corner_settings_get_activation_duration(EsdashboardHotCornerSettings *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_HOT_CORNER_SETTINGS(self), 0);

	return(self->priv->activationDuration);
}

void esdashboard_hot_corner_settings_set_activation_duration(EsdashboardHotCornerSettings *self, gint64 inDuration)
{
	EsdashboardHotCornerSettingsPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_HOT_CORNER_SETTINGS(self));
	g_return_if_fail(inDuration>0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->activationDuration!=inDuration)
	{
		/* Set value */
		priv->activationDuration=inDuration;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardHotCornerSettingsProperties[PROP_ACTIVATION_DURATION]);
	}
}

/* Get/set flag to check primary monitor only if hot corner was entered */
gboolean esdashboard_hot_corner_settings_get_primary_monitor_only(EsdashboardHotCornerSettings *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_HOT_CORNER_SETTINGS(self), 0);

	return(self->priv->primaryMonitorOnly);
}

void esdashboard_hot_corner_settings_set_primary_monitor_only(EsdashboardHotCornerSettings *self, gboolean inPrimaryOnly)
{
	EsdashboardHotCornerSettingsPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_HOT_CORNER_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->primaryMonitorOnly!=inPrimaryOnly)
	{
		/* Set value */
		priv->primaryMonitorOnly=inPrimaryOnly;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardHotCornerSettingsProperties[PROP_PRIMARY_MONITOR_ONLY]);
	}
}
