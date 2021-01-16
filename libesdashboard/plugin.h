/*
 * plugin: A plugin class managing loading the shared object as well as
 *         initializing and setting up extensions to this application
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

#ifndef __LIBESDASHBOARD_PLUGIN__
#define __LIBESDASHBOARD_PLUGIN__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * EsdashboardPluginFlag:
 * @ESDASHBOARD_PLUGIN_FLAG_NONE: Plugin does not request anything special.
 * @ESDASHBOARD_PLUGIN_FLAG_EARLY_INITIALIZATION: Plugin requests to get enabled before the stage is initialized
 *
 * Flags defining behaviour of this EsdashboardPlugin.
 */
typedef enum /*< flags,prefix=ESDASHBOARD_PLUGIN_FLAG >*/
{
	ESDASHBOARD_PLUGIN_FLAG_NONE=0,

	ESDASHBOARD_PLUGIN_FLAG_EARLY_INITIALIZATION=1 << 0,
} EsdashboardPluginFlag;


/* Helper macros to declare, define and register GObject types in plugins */
#define ESDASHBOARD_DECLARE_PLUGIN_TYPE(inFunctionNamePrefix) \
	void inFunctionNamePrefix##_register_plugin_type(EsdashboardPlugin *inPlugin);

#define ESDASHBOARD_DEFINE_PLUGIN_TYPE(inFunctionNamePrefix) \
	void inFunctionNamePrefix##_register_plugin_type(EsdashboardPlugin *inPlugin) \
	{ \
		inFunctionNamePrefix##_register_type(G_TYPE_MODULE(inPlugin)); \
	}

#define ESDASHBOARD_REGISTER_PLUGIN_TYPE(self, inFunctionNamePrefix) \
	inFunctionNamePrefix##_register_plugin_type(ESDASHBOARD_PLUGIN(self));


/* Object declaration */
#define ESDASHBOARD_TYPE_PLUGIN				(esdashboard_plugin_get_type())
#define ESDASHBOARD_PLUGIN(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_PLUGIN, EsdashboardPlugin))
#define ESDASHBOARD_IS_PLUGIN(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_PLUGIN))
#define ESDASHBOARD_PLUGIN_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_PLUGIN, EsdashboardPluginClass))
#define ESDASHBOARD_IS_PLUGIN_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_PLUGIN))
#define ESDASHBOARD_PLUGIN_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_PLUGIN, EsdashboardPluginClass))

typedef struct _EsdashboardPlugin			EsdashboardPlugin;
typedef struct _EsdashboardPluginClass		EsdashboardPluginClass;
typedef struct _EsdashboardPluginPrivate	EsdashboardPluginPrivate;

struct _EsdashboardPlugin
{
	/*< private >*/
	/* Parent instance */
	GTypeModule						parent_instance;

	/* Private structure */
	EsdashboardPluginPrivate		*priv;
};

struct _EsdashboardPluginClass
{
	/*< private >*/
	/* Parent class */
	GTypeModuleClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*enable)(EsdashboardPlugin *self);
	void (*disable)(EsdashboardPlugin *self);

	GObject* (*configure)(EsdashboardPlugin *self);
};

/* Error */
#define ESDASHBOARD_PLUGIN_ERROR					(esdashboard_plugin_error_quark())

GQuark esdashboard_plugin_error_quark(void);

typedef enum /*< prefix=ESDASHBOARD_PLUGIN_ERROR >*/
{
	ESDASHBOARD_PLUGIN_ERROR_NONE,
	ESDASHBOARD_PLUGIN_ERROR_ERROR,
} EsdashboardPluginErrorEnum;

/* Public API */
GType esdashboard_plugin_get_type(void) G_GNUC_CONST;

EsdashboardPlugin* esdashboard_plugin_new(const gchar *inPluginFilename, GError **outError);

const gchar* esdashboard_plugin_get_id(EsdashboardPlugin *self);
EsdashboardPluginFlag esdashboard_plugin_get_flags(EsdashboardPlugin *self);

void esdashboard_plugin_set_info(EsdashboardPlugin *self,
									const gchar *inFirstPropertyName, ...)
									G_GNUC_NULL_TERMINATED;

gboolean esdashboard_plugin_is_enabled(EsdashboardPlugin *self);
void esdashboard_plugin_enable(EsdashboardPlugin *self);
void esdashboard_plugin_disable(EsdashboardPlugin *self);

const gchar* esdashboard_plugin_get_config_path(EsdashboardPlugin *self);
const gchar* esdashboard_plugin_get_cache_path(EsdashboardPlugin *self);
const gchar* esdashboard_plugin_get_data_path(EsdashboardPlugin *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_PLUGIN__ */
