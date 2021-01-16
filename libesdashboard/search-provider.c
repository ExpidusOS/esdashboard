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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/search-provider.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/actor.h>
#include <libesdashboard/text-box.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
struct _EsdashboardSearchProviderPrivate
{
	/* Properties related */
	gchar					*providerID;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(EsdashboardSearchProvider,
									esdashboard_search_provider,
									G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_PROVIDER_ID,

	PROP_LAST
};

static GParamSpec* EsdashboardSearchProviderProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define ESDASHBOARD_SEARCH_PROVIDER_WARN_NOT_IMPLEMENTED(self, vfunc) \
	g_warning("Search provider of type %s does not implement required virtual function EsdashboardSearchProvider::%s",\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

#define ESDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, vfunc) \
	ESDASHBOARD_DEBUG(self, MISC,                                              \
						"Search provider of type %s does not implement virtual function EsdashboardSearchProvider::%s",\
						G_OBJECT_TYPE_NAME(self),                              \
						vfunc);

/* Set search provider ID */
static void _esdashboard_search_provider_set_id(EsdashboardSearchProvider *self, const gchar *inID)
{
	EsdashboardSearchProviderPrivate	*priv=self->priv;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_PROVIDER(self));
	g_return_if_fail(inID && *inID);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->providerID, inID)!=0)
	{
		if(priv->providerID) g_free(priv->providerID);
		priv->providerID=g_strdup(inID);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardSearchProviderProperties[PROP_PROVIDER_ID]);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_search_provider_dispose(GObject *inObject)
{
	EsdashboardSearchProvider			*self=ESDASHBOARD_SEARCH_PROVIDER(inObject);
	EsdashboardSearchProviderPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->providerID)
	{
		g_free(priv->providerID);
		priv->providerID=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_search_provider_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_search_provider_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	EsdashboardSearchProvider			*self=ESDASHBOARD_SEARCH_PROVIDER(inObject);

	switch(inPropID)
	{
		case PROP_PROVIDER_ID:
			_esdashboard_search_provider_set_id(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_search_provider_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	EsdashboardSearchProvider			*self=ESDASHBOARD_SEARCH_PROVIDER(inObject);

	switch(inPropID)
	{
		case PROP_PROVIDER_ID:
			g_value_set_string(outValue, self->priv->providerID);
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
static void esdashboard_search_provider_class_init(EsdashboardSearchProviderClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_esdashboard_search_provider_set_property;
	gobjectClass->get_property=_esdashboard_search_provider_get_property;
	gobjectClass->dispose=_esdashboard_search_provider_dispose;

	/* Define properties */
	EsdashboardSearchProviderProperties[PROP_PROVIDER_ID]=
		g_param_spec_string("provider-id",
							"Provider ID",
							"The internal ID used to register this type of search provider",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardSearchProviderProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_search_provider_init(EsdashboardSearchProvider *self)
{
	EsdashboardSearchProviderPrivate	*priv;

	priv=self->priv=esdashboard_search_provider_get_instance_private(self);

	/* Set up default values */
	priv->providerID=NULL;
}

/* IMPLEMENTATION: Public API */

/* Get view ID */
const gchar* esdashboard_search_provider_get_id(EsdashboardSearchProvider *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);

	return(self->priv->providerID);
}

/* Check if view has requested ID */
gboolean esdashboard_search_provider_has_id(EsdashboardSearchProvider *self, const gchar *inID)
{
	EsdashboardSearchProviderPrivate	*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	priv=self->priv;

	/* Check if requested ID matches the ID of this search provider */
	if(g_strcmp0(priv->providerID, inID)!=0) return(FALSE);

	/* If we get here the requested ID matches search provider's ID */
	return(TRUE);
}

/* Get name of search provider */
const gchar* esdashboard_search_provider_get_name(EsdashboardSearchProvider *self)
{
	EsdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);

	klass=ESDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Return name of search provider */
	if(klass->get_name)
	{
		return(klass->get_name(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_SEARCH_PROVIDER_WARN_NOT_IMPLEMENTED(self, "get_name");
	return(G_OBJECT_TYPE_NAME(self));
}

/* Get icon name of search provider */
const gchar* esdashboard_search_provider_get_icon(EsdashboardSearchProvider *self)
{
	EsdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);

	klass=ESDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Return icon of search provider */
	if(klass->get_icon)
	{
		return(klass->get_icon(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, "get_icon");
	return(NULL);
}

/* Get result set for list of search terms from search provider. If a previous result set
 * is provided do an incremental search on basis of provided result set. The returned
 * result set must be a new allocated object and its entries must already be sorted
 * in order in which they should be displayed.
 */
EsdashboardSearchResultSet* esdashboard_search_provider_get_result_set(EsdashboardSearchProvider *self,
																		const gchar **inSearchTerms,
																		EsdashboardSearchResultSet *inPreviousResultSet)
{
	EsdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);
	g_return_val_if_fail(inSearchTerms, NULL);
	g_return_val_if_fail(!inPreviousResultSet || ESDASHBOARD_IS_SEARCH_RESULT_SET(inPreviousResultSet), NULL);

	klass=ESDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Return result set of search provider */
	if(klass->get_result_set)
	{
		return(klass->get_result_set(self, inSearchTerms, inPreviousResultSet));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_SEARCH_PROVIDER_WARN_NOT_IMPLEMENTED(self, "get_result_set");
	return(NULL);
}

/* Returns an actor for requested result item */
ClutterActor* esdashboard_search_provider_create_result_actor(EsdashboardSearchProvider *self,
																GVariant *inResultItem)
{
	EsdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);
	g_return_val_if_fail(inResultItem, NULL);

	klass=ESDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Return actor created by search provider */
	if(klass->create_result_actor)
	{
		return(klass->create_result_actor(self, inResultItem));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_SEARCH_PROVIDER_WARN_NOT_IMPLEMENTED(self, "create_result_actor");
	return(NULL);
}

/* Launch search in external service or application the search provider relies on
 * with provided list of search terms.
 */
gboolean esdashboard_search_provider_launch_search(EsdashboardSearchProvider *self,
													const gchar **inSearchTerms)
{
	EsdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(inSearchTerms, FALSE);

	klass=ESDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Launch search by search provider */
	if(klass->launch_search)
	{
		return(klass->launch_search(self, inSearchTerms));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, "launch_search");
	return(FALSE);
}

/* A result item actor was clicked so ask search provider to handle it */
gboolean esdashboard_search_provider_activate_result(EsdashboardSearchProvider* self,
														GVariant *inResultItem,
														ClutterActor *inActor,
														const gchar **inSearchTerms)
{
	EsdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(inResultItem, FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), FALSE);
	g_return_val_if_fail(inSearchTerms, FALSE);

	klass=ESDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Handle click action at result item actor by search provider */
	if(klass->activate_result)
	{
		return(klass->activate_result(self, inResultItem, inActor, inSearchTerms));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, "activate_result");
	return(FALSE);
}
