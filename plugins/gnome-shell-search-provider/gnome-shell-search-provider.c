/*
 * gnome-shell-search-provider: A search provider using GnomeShell
 *                              search providers
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

#include "gnome-shell-search-provider.h"

#include <glib/gi18n-lib.h>
#include <gio/gio.h>


/* Define this class in GObject system */
struct _EsdashboardGnomeShellSearchProviderPrivate
{
	/* Instance related */
	gchar			*gnomeShellID;
	GFile			*file;
	GFileMonitor	*fileMonitor;

	gchar			*desktopID;
	gchar			*dbusBusName;
	gchar			*dbusObjectPath;
	gint			searchProviderVersion;

	gchar			*providerName;
	gchar			*providerIcon;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(EsdashboardGnomeShellSearchProvider,
								esdashboard_gnome_shell_search_provider,
								ESDASHBOARD_TYPE_SEARCH_PROVIDER,
								0,
								G_ADD_PRIVATE_DYNAMIC(EsdashboardGnomeShellSearchProvider))

/* Define this class in this plugin */
ESDASHBOARD_DEFINE_PLUGIN_TYPE(esdashboard_gnome_shell_search_provider);

/* IMPLEMENTATION: Private variables and methods */
#define ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_KEYFILE_GROUP		"Shell Search Provider"


/* IMPLEMENTATION: EsdashboardSearchProvider */

/* Update information about Gnome-Shell search provider from file */
static gboolean _esdashboard_gnome_shell_search_provider_update_from_file(EsdashboardGnomeShellSearchProvider *self,
																			GError **outError)
{
	EsdashboardGnomeShellSearchProviderPrivate		*priv;
	gchar											*filePath;
	GKeyFile										*providerKeyFile;
	EsdashboardApplicationDatabase					*appDB;
	GAppInfo										*appInfo;
	gchar											*desktopID;
	gchar											*dbusBusName;
	gchar											*dbusObjectPath;
	gint											searchProviderVersion;
	gchar											*providerName;
	gchar											*providerIcon;
	GError											*error;

	g_return_val_if_fail(ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Get path to Gnome-Shell search provider's data file */
	filePath=g_file_get_path(priv->file);

	/* Get data about search provider */
	providerKeyFile=g_key_file_new();
	if(!g_key_file_load_from_file(providerKeyFile,
									filePath,
									G_KEY_FILE_NONE,
									&error))
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(providerKeyFile) g_key_file_free(providerKeyFile);
		if(filePath) g_free(filePath);

		return(FALSE);
	}

	/* Get desktop ID from search provider's data file */
	desktopID=g_key_file_get_string(providerKeyFile,
									ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_KEYFILE_GROUP,
									"DesktopId",
									&error);
	if(!desktopID)
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(providerKeyFile) g_key_file_free(providerKeyFile);
		if(filePath) g_free(filePath);

		return(FALSE);
	}

	/* Get bus name from search provider's data file */
	dbusBusName=g_key_file_get_string(providerKeyFile,
										ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_KEYFILE_GROUP,
										"BusName",
										&error);
	if(!dbusBusName)
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(desktopID) g_free(desktopID);
		if(providerKeyFile) g_key_file_free(providerKeyFile);
		if(filePath) g_free(filePath);

		return(FALSE);
	}

	/* Get object path from search provider's data file */
	dbusObjectPath=g_key_file_get_string(providerKeyFile,
											ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_KEYFILE_GROUP,
											"ObjectPath",
											&error);
	if(!dbusObjectPath)
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(dbusBusName) g_free(dbusBusName);
		if(desktopID) g_free(desktopID);
		if(providerKeyFile) g_key_file_free(providerKeyFile);
		if(filePath) g_free(filePath);

		return(FALSE);
	}

	/* Get version from search provider's data file */
	searchProviderVersion=g_key_file_get_integer(providerKeyFile,
													ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_KEYFILE_GROUP,
													"Version",
													&error);
	if(!searchProviderVersion)
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(dbusObjectPath) g_free(dbusObjectPath);
		if(dbusBusName) g_free(dbusBusName);
		if(desktopID) g_free(desktopID);
		if(providerKeyFile) g_key_file_free(providerKeyFile);
		if(filePath) g_free(filePath);

		return(FALSE);
	}

	/* Get display name and icon from desktop file returned from application
	 * database by lookup of desktop ID.
	 */
	providerName=NULL;
	providerIcon=NULL;

	appDB=esdashboard_application_database_get_default();

	appInfo=esdashboard_application_database_lookup_desktop_id(appDB, desktopID);
	if(appInfo)
	{
		GIcon										*appIcon;

		/* Get name */
		providerName=g_strdup(g_app_info_get_display_name(appInfo));

		/* Get icon */
		appIcon=g_app_info_get_icon(appInfo);
		if(appIcon)
		{
			providerIcon=g_icon_to_string(appIcon);
			g_object_unref(appIcon);
		}
	}
		else
		{
			/* Show error message */
			g_warning("Unknown application '%s' for Gnome-Shell search provider '%s'",
						desktopID,
						priv->gnomeShellID);
		}

	/* We got all data from search provider's data file so update now.
	 * Set default values where appropiate (display name and icon).
	 */
	if(priv->desktopID) g_free(priv->desktopID);
	priv->desktopID=g_strdup(desktopID);

	if(priv->dbusBusName) g_free(priv->dbusBusName);
	priv->dbusBusName=g_strdup(dbusBusName);

	if(priv->dbusObjectPath) g_free(priv->dbusObjectPath);
	priv->dbusObjectPath=g_strdup(dbusObjectPath);

	priv->searchProviderVersion=searchProviderVersion;

	if(priv->providerName) g_free(priv->providerName);
	if(providerName) priv->providerName=g_strdup(providerName);
		else priv->providerName=g_strdup(priv->gnomeShellID);

	if(priv->providerIcon) g_free(priv->providerIcon);
	if(providerIcon) priv->providerIcon=g_strdup(providerIcon);
		else priv->providerIcon=g_strdup("image-missing");

	/* Release allocated resources */
	if(appInfo) g_object_unref(appInfo);
	if(appDB) g_object_unref(appDB);
	if(providerIcon) g_free(providerIcon);
	if(providerName) g_free(providerName);
	if(dbusObjectPath) g_free(dbusObjectPath);
	if(dbusBusName) g_free(dbusBusName);
	if(desktopID) g_free(desktopID);
	if(providerKeyFile) g_key_file_free(providerKeyFile);
	if(filePath) g_free(filePath);

	/* If we get here we could update from file successfully */
	g_debug("Updated search provider '%s' of type %s for Gnome-Shell search provider interface version %d using DBUS name '%s' and object path '%s' displayed as '%s' with icon '%s' from desktop ID '%s'",
				esdashboard_search_provider_get_id(ESDASHBOARD_SEARCH_PROVIDER(self)),
				G_OBJECT_TYPE_NAME(self),
				priv->searchProviderVersion,
				priv->dbusBusName,
				priv->dbusObjectPath,
				priv->providerName,
				priv->providerIcon,
				priv->desktopID);

	return(TRUE);
}

/* The data file of Gnome-Shell search provider has changed */
static void _esdashboard_gnome_shell_search_provider_on_data_file_changed(EsdashboardGnomeShellSearchProvider *self,
																			GFile *inFile,
																			GFile *inOtherFile,
																			GFileMonitorEvent inEventType,
																			gpointer inUserData)
{
	EsdashboardGnomeShellSearchProviderPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(self));
	g_return_if_fail(G_IS_FILE_MONITOR(inUserData));

	priv=self->priv;

	/* Check if a search provider was added */
	if(inEventType==G_FILE_MONITOR_EVENT_CHANGED &&
		g_file_equal(inFile, priv->file))
	{
		GError										*error;

		error=NULL;

		/* Update information about Gnome-Shell search provider from modified data file */
		if(!_esdashboard_gnome_shell_search_provider_update_from_file(self, &error))
		{
			/* Show warning message */
			g_warning("Cannot update information about Gnome-Shell search provider '%s': %s",
						priv->gnomeShellID,
						(error && error->message) ? error->message : "Unknown error");

			/* Release allocated resources */
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}
		}
			else
			{
				g_debug("Updated Gnome-Shell search provider '%s' of type %s with ID '%s' from modified data file successfully",
							priv->gnomeShellID,
							G_OBJECT_TYPE_NAME(self),
							esdashboard_search_provider_get_id(ESDASHBOARD_SEARCH_PROVIDER(self)));
			}
	}
}

/* One-time initialization of search provider */
static void _esdashboard_gnome_shell_search_provider_initialize(EsdashboardSearchProvider *inProvider)
{
	EsdashboardGnomeShellSearchProvider				*self;
	EsdashboardGnomeShellSearchProviderPrivate		*priv;
	GError											*error;

	g_return_if_fail(ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(inProvider));

	self=ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	error=NULL;

	/* Get ID of Gnome-Shell search provider */
	if(!priv->gnomeShellID)
	{
		const gchar									*providerID;

		providerID=esdashboard_search_provider_get_id(inProvider);
		priv->gnomeShellID=g_strdup(providerID+strlen(PLUGIN_ID)+1);
	}
	g_debug("Initializing search provider '%s' of type %s for Gnome-Shell search provider ID '%s'",
				esdashboard_search_provider_get_id(inProvider),
				G_OBJECT_TYPE_NAME(self),
				priv->gnomeShellID);

	/* Get Gnome-Shell search provider's data file */
	if(!priv->file)
	{
		gchar										*filename;
		gchar										*filePath;

		/* Get path to Gnome-Shell search provider's data file */
		filename=g_strdup_printf("%s.ini", priv->gnomeShellID);
		filePath=g_build_filename(GNOME_SHELL_PROVIDERS_PATH, filename, NULL);

		/* Get Gnome-Shell search provider's data file */
		priv->file=g_file_new_for_path(filePath);

		/* Release allocated resources */
		g_free(filename);
		g_free(filePath);
	}

	/* Get file monitor to detect changes at Gnome-Shell search provider's data file.
	 * It is not fatal if file monitor could not be initialized because we just do not
	 * get notified about any changes to the data file. But print a warning.
	 */
	if(!priv->fileMonitor)
	{
		priv->fileMonitor=g_file_monitor_file(priv->file, 0, NULL, &error);
		if(priv->fileMonitor)
		{
			g_debug("Created file monitor to watch for changes at Gnome-Shell search provider '%s'",
					priv->gnomeShellID);

			g_signal_connect_swapped(priv->fileMonitor,
										"changed",
										G_CALLBACK(_esdashboard_gnome_shell_search_provider_on_data_file_changed),
										self);
		}
			else
			{
				/* Show warning message */
				g_warning("Cannot initialize file monitor to detect changes for Gnome-Shell search provider '%s': %s",
							priv->gnomeShellID,
							(error && error->message) ? error->message : "Unknown error");

				/* Release allocated resources */
				if(error)
				{
					g_error_free(error);
					error=NULL;
				}
			}
	}

	/* Get information about Gnome-Shell search provider from file */
	if(!_esdashboard_gnome_shell_search_provider_update_from_file(self, &error))
	{
		/* Show warning message */
		g_warning("Cannot load information about Gnome-Shell search provider '%s': %s",
					priv->gnomeShellID,
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(error)
		{
			g_error_free(error);
			error=NULL;
		}
	}
		else
		{
			g_debug("Initialized Gnome-Shell search provider '%s' of type %s with ID '%s' successfully",
						priv->gnomeShellID,
						G_OBJECT_TYPE_NAME(self),
						esdashboard_search_provider_get_id(inProvider));
		}
}

/* Get display name for this search provider */
static const gchar* _esdashboard_gnome_shell_search_provider_get_name(EsdashboardSearchProvider *inProvider)
{
	EsdashboardGnomeShellSearchProvider				*self;
	EsdashboardGnomeShellSearchProviderPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(inProvider), NULL);

	self=ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(inProvider);
	priv=self->priv;

	return(priv->providerName);
}

/* Get icon-name for this search provider */
static const gchar* _esdashboard_gnome_shell_search_provider_get_icon(EsdashboardSearchProvider *inProvider)
{
	EsdashboardGnomeShellSearchProvider				*self;
	EsdashboardGnomeShellSearchProviderPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(inProvider), NULL);

	self=ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(inProvider);
	priv=self->priv;

	return(priv->providerIcon);
}

/* Get result set for requested search terms */
static EsdashboardSearchResultSet* _esdashboard_gnome_shell_search_provider_get_result_set(EsdashboardSearchProvider *inProvider,
																							const gchar **inSearchTerms,
																							EsdashboardSearchResultSet *inPreviousResultSet)
{
	EsdashboardGnomeShellSearchProvider				*self;
	EsdashboardGnomeShellSearchProviderPrivate		*priv;
	GError											*error;
	EsdashboardSearchResultSet						*resultSet;
	GVariant										*resultItem;
	GDBusProxy										*proxy;
	GVariant										*proxyResult;
	gchar											**proxyResultSet;
	gchar											**iter;

	g_return_val_if_fail(ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(inProvider), NULL);

	self=ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	error=NULL;
	resultSet=NULL;

	/* Connect to search provider via DBUS */
	proxy=g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
										G_DBUS_PROXY_FLAGS_NONE,
										NULL,
										priv->dbusBusName,
										priv->dbusObjectPath,
										"org.gnome.Shell.SearchProvider2",
										NULL,
										&error);
	if(!proxy)
	{
		/* Show error message */
		g_warning("Could not create dbus connection for Gnome-Shell search provider '%s': %s",
					priv->gnomeShellID,
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(error) g_error_free(error);

		/* Return NULL to indicate error */
		return(NULL);
	}

	/* Call search method at search provider depending on if a initial
	 * result set is requested or an update for a previous result set.
	 */
	if(!inPreviousResultSet)
	{
		/* Call search method at search provider to get initial result set */
		proxyResult=g_dbus_proxy_call_sync(proxy,
											"GetInitialResultSet",
											g_variant_new("(^as)", inSearchTerms),
											G_DBUS_CALL_FLAGS_NONE,
											-1,
											NULL,
											&error);
		g_debug("Fetched initial result set at %p for Gnome Shell search provider '%s' of type %s",
					proxyResult,
					priv->gnomeShellID,
					G_OBJECT_TYPE_NAME(self));
	}
		else
		{
			GVariantBuilder						builder;
			GList								*allPrevResults;
			GList								*allPrevIter;

			/* Initialize GVariant builder to get a GVariant with an array
			 * of strings for previous result set.
			 */
			g_variant_builder_init(&builder, G_VARIANT_TYPE_STRING_ARRAY);

			/* For each result item in previous result set add a string
			 * to GVariant builder.
			 */
			allPrevResults=esdashboard_search_result_set_get_all(inPreviousResultSet);
			for(allPrevIter=allPrevResults; allPrevIter; allPrevIter=g_list_next(allPrevIter))
			{
				g_variant_builder_add(&builder, "s", g_variant_get_string((GVariant*)allPrevIter->data, NULL));
			}
			g_debug("Built previous result set with %d entries for Gnome Shell search provider '%s' of type %s",
						g_list_length(allPrevResults),
						priv->gnomeShellID,
						G_OBJECT_TYPE_NAME(self));
			g_list_free_full(allPrevResults, (GDestroyNotify)g_variant_unref);

			/* Call search method at search provider to get an update
			 * for previous result set.
			 */
			proxyResult=g_dbus_proxy_call_sync(proxy,
												"GetSubsearchResultSet",
												g_variant_new("(as^as)", &builder, inSearchTerms),
												G_DBUS_CALL_FLAGS_NONE,
												-1,
												NULL,
												&error);
			g_debug("Fetched subset result set at %p for Gnome Shell search provider '%s' of type %s",
						proxyResult,
						priv->gnomeShellID,
						G_OBJECT_TYPE_NAME(self));
		}

	if(!proxyResult)
	{
		/* Show error message */
		g_warning("Could get result set from dbus connection for Gnome-Shell search provider '%s': %s",
					priv->gnomeShellID,
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(proxy) g_object_unref(proxy);

		/* Return NULL to indicate error */
		return(NULL);
	}

	/* Retrieve result set for this application from returned result set of
	 * search provider.
	 */
	proxyResultSet=NULL;
	g_variant_get(proxyResult, "(^as)", &proxyResultSet);

	if(proxyResultSet)
	{
		/* Initialize result set */
		resultSet=esdashboard_search_result_set_new();

		/* For each string in returned result set of search provider create a GVariant
		 * which gets added with full score to result set for this application.
		 */
		for(iter=proxyResultSet; *iter; iter++)
		{
			resultItem=g_variant_new_string(*iter);
			if(resultItem)
			{
				esdashboard_search_result_set_add_item(resultSet, g_variant_ref(resultItem));
				esdashboard_search_result_set_set_item_score(resultSet, resultItem, 1.0f);

				/* Release result item added */
				g_variant_unref(resultItem);
			}
		}
		g_debug("Got result set with %u entries for Gnome Shell search provider '%s' of type %s",
					esdashboard_search_result_set_get_size(resultSet),
					priv->gnomeShellID,
					G_OBJECT_TYPE_NAME(self));
	}

	/* Release allocated resources */
	if(proxyResultSet) g_strfreev(proxyResultSet);
	if(proxyResult) g_variant_unref(proxyResult);
	if(proxy) g_object_unref(proxy);

	/* Return result set */
	return(resultSet);
}

/* Create actor for a result item of the result set returned from a search request */
static ClutterActor* _esdashboard_gnome_shell_search_provider_create_result_actor(EsdashboardSearchProvider *inProvider,
																					GVariant *inResultItem)
{
	EsdashboardGnomeShellSearchProvider				*self;
	EsdashboardGnomeShellSearchProviderPrivate		*priv;
	GError											*error;
	const gchar										*identifier[2];
	ClutterActor									*actor;
	GDBusProxy										*proxy;
	GVariant										*proxyResult;
	GVariantIter									*resultIter;
	gchar											*name;
	gchar											*description;
	GIcon											*icon;
	ClutterContent									*iconImage;

	g_return_val_if_fail(ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(inProvider), NULL);
	g_return_val_if_fail(inResultItem, NULL);

	self=ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	actor=NULL;
	name=NULL;
	description=NULL;
	icon=NULL;
	iconImage=NULL;
	error=NULL;

	/* Get meta data of result item */
	proxy=g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
										G_DBUS_PROXY_FLAGS_NONE,
										NULL,
										priv->dbusBusName,
										priv->dbusObjectPath,
										"org.gnome.Shell.SearchProvider2",
										NULL,
										&error);
	if(!proxy)
	{
		/* Show error message */
		g_warning("Could not create dbus connection for Gnome-Shell search provider '%s': %s",
					priv->gnomeShellID,
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(error) g_error_free(error);

		/* Return NULL to indicate error */
		return(NULL);
	}

	identifier[0]=g_variant_get_string(inResultItem, NULL);
	identifier[1]=NULL;
	proxyResult=g_dbus_proxy_call_sync(proxy,
										"GetResultMetas",
										g_variant_new("(^as)", identifier),
										G_DBUS_CALL_FLAGS_NONE,
										-1,
										NULL,
										&error);
	if(!proxyResult)
	{
		/* Show error message */
		g_warning("Could get meta data for '%s' from dbus connection for Gnome-Shell search provider '%s': %s",
					identifier[0],
					priv->gnomeShellID,
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(proxy) g_object_unref(proxy);

		/* Return NULL to indicate error */
		return(NULL);
	}

	/* Extract meta data from result we got from dbus call */
	resultIter=NULL;

	g_variant_get(proxyResult, "(aa{sv})", &resultIter);
	if(resultIter)
	{
		GVariant									*metaData;

		while((metaData=g_variant_iter_next_value(resultIter)))
		{
			GVariant								*iconVariant;
			gchar									*iconString;
			gint32									iconWidth;
			gint32									iconHeight;
			gint32									iconRowstride;
			gboolean								iconHasAlpha;
			gint32									iconBits;
			gint32									iconChannels;
			guchar									*iconData;
			gchar									*resultID;

			/* Get ID from meta data and check if it matches the ID requested.
			 * If not try next meta data item in result.
			 */
			resultID=NULL;
			if(!g_variant_lookup(metaData, "id", "s", &resultID) ||
				g_strcmp0(resultID, identifier[0])!=0)
			{
				if(resultID) g_free(resultID);
				continue;
			}

			g_free(resultID);

			/* Get name from meta data */
			g_variant_lookup(metaData, "name", "s", &name);

			/* Get description from meta data */
			g_variant_lookup(metaData, "description", "s", &description);

			/* Get icon from meta data.
			 * Try first to deserialize "icon" if available and supported,
			 * then try to decode "gicon" if available and at last try
			 * raw bytes from "icon-data".
			 */
#if GLIB_CHECK_VERSION(2, 38, 0)
			if(!icon && g_variant_lookup(metaData, "icon", "v", &iconVariant))
			{
				/* Try deserializing icon from variant extracted from meta data */
				icon=g_icon_deserialize(iconVariant);
				if(!icon)
				{
					/* Show error message */
					g_warning("Could get icon for '%s' of key '%s' for Gnome-Shell search provider '%s': %s",
								identifier[0],
								"icon",
								priv->gnomeShellID,
								"Deserialization failed");
				}

				/* Release data extracted for icon */
				g_variant_unref(iconVariant);
			}
#endif

			if(!icon && g_variant_lookup(metaData, "gicon", "s", &iconString))
			{
				/* Try decoding icon from string extracted from meta data */
				icon=g_icon_new_for_string(iconString, &error);
				if(!icon)
				{
					/* Show error message */
					g_warning("Could get icon for '%s' of key '%s' for Gnome-Shell search provider '%s': %s",
								identifier[0],
								"gicon",
								priv->gnomeShellID,
								(error && error->message) ? error->message : "Unknown error");

					/* Release allocated resources */
					if(error)
					{
						g_error_free(error);
						error=NULL;
					}
				}

				/* Release data extracted for icon */
				g_free(iconString);
			}

			if(g_variant_lookup(metaData, "icon-data", "(iiibiiay)", &iconWidth, &iconHeight, &iconRowstride, &iconHasAlpha, &iconBits, &iconChannels, &iconData))
			{
				/* Create image from icon data */
				iconImage=clutter_image_new();
				if(!clutter_image_set_data(CLUTTER_IMAGE(iconImage), iconData, iconHasAlpha ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888, iconWidth, iconHeight, iconRowstride, &error))
				{
					/* Show error message */
					g_warning("Could get icon for '%s' of key '%s' for Gnome-Shell search provider '%s': %s",
								identifier[0],
								"icon-data",
								priv->gnomeShellID,
								(error && error->message) ? error->message : "Unknown error");

					/* Release allocated resources */
					if(error)
					{
						g_error_free(error);
						error=NULL;
					}
				}

				/* Release data extracted for icon */
				g_free(iconData);
			}

			/* Release allocated resources */
			g_variant_unref(metaData);
		}
	}

	/* Create actor for result item */
	if(name)
	{
		gchar										*buttonText;

		/* Build text to show at button */
		if(description) buttonText=g_markup_printf_escaped("<b>%s</b>\n\n%s", name, description);
			else buttonText=g_markup_printf_escaped("<b>%s</b>", name);

		/* Create actor and set icon if available */
		actor=esdashboard_button_new_with_text(buttonText);

		if(icon)
		{
			esdashboard_label_set_style(ESDASHBOARD_LABEL(actor), ESDASHBOARD_LABEL_STYLE_BOTH);
			esdashboard_label_set_gicon(ESDASHBOARD_LABEL(actor), icon);
		}
			else if(iconImage)
			{
				esdashboard_label_set_style(ESDASHBOARD_LABEL(actor), ESDASHBOARD_LABEL_STYLE_BOTH);
				esdashboard_label_set_icon_image(ESDASHBOARD_LABEL(actor), CLUTTER_IMAGE(iconImage));
			}

		clutter_actor_show(actor);

		/* Release allocated resources */
		g_free(buttonText);
	}

	/* Release allocated resources */
	if(iconImage) g_object_unref(iconImage);
	if(icon) g_object_unref(icon);
	if(description) g_free(description);
	if(name) g_free(name);
	if(resultIter) g_variant_iter_free(resultIter);
	if(proxyResult) g_variant_unref(proxyResult);
	if(proxy) g_object_unref(proxy);

	/* Return created actor */
	return(actor);
}

/* Activate result item */
static gboolean _esdashboard_gnome_shell_search_provider_activate_result(EsdashboardSearchProvider* inProvider,
																			GVariant *inResultItem,
																			ClutterActor *inActor,
																			const gchar **inSearchTerms)
{
	EsdashboardGnomeShellSearchProvider				*self;
	EsdashboardGnomeShellSearchProviderPrivate		*priv;
	GError											*error;
	const gchar										*identifier;
	GDBusProxy										*proxy;
	GVariant										*proxyResult;

	g_return_val_if_fail(ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(inProvider), FALSE);
	g_return_val_if_fail(inResultItem, FALSE);

	self=ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	error=NULL;

	/* Get identifier to activate */
	identifier=g_variant_get_string(inResultItem, NULL);

	/* Call 'ActivateResult' over DBUS at Gnome-Shell search provider */
	proxy=g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
										G_DBUS_PROXY_FLAGS_NONE,
										NULL,
										priv->dbusBusName,
										priv->dbusObjectPath,
										"org.gnome.Shell.SearchProvider2",
										NULL,
										&error);
	if(!proxy)
	{
		/* Show error message */
		g_warning("Could not create dbus connection for Gnome-Shell search provider '%s': %s",
					priv->gnomeShellID,
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(error) g_error_free(error);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	proxyResult=g_dbus_proxy_call_sync(proxy,
										"ActivateResult",
										g_variant_new("(s^asu)",
														identifier,
														inSearchTerms,
														clutter_get_current_event_time()),
										G_DBUS_CALL_FLAGS_NONE,
										-1,
										NULL,
										&error);
	if(!proxyResult)
	{
		/* Show error message */
		g_warning("Could activate result item '%s' over dbus connection for Gnome-Shell search provider '%s': %s",
					identifier,
					priv->gnomeShellID,
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(proxy) g_object_unref(proxy);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Release allocated resources */
	if(proxyResult) g_variant_unref(proxyResult);
	if(proxy) g_object_unref(proxy);

	/* If we get here activating result item was successful, so return TRUE */
	return(TRUE);
}

/* Launch search in external service or application of search provider */
static gboolean _esdashboard_gnome_shell_search_provider_launch_search(EsdashboardSearchProvider* inProvider,
																		const gchar **inSearchTerms)
{
	EsdashboardGnomeShellSearchProvider				*self;
	EsdashboardGnomeShellSearchProviderPrivate		*priv;
	GError											*error;
	GDBusProxy										*proxy;
	GVariant										*proxyResult;

	g_return_val_if_fail(ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(inProvider), FALSE);
	g_return_val_if_fail(inSearchTerms, FALSE);

	self=ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	error=NULL;

	/* Call 'LaunchSearch' over DBUS at Gnome-Shell search provider */
	proxy=g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
										G_DBUS_PROXY_FLAGS_NONE,
										NULL,
										priv->dbusBusName,
										priv->dbusObjectPath,
										"org.gnome.Shell.SearchProvider2",
										NULL,
										&error);
	if(!proxy)
	{
		/* Show error message */
		g_warning("Could not create dbus connection for Gnome-Shell search provider '%s': %s",
					priv->gnomeShellID,
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(error) g_error_free(error);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	proxyResult=g_dbus_proxy_call_sync(proxy,
										"LaunchSearch",
										g_variant_new("(^asu)",
														inSearchTerms,
														clutter_get_current_event_time()),
										G_DBUS_CALL_FLAGS_NONE,
										-1,
										NULL,
										&error);
	if(!proxyResult)
	{
		/* Show error message */
		g_warning("Could not launch search over dbus connection for Gnome-Shell search provider '%s': %s",
					priv->gnomeShellID,
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(proxy) g_object_unref(proxy);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Release allocated resources */
	if(proxyResult) g_variant_unref(proxyResult);
	if(proxy) g_object_unref(proxy);

	/* If we get here launching search was successful, so return TRUE */
	return(TRUE);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_gnome_shell_search_provider_dispose(GObject *inObject)
{
	EsdashboardGnomeShellSearchProvider			*self=ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(inObject);
	EsdashboardGnomeShellSearchProviderPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->gnomeShellID)
	{
		g_free(priv->gnomeShellID);
		priv->gnomeShellID=NULL;
	}

	if(priv->file)
	{
		g_object_unref(priv->file);
		priv->file=NULL;
	}

	if(priv->fileMonitor)
	{
		g_object_unref(priv->fileMonitor);
		priv->fileMonitor=NULL;
	}

	if(priv->desktopID)
	{
		g_free(priv->desktopID);
		priv->desktopID=NULL;
	}

	if(priv->dbusBusName)
	{
		g_free(priv->dbusBusName);
		priv->dbusBusName=NULL;
	}

	if(priv->dbusObjectPath)
	{
		g_free(priv->dbusObjectPath);
		priv->dbusObjectPath=NULL;
	}

	if(priv->providerIcon)
	{
		g_free(priv->providerIcon);
		priv->providerIcon=NULL;
	}

	if(priv->providerName)
	{
		g_free(priv->providerName);
		priv->providerName=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_gnome_shell_search_provider_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_gnome_shell_search_provider_class_init(EsdashboardGnomeShellSearchProviderClass *klass)
{
	EsdashboardSearchProviderClass	*providerClass=ESDASHBOARD_SEARCH_PROVIDER_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_gnome_shell_search_provider_dispose;

	providerClass->initialize=_esdashboard_gnome_shell_search_provider_initialize;
	providerClass->get_icon=_esdashboard_gnome_shell_search_provider_get_icon;
	providerClass->get_name=_esdashboard_gnome_shell_search_provider_get_name;
	providerClass->get_result_set=_esdashboard_gnome_shell_search_provider_get_result_set;
	providerClass->create_result_actor=_esdashboard_gnome_shell_search_provider_create_result_actor;
	providerClass->activate_result=_esdashboard_gnome_shell_search_provider_activate_result;
	providerClass->launch_search=_esdashboard_gnome_shell_search_provider_launch_search;
}

/* Class finalization */
void esdashboard_gnome_shell_search_provider_class_finalize(EsdashboardGnomeShellSearchProviderClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_gnome_shell_search_provider_init(EsdashboardGnomeShellSearchProvider *self)
{
	EsdashboardGnomeShellSearchProviderPrivate		*priv;

	self->priv=priv=esdashboard_gnome_shell_search_provider_get_instance_private(self);

	/* Set up default values */
	priv->gnomeShellID=NULL;
	priv->file=NULL;
	priv->fileMonitor=NULL;
	priv->desktopID=NULL;
	priv->dbusBusName=NULL;
	priv->dbusObjectPath=NULL;
	priv->providerName=NULL;
	priv->providerIcon=NULL;
}
