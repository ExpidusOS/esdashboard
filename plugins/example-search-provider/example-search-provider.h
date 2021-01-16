/*
 * example-search-provider: An example search provider
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

#ifndef __ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER__
#define __ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER__

#include <libesdashboard/libesdashboard.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER			(esdashboard_example_search_provider_get_type())
#define ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER, EsdashboardExampleSearchProvider))
#define ESDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER))
#define ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER, EsdashboardExampleSearchProviderClass))
#define ESDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER))
#define ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER, EsdashboardExampleSearchProviderClass))

typedef struct _EsdashboardExampleSearchProvider			EsdashboardExampleSearchProvider; 
typedef struct _EsdashboardExampleSearchProviderPrivate		EsdashboardExampleSearchProviderPrivate;
typedef struct _EsdashboardExampleSearchProviderClass		EsdashboardExampleSearchProviderClass;

struct _EsdashboardExampleSearchProvider
{
	/* Parent instance */
	EsdashboardSearchProvider					parent_instance;

	/* Private structure */
	EsdashboardExampleSearchProviderPrivate		*priv;
};

struct _EsdashboardExampleSearchProviderClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardSearchProviderClass				parent_class;
};

/* Public API */
GType esdashboard_example_search_provider_get_type(void) G_GNUC_CONST;
void esdashboard_example_search_provider_type_register(GTypeModule *inModule);

ESDASHBOARD_DECLARE_PLUGIN_TYPE(esdashboard_example_search_provider);

G_END_DECLS

#endif
