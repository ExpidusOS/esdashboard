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

#ifndef __ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER__
#define __ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER__

#include <libesdashboard/libesdashboard.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER			(esdashboard_gnome_shell_search_provider_get_type())
#define ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER, EsdashboardGnomeShellSearchProvider))
#define ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER))
#define ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER, EsdashboardGnomeShellSearchProviderClass))
#define ESDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER))
#define ESDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER, EsdashboardGnomeShellSearchProviderClass))

typedef struct _EsdashboardGnomeShellSearchProvider				EsdashboardGnomeShellSearchProvider; 
typedef struct _EsdashboardGnomeShellSearchProviderPrivate		EsdashboardGnomeShellSearchProviderPrivate;
typedef struct _EsdashboardGnomeShellSearchProviderClass		EsdashboardGnomeShellSearchProviderClass;

struct _EsdashboardGnomeShellSearchProvider
{
	/* Parent instance */
	EsdashboardSearchProvider					parent_instance;

	/* Private structure */
	EsdashboardGnomeShellSearchProviderPrivate	*priv;
};

struct _EsdashboardGnomeShellSearchProviderClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardSearchProviderClass				parent_class;
};

/* Public API */
GType esdashboard_gnome_shell_search_provider_get_type(void) G_GNUC_CONST;
void esdashboard_gnome_shell_search_provider_type_register(GTypeModule *inModule);

ESDASHBOARD_DECLARE_PLUGIN_TYPE(esdashboard_gnome_shell_search_provider);

G_END_DECLS

#endif
