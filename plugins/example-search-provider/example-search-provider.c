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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "example-search-provider.h"

#include <glib/gi18n-lib.h>
#include <gio/gio.h>


/* Define this class in GObject system */
struct _EsdashboardExampleSearchProviderPrivate
{
	/* Instance related */
	gint			reserved;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(EsdashboardExampleSearchProvider,
								esdashboard_example_search_provider,
								ESDASHBOARD_TYPE_SEARCH_PROVIDER,
								0,
								G_ADD_PRIVATE_DYNAMIC(EsdashboardExampleSearchProvider))

/* Define this class in this plugin */
ESDASHBOARD_DEFINE_PLUGIN_TYPE(esdashboard_example_search_provider);


/* IMPLEMENTATION: EsdashboardSearchProvider */

/* One-time initialization of search provider */
static void _esdashboard_example_search_provider_initialize(EsdashboardSearchProvider *inProvider)
{
	EsdashboardExampleSearchProvider			*self;
	EsdashboardExampleSearchProviderPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider));

	self=ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER(inProvider);
	priv=self->priv;

	/* Initialize search provider */
	/* TODO: Here the search provider is initialized. This function is only called once after
	 *       the search provider was enabled.
	 */
}

/* Get display name for this search provider */
static const gchar* _esdashboard_example_search_provider_get_name(EsdashboardSearchProvider *inProvider)
{
	return(_("Example search"));
}

/* Get icon-name for this search provider */
static const gchar* _esdashboard_example_search_provider_get_icon(EsdashboardSearchProvider *inProvider)
{
	return("edit-find");
}

/* Get result set for requested search terms */
static EsdashboardSearchResultSet* _esdashboard_example_search_provider_get_result_set(EsdashboardSearchProvider *inProvider,
																						const gchar **inSearchTerms,
																						EsdashboardSearchResultSet *inPreviousResultSet)
{
	EsdashboardExampleSearchProvider				*self;
	EsdashboardExampleSearchProviderPrivate			*priv;
	EsdashboardSearchResultSet						*resultSet;
	GVariant										*resultItem;
	gchar											*resultItemTitle;

	g_return_val_if_fail(ESDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider), NULL);

	self=ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	resultSet=NULL;

	/* Create empty result set to store matching result items */
	resultSet=esdashboard_search_result_set_new();

	/* Create result item */
	/* TOOO: This example just creates one long string from entered search terms
	 *       as search result. More complex data are possible as GVariant is used
	 *       in result set for each result item.
	 */
	resultItemTitle=g_strjoinv(" ", (gchar**)inSearchTerms);
	resultItem=g_variant_new_string(resultItemTitle);
	g_free(resultItemTitle);

	/* Add result item to result set */
	esdashboard_search_result_set_add_item(resultSet, resultItem);
	/* TODO: This example search provider assumes that each result item is a
	 *       full match and sets a score of 1. This score is used to determine
	 *       the relevance of this result item against the search terms entered.
	 *       The score must be a float value between 0.0f and 1.0f.
	 */
	esdashboard_search_result_set_set_item_score(resultSet, resultItem, 1.0f);

	/* Return result set */
	return(resultSet);
}

/* Create actor for a result item of the result set returned from a search request */
static ClutterActor* _esdashboard_example_search_provider_create_result_actor(EsdashboardSearchProvider *inProvider,
																				GVariant *inResultItem)
{
	EsdashboardExampleSearchProvider				*self;
	EsdashboardExampleSearchProviderPrivate			*priv;
	ClutterActor									*actor;
	gchar											*title;

	g_return_val_if_fail(ESDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider), NULL);
	g_return_val_if_fail(inResultItem, NULL);

	self=ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	actor=NULL;

	/* Create actor for result item */
	/* TODO: This example search provider will just create a button with a title
	 *       taken from the result item. More complex actors are possible.
	 */
	title=g_markup_printf_escaped("<b>%s</b>\n\nSearch for '%s' with search provider plugin '%s'", g_variant_get_string(inResultItem, NULL), g_variant_get_string(inResultItem, NULL), PLUGIN_ID);
	actor=esdashboard_button_new_with_text(title);
	g_free(title);

	/* Return created actor */
	return(actor);
}

/* Activate result item */
static gboolean _esdashboard_example_search_provider_activate_result(EsdashboardSearchProvider* inProvider,
																		GVariant *inResultItem,
																		ClutterActor *inActor,
																		const gchar **inSearchTerms)
{
	EsdashboardExampleSearchProvider				*self;
	EsdashboardExampleSearchProviderPrivate			*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider), FALSE);
	g_return_val_if_fail(inResultItem, FALSE);

	self=ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	
	/* Activate result item */
	/* TODO: Here you have to perform the default action when a result item of
	 *       this search provider was activated, i.e. clicked.
	 */

	/* If we get here activating result item was successful, so return TRUE */
	return(TRUE);
}

/* Launch search in external service or application of search provider */
static gboolean _esdashboard_example_search_provider_launch_search(EsdashboardSearchProvider* inProvider,
																	const gchar **inSearchTerms)
{
	EsdashboardExampleSearchProvider				*self;
	EsdashboardExampleSearchProviderPrivate			*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider), FALSE);
	g_return_val_if_fail(inSearchTerms, FALSE);

	self=ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER(inProvider);
	priv=self->priv;

	/* Launch selected result item */
	/* TODO: Here you have to launch the application or other external services
	 *       when the provider icon of this search provider was clicked.
	 */

	/* If we get here launching search was successful, so return TRUE */
	return(TRUE);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_example_search_provider_dispose(GObject *inObject)
{
	EsdashboardExampleSearchProvider			*self=ESDASHBOARD_EXAMPLE_SEARCH_PROVIDER(inObject);
	EsdashboardExampleSearchProviderPrivate		*priv=self->priv;

	/* Release allocated resources */
	/* TODO: Release data in private instance if any */

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_example_search_provider_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_example_search_provider_class_init(EsdashboardExampleSearchProviderClass *klass)
{
	EsdashboardSearchProviderClass	*providerClass=ESDASHBOARD_SEARCH_PROVIDER_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_example_search_provider_dispose;

	providerClass->initialize=_esdashboard_example_search_provider_initialize;
	providerClass->get_icon=_esdashboard_example_search_provider_get_icon;
	providerClass->get_name=_esdashboard_example_search_provider_get_name;
	providerClass->get_result_set=_esdashboard_example_search_provider_get_result_set;
	providerClass->create_result_actor=_esdashboard_example_search_provider_create_result_actor;
	providerClass->activate_result=_esdashboard_example_search_provider_activate_result;
	providerClass->launch_search=_esdashboard_example_search_provider_launch_search;
}

/* Class finalization */
void esdashboard_example_search_provider_class_finalize(EsdashboardExampleSearchProviderClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_example_search_provider_init(EsdashboardExampleSearchProvider *self)
{
	EsdashboardExampleSearchProviderPrivate		*priv;

	self->priv=priv=esdashboard_example_search_provider_get_instance_private(self);

	/* Set up default values */
	/* TODO: Set up default value in private instance data if any */
}
