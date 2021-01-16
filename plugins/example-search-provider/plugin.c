/*
 * plugin: Plugin functions for 'example-search-provider'
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
#include <config.h>
#endif

#include <libesdashboard/libesdashboard.h>
#include <libexpidus1util/libexpidus1util.h>

#include "example-search-provider.h"


/* Forward declarations */
G_MODULE_EXPORT void plugin_init(EsdashboardPlugin *self);


/* IMPLEMENTATION: EsdashboardPlugin */

/* Plugin enable function */
static void plugin_enable(EsdashboardPlugin *self, gpointer inUserData)
{
	EsdashboardSearchManager		*searchManager;

	/* Register search provider */
	searchManager=esdashboard_search_manager_get_default();

	esdashboard_search_manager_register(searchManager, PLUGIN_ID, ESDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER);

	g_object_unref(searchManager);
}

/* Plugin disable function */
static void plugin_disable(EsdashboardPlugin *self, gpointer inUserData)
{
	EsdashboardSearchManager		*searchManager;

	/* Register search provider */
	searchManager=esdashboard_search_manager_get_default();

	esdashboard_search_manager_unregister(searchManager, PLUGIN_ID);

	g_object_unref(searchManager);
}

/* Plugin initialization function */
G_MODULE_EXPORT void plugin_init(EsdashboardPlugin *self)
{
	/* Set up localization */
	expidus_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* Set plugin info */
	esdashboard_plugin_set_info(self,
								"flags", ESDASHBOARD_PLUGIN_FLAG_EARLY_INITIALIZATION,
								"name", _("Example search provider"),
								"description", _("This is just a useless example search provider plugin"),
								"author", "Stephan Haller <nomad@froevel.de>",
								NULL);

	/* Register GObject types of this plugin */
	ESDASHBOARD_REGISTER_PLUGIN_TYPE(self, esdashboard_example_search_provider);

	/* Connect plugin action handlers */
	g_signal_connect(self, "enable", G_CALLBACK(plugin_enable), NULL);
	g_signal_connect(self, "disable", G_CALLBACK(plugin_disable), NULL);
}
