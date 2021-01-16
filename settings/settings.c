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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "settings.h"

#include <glib/gi18n-lib.h>
#include <libexpidus1ui/libexpidus1ui.h>
#include <esconf/esconf.h>
#include <math.h>

#include "general.h"
#include "plugins.h"
#include "themes.h"


/* Define this class in GObject system */
struct _EsdashboardSettingsPrivate
{
	/* Instance related */
	EsconfChannel					*esconfChannel;

	GtkBuilder						*builder;
	GObject							*dialog;

	EsdashboardSettingsGeneral		*general;
	EsdashboardSettingsThemes		*themes;
	EsdashboardSettingsPlugins		*plugins;

	GtkWidget						*widgetHelpButton;
	GtkWidget						*widgetCloseButton;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardSettings,
							esdashboard_settings,
							G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
#define ESDASHBOARD_ESCONF_CHANNEL					"esdashboard"

#define PREFERENCES_UI_FILE							"preferences.ui"


/* Help button was clicked */
static void _esdashboard_settings_on_help_clicked(EsdashboardSettings *self,
													GtkWidget *inWidget)
{
	EsdashboardSettingsPrivate				*priv;
	GtkWindow								*window;

	g_return_if_fail(ESDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Show online manual for esdashboard but ask user if needed */
	window=NULL;
	if(GTK_IS_WINDOW(priv->dialog)) window=GTK_WINDOW(priv->dialog);

	expidus_dialog_show_help_with_version(window,
										"esdashboard",
										"start",
										NULL,
										NULL);
}

/* Close button was clicked */
static void _esdashboard_settings_on_close_clicked(EsdashboardSettings *self,
													GtkWidget *inWidget)
{
	g_return_if_fail(ESDASHBOARD_IS_SETTINGS(self));

	/* Quit main loop */
	gtk_main_quit();
}

/* Create and set up GtkBuilder */
static gboolean _esdashboard_settings_create_builder(EsdashboardSettings *self)
{
	EsdashboardSettingsPrivate				*priv;
	gchar									*builderFile;
	GtkBuilder								*builder;
	GError									*error;

	g_return_val_if_fail(ESDASHBOARD_IS_SETTINGS(self), FALSE);

	priv=self->priv;
	builderFile=NULL;
	builder=NULL;
	error=NULL;

	/* If builder is already set up return immediately */
	if(priv->builder) return(TRUE);

	/* Search UI file in given environment variable if set.
	 * This makes development easier to test modifications at UI file.
	 */
	if(!builderFile)
	{
		const gchar		*envPath;

		envPath=g_getenv("ESDASHBOARD_UI_PATH");
		if(envPath)
		{
			builderFile=g_build_filename(envPath, PREFERENCES_UI_FILE, NULL);
			g_debug("Trying UI file: %s", builderFile);
			if(!g_file_test(builderFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_free(builderFile);
				builderFile=NULL;
			}
		}
	}

	/* Find UI file at install path */
	if(!builderFile)
	{
		builderFile=g_build_filename(PACKAGE_DATADIR, "esdashboard", PREFERENCES_UI_FILE, NULL);
		g_debug("Trying UI file: %s", builderFile);
		if(!g_file_test(builderFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			g_critical("Could not find UI file '%s'.", builderFile);

			/* Release allocated resources */
			g_free(builderFile);

			/* Return fail result */
			return(FALSE);
		}
	}

	/* Create builder */
	builder=gtk_builder_new();
	if(!gtk_builder_add_from_file(builder, builderFile, &error))
	{
		g_critical("Could not load UI resources from '%s': %s",
					builderFile,
					error ? error->message : "Unknown error");

		/* Release allocated resources */
		g_free(builderFile);
		g_object_unref(builder);
		if(error) g_error_free(error);

		/* Return fail result */
		return(FALSE);
	}

	/* Loading UI resource was successful so take extra reference
	 * from builder object to keep it alive. Also get widget, set up
	 * esconf bindings and connect signals.
	 * REMEMBER: Set (widget's) default value _before_ setting up
	 * esconf binding.
	 */
	priv->builder=GTK_BUILDER(g_object_ref(builder));
	g_debug("Loaded UI resources from '%s' successfully.", builderFile);

	/* Setup common widgets */
	priv->widgetHelpButton=GTK_WIDGET(gtk_builder_get_object(priv->builder, "help-button"));
	g_signal_connect_swapped(priv->widgetHelpButton,
								"clicked",
								G_CALLBACK(_esdashboard_settings_on_help_clicked),
								self);

	priv->widgetCloseButton=GTK_WIDGET(gtk_builder_get_object(priv->builder, "close-button"));
	g_signal_connect_swapped(priv->widgetCloseButton,
								"clicked",
								G_CALLBACK(_esdashboard_settings_on_close_clicked),
								self);

	/* Tab: General */
	priv->general=esdashboard_settings_general_new(builder);

	/* Tab: Themes */
	priv->themes=esdashboard_settings_themes_new(builder);

	/* Tab: Plugins */
	priv->plugins=esdashboard_settings_plugins_new(builder);

	/* Release allocated resources */
	g_free(builderFile);
	g_object_unref(builder);

	/* Return success result */
	return(TRUE);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_settings_dispose(GObject *inObject)
{
	EsdashboardSettings			*self=ESDASHBOARD_SETTINGS(inObject);
	EsdashboardSettingsPrivate	*priv=self->priv;

	/* Release allocated resouces */
	priv->dialog=NULL;
	priv->widgetHelpButton=NULL;
	priv->widgetCloseButton=NULL;

	if(priv->themes)
	{
		g_object_unref(priv->themes);
		priv->themes=NULL;
	}

	if(priv->general)
	{
		g_object_unref(priv->general);
		priv->general=NULL;
	}

	if(priv->plugins)
	{
		g_object_unref(priv->plugins);
		priv->plugins=NULL;
	}

	if(priv->esconfChannel)
	{
		priv->esconfChannel=NULL;
	}

	if(priv->builder)
	{
		g_object_unref(priv->builder);
		priv->builder=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_settings_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void esdashboard_settings_class_init(EsdashboardSettingsClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_settings_dispose;
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_settings_init(EsdashboardSettings *self)
{
	EsdashboardSettingsPrivate	*priv;

	priv=self->priv=esdashboard_settings_get_instance_private(self);

	/* Set default values */
	priv->esconfChannel=esconf_channel_get(ESDASHBOARD_ESCONF_CHANNEL);
	priv->builder=NULL;
	priv->dialog=NULL;
	priv->general=NULL;
	priv->themes=NULL;
	priv->plugins=NULL;
	priv->widgetHelpButton=NULL;
	priv->widgetCloseButton=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create instance of this class */
EsdashboardSettings* esdashboard_settings_new(void)
{
	return(ESDASHBOARD_SETTINGS(g_object_new(ESDASHBOARD_TYPE_SETTINGS, NULL)));
}

/* Create standalone dialog for this settings instance */
GtkWidget* esdashboard_settings_create_dialog(EsdashboardSettings *self)
{
	EsdashboardSettingsPrivate	*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_SETTINGS(self), NULL);

	priv=self->priv;

	/* Get builder if not available */
	if(!_esdashboard_settings_create_builder(self))
	{
		/* An critical error message should be displayed so just return NULL */
		return(NULL);
	}

	/* Get dialog object */
	g_assert(priv->dialog==NULL);

	priv->dialog=gtk_builder_get_object(priv->builder, "preferences-dialog");
	if(!priv->dialog)
	{
		g_critical("Could not get dialog from UI file.");
		return(NULL);
	}

	/* Return widget */
	return(GTK_WIDGET(priv->dialog));
}

/* Create "pluggable" dialog for this settings instance */
GtkWidget* esdashboard_settings_create_plug(EsdashboardSettings *self, Window inSocketID)
{
	EsdashboardSettingsPrivate	*priv;
	GtkWidget					*plug;
	GObject						*dialogChild;
#if GTK_CHECK_VERSION(3, 14 ,0)
	GtkWidget					*dialogParent;
#endif

	g_return_val_if_fail(ESDASHBOARD_IS_SETTINGS(self), NULL);
	g_return_val_if_fail(inSocketID, NULL);

	priv=self->priv;

	/* Get builder if not available */
	if(!_esdashboard_settings_create_builder(self))
	{
		/* An critical error message should be displayed so just return NULL */
		return(NULL);
	}

	/* Get dialog object */
	dialogChild=gtk_builder_get_object(priv->builder, "preferences-plug-child");
	if(!dialogChild)
	{
		g_critical("Could not get dialog from UI file.");
		return(NULL);
	}

	/* Create plug widget and reparent dialog object to it */
	plug=gtk_plug_new(inSocketID);
#if GTK_CHECK_VERSION(3, 14 ,0)
	g_object_ref(G_OBJECT(dialogChild));

	dialogParent=gtk_widget_get_parent(GTK_WIDGET(dialogChild));
	gtk_container_remove(GTK_CONTAINER(dialogParent), GTK_WIDGET(dialogChild));
	gtk_container_add(GTK_CONTAINER(plug), GTK_WIDGET(dialogChild));

	g_object_unref(G_OBJECT(dialogChild));
#else
	gtk_widget_reparent(GTK_WIDGET(dialogChild), plug);
#endif
	gtk_widget_show(GTK_WIDGET(dialogChild));

	/* Return widget */
	return(GTK_WIDGET(plug));
}
