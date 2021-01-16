/*
 * search-provider: Abstract class for search providers
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

#ifndef __LIBESDASHBOARD_SEARCH_PROVIDER__
#define __LIBESDASHBOARD_SEARCH_PROVIDER__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/search-result-set.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SEARCH_PROVIDER				(esdashboard_search_provider_get_type())
#define ESDASHBOARD_SEARCH_PROVIDER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SEARCH_PROVIDER, EsdashboardSearchProvider))
#define ESDASHBOARD_IS_SEARCH_PROVIDER(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SEARCH_PROVIDER))
#define ESDASHBOARD_SEARCH_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SEARCH_PROVIDER, EsdashboardSearchProviderClass))
#define ESDASHBOARD_IS_SEARCH_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SEARCH_PROVIDER))
#define ESDASHBOARD_SEARCH_PROVIDER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SEARCH_PROVIDER, EsdashboardSearchProviderClass))

typedef struct _EsdashboardSearchProvider				EsdashboardSearchProvider; 
typedef struct _EsdashboardSearchProviderPrivate		EsdashboardSearchProviderPrivate;
typedef struct _EsdashboardSearchProviderClass			EsdashboardSearchProviderClass;

struct _EsdashboardSearchProvider
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardSearchProviderPrivate	*priv;
};

struct _EsdashboardSearchProviderClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*initialize)(EsdashboardSearchProvider *self);

	const gchar* (*get_name)(EsdashboardSearchProvider *self);
	const gchar* (*get_icon)(EsdashboardSearchProvider *self);

	EsdashboardSearchResultSet* (*get_result_set)(EsdashboardSearchProvider *self,
													const gchar **inSearchTerms,
													EsdashboardSearchResultSet *inPreviousResultSet);

	ClutterActor* (*create_result_actor)(EsdashboardSearchProvider *self,
											GVariant *inResultItem);

	gboolean (*launch_search)(EsdashboardSearchProvider *self,
								const gchar **inSearchTerms);

	gboolean (*activate_result)(EsdashboardSearchProvider* self,
								GVariant *inResultItem,
								ClutterActor *inActor,
								const gchar **inSearchTerms);
};

/* Public API */
GType esdashboard_search_provider_get_type(void) G_GNUC_CONST;

const gchar* esdashboard_search_provider_get_id(EsdashboardSearchProvider *self);
gboolean esdashboard_search_provider_has_id(EsdashboardSearchProvider *self, const gchar *inID);

const gchar* esdashboard_search_provider_get_name(EsdashboardSearchProvider *self);
const gchar* esdashboard_search_provider_get_icon(EsdashboardSearchProvider *self);

EsdashboardSearchResultSet* esdashboard_search_provider_get_result_set(EsdashboardSearchProvider *self,
																		const gchar **inSearchTerms,
																		EsdashboardSearchResultSet *inPreviousResultSet);

ClutterActor* esdashboard_search_provider_create_result_actor(EsdashboardSearchProvider *self,
																GVariant *inResultItem);

gboolean esdashboard_search_provider_launch_search(EsdashboardSearchProvider *self,
													const gchar **inSearchTerms);

gboolean esdashboard_search_provider_activate_result(EsdashboardSearchProvider* self,
														GVariant *inResultItem,
														ClutterActor *inActor,
														const gchar **inSearchTerms);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_SEARCH_PROVIDER__ */
