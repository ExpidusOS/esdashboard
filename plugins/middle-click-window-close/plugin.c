/*
 * plugin: Plugin functions for 'middle-click-window-close'
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

#include <libexpidus1util/libexpidus1util.h>
#include <gtk/gtk.h>

#include "middle-click-window-close.h"


/* Forward declarations */
G_MODULE_EXPORT void plugin_init(EsdashboardPlugin *self);


/* IMPLEMENTATION: EsdashboardPlugin */

static EsdashboardMiddleClickWindowClose			*middleClickWindowClose=NULL;

/* Plugin enable function */
static void plugin_enable(EsdashboardPlugin *self, gpointer inUserData)
{
	/* Create instance of hot corner */
	if(!middleClickWindowClose)
	{
		middleClickWindowClose=esdashboard_middle_click_window_close_new();
	}
}

/* Plugin disable function */
static void plugin_disable(EsdashboardPlugin *self, gpointer inUserData)
{
	/* Destroy instance of hot corner */
	if(middleClickWindowClose)
	{
		g_object_unref(middleClickWindowClose);
		middleClickWindowClose=NULL;
	}
}

/* Plugin initialization function */
G_MODULE_EXPORT void plugin_init(EsdashboardPlugin *self)
{
	/* Set up localization */
	expidus_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* Set plugin info */
	esdashboard_plugin_set_info(self,
								"name", _("Middle-click window close"),
								"description", _("Closes windows in windows view by middle-click"),
								"author", "Stephan Haller <nomad@froevel.de>",
								NULL);

	/* Register GObject types of this plugin */
	ESDASHBOARD_REGISTER_PLUGIN_TYPE(self, esdashboard_middle_click_window_close);

	/* Connect plugin action handlers */
	g_signal_connect(self, "enable", G_CALLBACK(plugin_enable), NULL);
	g_signal_connect(self, "disable", G_CALLBACK(plugin_disable), NULL);
}
