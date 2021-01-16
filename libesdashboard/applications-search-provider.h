/*
 * applications-search-provider: Search provider for searching installed
 *                               applications
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

#ifndef __LIBESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER__
#define __LIBESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/search-provider.h>

G_BEGIN_DECLS

/* Public definitions */
typedef enum /*< flags,prefix=ESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE >*/
{
	ESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NONE=0,

	ESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NAMES=1 << 0,
	ESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_MOST_USED=1 << 1,
} EsdashboardApplicationsSearchProviderSortMode;


/* Object declaration */
#define ESDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER				(esdashboard_applications_search_provider_get_type())
#define ESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER, EsdashboardApplicationsSearchProvider))
#define ESDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER))
#define ESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER, EsdashboardApplicationsSearchProviderClass))
#define ESDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER))
#define ESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER, EsdashboardApplicationsSearchProviderClass))

typedef struct _EsdashboardApplicationsSearchProvider				EsdashboardApplicationsSearchProvider; 
typedef struct _EsdashboardApplicationsSearchProviderPrivate		EsdashboardApplicationsSearchProviderPrivate;
typedef struct _EsdashboardApplicationsSearchProviderClass			EsdashboardApplicationsSearchProviderClass;

struct _EsdashboardApplicationsSearchProvider
{
	/*< private >*/
	/* Parent instance */
	EsdashboardSearchProvider						parent_instance;

	/* Private structure */
	EsdashboardApplicationsSearchProviderPrivate	*priv;
};

struct _EsdashboardApplicationsSearchProviderClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardSearchProviderClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};


/* Public API */
GType esdashboard_applications_search_provider_get_type(void) G_GNUC_CONST;

EsdashboardApplicationsSearchProviderSortMode esdashboard_applications_search_provider_get_sort_mode(EsdashboardApplicationsSearchProvider *self);
void esdashboard_applications_search_provider_set_sort_mode(EsdashboardApplicationsSearchProvider *self, const EsdashboardApplicationsSearchProviderSortMode inMode);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_APPLICATIONS_SEARCH_PROVIDER__ */
