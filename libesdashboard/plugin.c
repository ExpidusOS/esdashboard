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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/plugin.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/enums.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Forward declaration */
typedef enum /*< skip,prefix=ESDASHBOARD_PLUGIN_STATE >*/
{
	ESDASHBOARD_PLUGIN_STATE_NONE=0,
	ESDASHBOARD_PLUGIN_STATE_INITIALIZED,
	ESDASHBOARD_PLUGIN_STATE_ENABLED,
} EsdashboardPluginState;


/* Define this class in GObject system */
struct _EsdashboardPluginPrivate
{
	/* Properties related */
	gchar						*id;
	EsdashboardPluginFlag		flags;
	gchar						*name;
	gchar						*description;
	gchar						*author;
	gchar						*copyright;
	gchar						*license;

	gchar						*configPath;
	gchar						*cachePath;
	gchar						*dataPath;

	/* Instance related */
	gchar						*filename;
	GModule						*module;
	void 						(*initialize)(EsdashboardPlugin *self);
	EsdashboardPluginState		state;
	gchar						*lastLoadingError;

};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardPlugin,
							esdashboard_plugin,
							G_TYPE_TYPE_MODULE)

/* Properties */
enum
{
	PROP_0,

	PROP_FILENAME,

	PROP_ID,
	PROP_FLAGS,
	PROP_NAME,
	PROP_DESCRIPTION,
	PROP_AUTHOR,
	PROP_COPYRIGHT,
	PROP_LICENSE,

	PROP_CONFIG_PATH,
	PROP_CACHE_PATH,
	PROP_DATA_PATH,

	PROP_LAST
};

static GParamSpec* EsdashboardPluginProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	/* Actions */
	ACTION_ENABLE,
	ACTION_DISABLE,
	ACTION_CONFIGURE,

	SIGNAL_LAST
};

static guint EsdashboardPluginSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define ESDASHBOARD_PLUGIN_FUNCTION_NAME_INITIALIZE		"plugin_init"

/* Get display name for ESDASHBOARD_PLUGIN_STATE_* enum values */
static const gchar* _esdashboard_plugin_get_plugin_state_value_name(EsdashboardPluginState inState)
{
	g_return_val_if_fail(inState>ESDASHBOARD_PLUGIN_STATE_ENABLED, NULL);

	/* Lookup name for value and return it */
	switch(inState)
	{
		case ESDASHBOARD_PLUGIN_STATE_NONE:
			return("none");

		case ESDASHBOARD_PLUGIN_STATE_INITIALIZED:
			return("initialized");

		case ESDASHBOARD_PLUGIN_STATE_ENABLED:
			return("enabled");

		default:
			break;
	}

	/* We should normally never get here. But if we do return NULL. */
	return(NULL);
}

/* Return error message of last load attempt */
static const gchar* _esdashboard_plugin_get_loading_error(EsdashboardPlugin *self)
{
	EsdashboardPluginPrivate	*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_PLUGIN(self), NULL);

	priv=self->priv;

	/* Return error message of last loading attempt */
	return(priv->lastLoadingError);
}

/* Set file name for plugin */
static void _esdashboard_plugin_set_filename(EsdashboardPlugin *self, const gchar *inFilename)
{
	EsdashboardPluginPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(inFilename && *inFilename);
	g_return_if_fail(self->priv->state==ESDASHBOARD_PLUGIN_STATE_NONE);
	g_return_if_fail(self->priv->filename==NULL);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->filename, inFilename)!=0)
	{
		/* Set value */
		if(priv->filename) g_free(priv->filename);
		priv->filename=g_strdup(inFilename);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPluginProperties[PROP_FILENAME]);
	}
}

/* Update special paths for this plugin */
static gboolean _esdashboard_plugin_update_special_paths(EsdashboardPlugin *self)
{
	EsdashboardPluginPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_PLUGIN(self), FALSE);

	priv=self->priv;

	/* Check that ID of plugin is set */
	if(!priv->id)
	{
		g_critical("Cannot get path for plugin at %s", priv->filename);
		return(FALSE);
	}

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Update paths of this plugin */
	if(priv->configPath)
	{
		g_free(priv->configPath);
		priv->configPath=NULL;
	}
	priv->configPath=g_build_filename(g_get_user_config_dir(), "esdashboard", priv->id, NULL);

	if(priv->cachePath)
	{
		g_free(priv->cachePath);
		priv->cachePath=NULL;
	}
	priv->cachePath=g_build_filename(g_get_user_cache_dir(), "esdashboard", priv->id, NULL);

	if(priv->dataPath)
	{
		g_free(priv->dataPath);
		priv->dataPath=NULL;
	}
	priv->dataPath=g_build_filename(g_get_user_data_dir(), "esdashboard", priv->id, NULL);

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));

	/* Updating paths was successful */
	return(TRUE);
}

/* Set ID for plugin */
static void _esdashboard_plugin_set_id(EsdashboardPlugin *self, const gchar *inID)
{
	EsdashboardPluginPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(inID && *inID);
	g_return_if_fail(self->priv->id==NULL);
	g_return_if_fail(self->priv->state==ESDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->id, inID)!=0)
	{
		/* Set value */
		if(priv->id) g_free(priv->id);
		priv->id=g_strdup(inID);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPluginProperties[PROP_ID]);

		/* When ID changes then also paths of this plugin change */
		_esdashboard_plugin_update_special_paths(self);
	}
}

/* Set flags for plugin */
static void _esdashboard_plugin_set_flags(EsdashboardPlugin *self, EsdashboardPluginFlag inFlags)
{
	EsdashboardPluginPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->flags==ESDASHBOARD_PLUGIN_FLAG_NONE);
	g_return_if_fail(self->priv->state==ESDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(priv->flags!=inFlags)
	{
		/* Set value */
		priv->flags=inFlags;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPluginProperties[PROP_FLAGS]);
	}
}

/* Set name for plugin */
static void _esdashboard_plugin_set_name(EsdashboardPlugin *self, const gchar *inName)
{
	EsdashboardPluginPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->name==NULL);
	g_return_if_fail(self->priv->state==ESDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->name, inName)!=0)
	{
		/* Set value */
		if(priv->name)
		{
			g_free(priv->name);
			priv->name=NULL;
		}

		if(inName) priv->name=g_strdup(inName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPluginProperties[PROP_NAME]);
	}
}

/* Set description for plugin */
static void _esdashboard_plugin_set_description(EsdashboardPlugin *self, const gchar *inDescription)
{
	EsdashboardPluginPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->description==NULL);
	g_return_if_fail(self->priv->state==ESDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->description, inDescription)!=0)
	{
		/* Set value */
		if(priv->description)
		{
			g_free(priv->description);
			priv->description=NULL;
		}

		if(inDescription) priv->description=g_strdup(inDescription);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPluginProperties[PROP_DESCRIPTION]);
	}
}

/* Set author for plugin */
static void _esdashboard_plugin_set_author(EsdashboardPlugin *self, const gchar *inAuthor)
{
	EsdashboardPluginPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->author==NULL);
	g_return_if_fail(self->priv->state==ESDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->author, inAuthor)!=0)
	{
		/* Set value */
		if(priv->author)
		{
			g_free(priv->author);
			priv->author=NULL;
		}

		if(inAuthor) priv->author=g_strdup(inAuthor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPluginProperties[PROP_AUTHOR]);
	}
}

/* Set copyright for plugin */
static void _esdashboard_plugin_set_copyright(EsdashboardPlugin *self, const gchar *inCopyright)
{
	EsdashboardPluginPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->copyright==NULL);
	g_return_if_fail(self->priv->state==ESDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->copyright, inCopyright)!=0)
	{
		/* Set value */
		if(priv->copyright)
		{
			g_free(priv->copyright);
			priv->copyright=NULL;
		}

		if(inCopyright) priv->copyright=g_strdup(inCopyright);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPluginProperties[PROP_COPYRIGHT]);
	}
}

/* Set license for plugin */
static void _esdashboard_plugin_set_license(EsdashboardPlugin *self, const gchar *inLicense)
{
	EsdashboardPluginPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->license==NULL);
	g_return_if_fail(self->priv->state==ESDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->license, inLicense)!=0)
	{
		/* Set value */
		if(priv->license)
		{
			g_free(priv->license);
			priv->license=NULL;
		}

		if(inLicense) priv->license=g_strdup(inLicense);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPluginProperties[PROP_LICENSE]);
	}
}

/* IMPLEMENTATION: GTypeModule */

/* Load and initialize plugin */
static gboolean _esdashboard_plugin_load(GTypeModule *inModule)
{
	EsdashboardPlugin			*self;
	EsdashboardPluginPrivate	*priv;
	guint						signalID;
	gulong						handlerID;

	g_return_val_if_fail(ESDASHBOARD_IS_PLUGIN(inModule), FALSE);
	g_return_val_if_fail(G_IS_TYPE_MODULE(inModule), FALSE);

	self=ESDASHBOARD_PLUGIN(inModule);
	priv=self->priv;

	/* Reset last loading error if set */
	if(priv->lastLoadingError)
	{
		g_free(priv->lastLoadingError);
		priv->lastLoadingError=NULL;
	}

	/* Check if path to plugin was set and exists */
	if(!priv->filename)
	{
		priv->lastLoadingError=g_strdup("Missing path to plugin");
		return(FALSE);
	}

	if(!g_file_test(priv->filename, G_FILE_TEST_IS_REGULAR))
	{
		priv->lastLoadingError=g_strdup_printf("Path '%s' does not exist", priv->filename);
		return(FALSE);
	}

	/* Check that plugin is not in any state */
	if(priv->state!=ESDASHBOARD_PLUGIN_STATE_NONE)
	{
		priv->lastLoadingError=
			g_strdup_printf("Bad state '%s' - expected '%s",
							_esdashboard_plugin_get_plugin_state_value_name(priv->state),
							_esdashboard_plugin_get_plugin_state_value_name(ESDASHBOARD_PLUGIN_STATE_NONE));
		return(FALSE);
	}

	/* Open plugin module */
	if(priv->module)
	{
		priv->lastLoadingError=g_strdup("Plugin was already initialized");
		return(FALSE);
	}

	priv->module=g_module_open(priv->filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
	if(!priv->module)
	{
		priv->lastLoadingError=g_strdup(g_module_error());
		return(FALSE);
	}

	/* Check that plugin provides all necessary functions and get the address
	 * to these functions.
	 */
	if(!g_module_symbol(priv->module, ESDASHBOARD_PLUGIN_FUNCTION_NAME_INITIALIZE, (gpointer*)&priv->initialize))
	{
		priv->lastLoadingError=g_strdup(g_module_error());
		return(FALSE);
	}

	/* Initialize plugin */
	if(priv->initialize)
	{
		priv->initialize(self);
	}
		else
		{
			/* If we get here the virtual function was not overridden */
			priv->lastLoadingError=g_strdup_printf("Plugin does not implement required function %s", ESDASHBOARD_PLUGIN_FUNCTION_NAME_INITIALIZE);
			g_critical("Loading plugin at '%s' failed: %s", priv->filename, priv->lastLoadingError);
			return(FALSE);
		}

	/* Check that plugin has required properties set */
	if(!priv->id)
	{
		priv->lastLoadingError=g_strdup("Plugin did not set required ID");
		return(FALSE);
	}

	/* Check if plugin is valid, that means that plugin can be enabled and disabled */
	signalID=g_signal_lookup("enable", ESDASHBOARD_TYPE_PLUGIN);
	handlerID=g_signal_handler_find(self,
									G_SIGNAL_MATCH_ID,
									signalID,
									0,
									NULL,
									NULL,
									NULL);
	if(!handlerID)
	{
		priv->lastLoadingError=g_strdup("Plugin cannot be enabled");
		g_critical("Loading plugin at '%s' failed: %s", priv->filename, priv->lastLoadingError);
		return(FALSE);
	}

	signalID=g_signal_lookup("disable", ESDASHBOARD_TYPE_PLUGIN);
	handlerID=g_signal_handler_find(self,
									G_SIGNAL_MATCH_ID,
									signalID,
									0,
									NULL,
									NULL,
									NULL);
	if(!handlerID)
	{
		priv->lastLoadingError=g_strdup("Plugin cannot be disabled");
		g_critical("Loading plugin at '%s' failed: %s", priv->filename, priv->lastLoadingError);
		return(FALSE);
	}

	/* Set state of plugin */
	priv->state=ESDASHBOARD_PLUGIN_STATE_INITIALIZED;

	/* If we get here then loading and initializing plugin was successful */
	ESDASHBOARD_DEBUG(self, PLUGINS,
						"Loaded plugin '%s' successfully:\n  File: %s\n  Name: %s\n  Description: %s\n  Author: %s\n  Copyright: %s\n  License: %s",
						priv->id,
						priv->filename,
						priv->name ? priv->name : "",
						priv->description ? priv->description : "",
						priv->author ? priv->author : "",
						priv->copyright ? priv->copyright : "",
						priv->license ? priv->license : "");

	return(TRUE);
}

/* Disable and unload plugin */
static void _esdashboard_plugin_unload(GTypeModule *inModule)
{
	EsdashboardPlugin			*self;
	EsdashboardPluginPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(inModule));
	g_return_if_fail(G_IS_TYPE_MODULE(inModule));

	self=ESDASHBOARD_PLUGIN(inModule);
	priv=self->priv;

	/* Disable plugin if it is still enabled */
	if(priv->state==ESDASHBOARD_PLUGIN_STATE_ENABLED)
	{
		ESDASHBOARD_DEBUG(self, PLUGINS,
							"Disabing plugin '%s' before unloading module",
							priv->id);
		esdashboard_plugin_disable(self);
	}

	/* Close plugin module */
	if(priv->module)
	{
		/* Close module */
		if(!g_module_close(priv->module))
		{
			g_warning("Plugin '%s' could not be unloaded successfully: %s",
						priv->id ? priv->id : "Unknown",
						g_module_error());
			return;
		}

		/* Unset module and function pointers from plugin module */
		priv->initialize=NULL;

		priv->module=NULL;
	}

	/* Set state of plugin */
	priv->state=ESDASHBOARD_PLUGIN_STATE_NONE;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_plugin_dispose(GObject *inObject)
{
	EsdashboardPlugin			*self=ESDASHBOARD_PLUGIN(inObject);
	EsdashboardPluginPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->module)
	{
		_esdashboard_plugin_unload(G_TYPE_MODULE(self));
	}

	if(priv->lastLoadingError)
	{
		g_free(priv->lastLoadingError);
		priv->lastLoadingError=NULL;
	}

	if(priv->id)
	{
		g_free(priv->id);
		priv->id=NULL;
	}

	if(priv->name)
	{
		g_free(priv->name);
		priv->name=NULL;
	}

	if(priv->description)
	{
		g_free(priv->description);
		priv->description=NULL;
	}

	if(priv->author)
	{
		g_free(priv->author);
		priv->author=NULL;
	}

	if(priv->copyright)
	{
		g_free(priv->copyright);
		priv->copyright=NULL;
	}

	if(priv->license)
	{
		g_free(priv->license);
		priv->license=NULL;
	}

	if(priv->configPath)
	{
		g_free(priv->configPath);
		priv->configPath=NULL;
	}

	if(priv->cachePath)
	{
		g_free(priv->cachePath);
		priv->cachePath=NULL;
	}

	if(priv->dataPath)
	{
		g_free(priv->dataPath);
		priv->dataPath=NULL;
	}

	/* Sanity checks that module was unloaded - at least by us */
	g_assert(priv->initialize==NULL);

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_plugin_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_plugin_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	EsdashboardPlugin			*self=ESDASHBOARD_PLUGIN(inObject);
	
	switch(inPropID)
	{
		case PROP_FILENAME:
			_esdashboard_plugin_set_filename(self, g_value_get_string(inValue));
			break;

		case PROP_ID:
			_esdashboard_plugin_set_id(self, g_value_get_string(inValue));
			break;

		case PROP_FLAGS:
			_esdashboard_plugin_set_flags(self, g_value_get_flags(inValue));
			break;

		case PROP_NAME:
			_esdashboard_plugin_set_name(self, g_value_get_string(inValue));
			break;

		case PROP_DESCRIPTION:
			_esdashboard_plugin_set_description(self, g_value_get_string(inValue));
			break;

		case PROP_AUTHOR:
			_esdashboard_plugin_set_author(self, g_value_get_string(inValue));
			break;

		case PROP_COPYRIGHT:
			_esdashboard_plugin_set_copyright(self, g_value_get_string(inValue));
			break;

		case PROP_LICENSE:
			_esdashboard_plugin_set_license(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_plugin_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	EsdashboardPlugin			*self=ESDASHBOARD_PLUGIN(inObject);
	EsdashboardPluginPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_FILENAME:
			g_value_set_string(outValue, priv->filename);
			break;

		case PROP_ID:
			g_value_set_string(outValue, priv->id);
			break;

		case PROP_FLAGS:
			g_value_set_flags(outValue, priv->flags);
			break;

		case PROP_NAME:
			g_value_set_string(outValue, priv->name);
			break;

		case PROP_DESCRIPTION:
			g_value_set_string(outValue, priv->description);
			break;

		case PROP_AUTHOR:
			g_value_set_string(outValue, priv->author);
			break;

		case PROP_COPYRIGHT:
			g_value_set_string(outValue, priv->copyright);
			break;

		case PROP_LICENSE:
			g_value_set_string(outValue, priv->license);
			break;

		case PROP_CONFIG_PATH:
			g_value_set_string(outValue, priv->configPath);
			break;

		case PROP_CACHE_PATH:
			g_value_set_string(outValue, priv->cachePath);
			break;

		case PROP_DATA_PATH:
			g_value_set_string(outValue, priv->dataPath);
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
static void esdashboard_plugin_class_init(EsdashboardPluginClass *klass)
{
	GTypeModuleClass		*moduleClass=G_TYPE_MODULE_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	moduleClass->load=_esdashboard_plugin_load;
	moduleClass->unload=_esdashboard_plugin_unload;

	gobjectClass->set_property=_esdashboard_plugin_set_property;
	gobjectClass->get_property=_esdashboard_plugin_get_property;
	gobjectClass->dispose=_esdashboard_plugin_dispose;

	/* Define properties */
	EsdashboardPluginProperties[PROP_FILENAME]=
		g_param_spec_string("filename",
							"File name",
							"Path and file name of this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	EsdashboardPluginProperties[PROP_ID]=
		g_param_spec_string("id",
							"ID",
							"The unique ID for this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	EsdashboardPluginProperties[PROP_FLAGS]=
		g_param_spec_flags("flags",
							"Flags",
							"Flags defining behaviour of this plugin",
							ESDASHBOARD_TYPE_PLUGIN_FLAG,
							ESDASHBOARD_PLUGIN_FLAG_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardPluginProperties[PROP_NAME]=
		g_param_spec_string("name",
							"name",
							"Name of plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardPluginProperties[PROP_DESCRIPTION]=
		g_param_spec_string("description",
							"Description",
							"A short description about this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardPluginProperties[PROP_AUTHOR]=
		g_param_spec_string("author",
							"Author",
							"The author of this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardPluginProperties[PROP_COPYRIGHT]=
		g_param_spec_string("copyright",
							"Copyright",
							"The copyright of this plugin which usually contains year of development",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardPluginProperties[PROP_LICENSE]=
		g_param_spec_string("license",
							"License",
							"The license of this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardPluginProperties[PROP_CONFIG_PATH]=
		g_param_spec_string("config-path",
							"Config path",
							"The base path to configuration files of this plugin",
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	EsdashboardPluginProperties[PROP_CACHE_PATH]=
		g_param_spec_string("cache-path",
							"Cache path",
							"The base path to cache files of this plugin",
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	EsdashboardPluginProperties[PROP_DATA_PATH]=
		g_param_spec_string("data-path",
							"Data path",
							"The base path to data files of this plugin",
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardPluginProperties);

	/* Define signals */
	EsdashboardPluginSignals[ACTION_ENABLE]=
		g_signal_new("enable",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardPluginClass, enable),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	EsdashboardPluginSignals[ACTION_DISABLE]=
		g_signal_new("disable",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardPluginClass, disable),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	EsdashboardPluginSignals[ACTION_CONFIGURE]=
		g_signal_new("configure",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardPluginClass, configure),
						NULL,
						NULL,
						_esdashboard_marshal_OBJECT__VOID,
						G_TYPE_OBJECT,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_plugin_init(EsdashboardPlugin *self)
{
	EsdashboardPluginPrivate	*priv;

	priv=self->priv=esdashboard_plugin_get_instance_private(self);

	/* Set up default values */
	priv->filename=NULL;
	priv->module=NULL;
	priv->initialize=NULL;
	priv->state=ESDASHBOARD_PLUGIN_STATE_NONE;
	priv->lastLoadingError=NULL;

	priv->id=NULL;
	priv->flags=ESDASHBOARD_PLUGIN_FLAG_NONE;
	priv->name=NULL;
	priv->description=NULL;
	priv->author=NULL;
	priv->copyright=NULL;
	priv->license=NULL;

	priv->configPath=NULL;
	priv->cachePath=NULL;
	priv->dataPath=NULL;
}


/* IMPLEMENTATION: Errors */

GQuark esdashboard_plugin_error_quark(void)
{
	return(g_quark_from_static_string("esdashboard-plugin-error-quark"));
}


/* IMPLEMENTATION: Public API */

/* Create an uninitialized plugin */
EsdashboardPlugin* esdashboard_plugin_new(const gchar *inPluginFilename, GError **outError)
{
	GObject			*plugin;
	gchar			*pluginBasename;
	gchar			*pluginID;

	g_return_val_if_fail(inPluginFilename && *inPluginFilename, NULL);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	/* Get plugin ID from filename */
	pluginBasename=g_filename_display_basename(inPluginFilename);
	if(!pluginBasename)
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_PLUGIN_ERROR,
					ESDASHBOARD_PLUGIN_ERROR_ERROR,
					"Could not get plugin ID for file %s",
					inPluginFilename);

		/* Return NULL to indicate failure */
		return(NULL);
	}

	if(g_str_has_suffix(pluginBasename, G_MODULE_SUFFIX))
	{
		pluginID=g_strndup(pluginBasename, strlen(pluginBasename)-strlen(G_MODULE_SUFFIX)-1);
	}
		else
		{
			pluginID=g_strdup(pluginBasename);
		}

	/* Create object instance */
	plugin=g_object_new(ESDASHBOARD_TYPE_PLUGIN,
						"filename", inPluginFilename,
						"id", pluginID,
						NULL);
	if(!plugin)
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_PLUGIN_ERROR,
					ESDASHBOARD_PLUGIN_ERROR_ERROR,
					"Could not create plugin instance");

		/* Release allocated resources */
		if(pluginID) g_free(pluginID);
		if(pluginBasename) g_free(pluginBasename);

		/* Return NULL to indicate failure */
		return(NULL);
	}

	/* Load plugin */
	if(!g_type_module_use(G_TYPE_MODULE(plugin)))
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_PLUGIN_ERROR,
					ESDASHBOARD_PLUGIN_ERROR_ERROR,
					"%s",
					_esdashboard_plugin_get_loading_error(ESDASHBOARD_PLUGIN(plugin)));

		/* Release allocated resources */
		if(pluginID) g_free(pluginID);
		if(pluginBasename) g_free(pluginBasename);

		/* At this point we return NULL to indicate failure although the object
		 * instance (subclassing GTypeModule) now exists and it was tried to
		 * use it (via g_type_module_use). As describe in GObject documentation
		 * the object must not be unreffed via g_object_unref() but we also should
		 * not call g_type_module_unuse() because loading failed and the reference
		 * counter was not increased.
		 */
		return(NULL);
	}

	/* Release allocated resources */
	if(pluginID) g_free(pluginID);
	if(pluginBasename) g_free(pluginBasename);

	/* Plugin loaded so return it */
	return(ESDASHBOARD_PLUGIN(plugin));
}

/* Get ID of plugin */
const gchar* esdashboard_plugin_get_id(EsdashboardPlugin *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_PLUGIN(self), NULL);

	return(self->priv->id);
}

/* Get flags of plugin */
EsdashboardPluginFlag esdashboard_plugin_get_flags(EsdashboardPlugin *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_PLUGIN(self), ESDASHBOARD_PLUGIN_FLAG_NONE);

	return(self->priv->flags);
}

/* Set plugin information */
void esdashboard_plugin_set_info(EsdashboardPlugin *self,
									const gchar *inFirstPropertyName, ...)
{
	EsdashboardPluginPrivate		*priv;
	va_list							args;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));

	priv=self->priv;

	/* Check that plugin is not initialized already */
	if(priv->state!=ESDASHBOARD_PLUGIN_STATE_NONE)
	{
		g_critical("Setting plugin information for plugin '%s' at path '%s' failed: Plugin is already initialized",
					priv->id ? priv->id : "Unknown",
					priv->filename);
		return;
	}

	/* Set up properties */
	va_start(args, inFirstPropertyName);
	g_object_set_valist(G_OBJECT(self), inFirstPropertyName, args);
	va_end(args);
}

/* Get enabled state of plugin */
gboolean esdashboard_plugin_is_enabled(EsdashboardPlugin *self)
{
	EsdashboardPluginPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_PLUGIN(self), FALSE);

	priv=self->priv;

	/* Only return TRUE if state is enabled */
	if(priv->state==ESDASHBOARD_PLUGIN_STATE_ENABLED) return(TRUE);

	/* If we get here plugin is not in enabled state so return FALSE */
	return(FALSE);
}

/* Enable plugin */
void esdashboard_plugin_enable(EsdashboardPlugin *self)
{
	EsdashboardPluginPrivate		*priv;
	gboolean						result;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));

	priv=self->priv;
	result=FALSE;

	/* Do nothing and return immediately if plugin is enabled already */
	if(priv->state==ESDASHBOARD_PLUGIN_STATE_ENABLED)
	{
		ESDASHBOARD_DEBUG(self, PLUGINS,
							"Plugin '%s' is already enabled",
							priv->id);
		return;
	}

	/* Check that plugin is initialized */
	if(priv->state!=ESDASHBOARD_PLUGIN_STATE_INITIALIZED)
	{
		g_critical("Enabling plugin '%s' failed: Bad state '%s' - expected '%s'",
					priv->id ? priv->id : "Unknown",
					_esdashboard_plugin_get_plugin_state_value_name(priv->state),
					_esdashboard_plugin_get_plugin_state_value_name(ESDASHBOARD_PLUGIN_STATE_INITIALIZED));
		return;
	}

	/* Emit signal action 'enable' to enable plugin */
	g_signal_emit(self, EsdashboardPluginSignals[ACTION_ENABLE], 0, &result);
	ESDASHBOARD_DEBUG(self, PLUGINS,
						"Plugin '%s' enabled",
						priv->id);

	/* Set enabled state */
	priv->state=ESDASHBOARD_PLUGIN_STATE_ENABLED;
}

/* Disable plugin */
void esdashboard_plugin_disable(EsdashboardPlugin *self)
{
	EsdashboardPluginPrivate		*priv;
	gboolean						result;

	g_return_if_fail(ESDASHBOARD_IS_PLUGIN(self));

	priv=self->priv;

	/* Do nothing and return immediately if plugin is not enabled */
	if(priv->state!=ESDASHBOARD_PLUGIN_STATE_ENABLED)
	{
		ESDASHBOARD_DEBUG(self, PLUGINS,
							"Plugin '%s' is already disabled",
							priv->id);
		return;
	}

	/* Emit signal action 'disable' to disable plugin */
	g_signal_emit(self, EsdashboardPluginSignals[ACTION_DISABLE], 0, &result);
	ESDASHBOARD_DEBUG(self, PLUGINS,
						"Plugin '%s' disabled",
						priv->id);

	/* Set disabled state, i.e. revert to initialized state */
	priv->state=ESDASHBOARD_PLUGIN_STATE_INITIALIZED;
}

/* Get base path to configuration files of this plugin */
const gchar* esdashboard_plugin_get_config_path(EsdashboardPlugin *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_PLUGIN(self), NULL);

	return(self->priv->configPath);
}

/* Get base path to cache files of this plugin */
const gchar* esdashboard_plugin_get_cache_path(EsdashboardPlugin *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_PLUGIN(self), NULL);

	return(self->priv->cachePath);
}

/* Get base path to data files of this plugin */
const gchar* esdashboard_plugin_get_data_path(EsdashboardPlugin *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_PLUGIN(self), NULL);

	return(self->priv->dataPath);
}
