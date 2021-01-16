/*
 * search-view: A view showing applications matching search criteria
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

/**
 * SECTION:search-view
 * @short_description: A view showing results for a search of requested search terms
 * @include: esdashboard/search-view.h
 *
 * This view #EsdashboardSearchView is a view used to show the results of a search.
 * It requests all registered and enabled search providers to return a result set
 * for the search term provided with esdashboard_search_view_update_search(). For
 * each item in the result set this view will requests an actor at the associated
 * search provider to display that result item.
 *
 * To clear the results and to stop further searches the function
 * esdashboard_search_view_reset_search() should be called. Usually the application
 * will also switch back to active view before the search was started.
 *
 * <note><para>
 * This view is an internal view and registered by the core of the application.
 * You should not register an additional instance of this view at #EsdashboardViewManager.
 * </para></note>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/search-view.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <libesdashboard/search-manager.h>
#include <libesdashboard/focusable.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/search-result-container.h>
#include <libesdashboard/focus-manager.h>
#include <libesdashboard/enums.h>
#include <libesdashboard/application.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Forward declarations */
typedef struct _EsdashboardSearchViewProviderData	EsdashboardSearchViewProviderData;
typedef struct _EsdashboardSearchViewSearchTerms	EsdashboardSearchViewSearchTerms;

/* Define this class in GObject system */
static void _esdashboard_search_view_focusable_iface_init(EsdashboardFocusableInterface *iface);

struct _EsdashboardSearchViewPrivate
{
	/* Instance related */
	EsdashboardSearchManager			*searchManager;
	GList								*providers;

	EsdashboardSearchViewSearchTerms	*lastTerms;

	EsconfChannel						*esconfChannel;
	gboolean							delaySearch;
	EsdashboardSearchViewSearchTerms	*delaySearchTerms;
	gint								delaySearchTimeoutID;

	EsdashboardSearchViewProviderData	*selectionProvider;
	guint								repaintID;

	EsdashboardFocusManager				*focusManager;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardSearchView,
						esdashboard_search_view,
						ESDASHBOARD_TYPE_VIEW,
						G_ADD_PRIVATE(EsdashboardSearchView)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_FOCUSABLE, _esdashboard_search_view_focusable_iface_init))

/* Signals */
enum
{
	SIGNAL_SEARCH_RESET,
	SIGNAL_SEARCH_UPDATED,

	SIGNAL_LAST
};

static guint EsdashboardSearchViewSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DELAY_SEARCH_TIMEOUT_ESCONF_PROP		"/components/search-view/delay-search-timeout"
#define DEFAULT_DELAY_SEARCH_TIMEOUT			0

struct _EsdashboardSearchViewProviderData
{
	gint								refCount;

	EsdashboardSearchProvider			*provider;

	EsdashboardSearchView				*view;
	EsdashboardSearchViewSearchTerms	*lastTerms;
	EsdashboardSearchResultSet			*lastResultSet;

	ClutterActor						*container;
};

struct _EsdashboardSearchViewSearchTerms
{
	gint								refCount;

	gchar								*termString;
	gchar								**termList;
};

/* Callback to ensure current selection is visible after search results were updated */
static gboolean _esdashboard_search_view_on_repaint_after_update_callback(gpointer inUserData)
{
	EsdashboardSearchView					*self;
	EsdashboardSearchViewPrivate			*priv;
	ClutterActor							*selection;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(inUserData), G_SOURCE_REMOVE);

	self=ESDASHBOARD_SEARCH_VIEW(inUserData);
	priv=self->priv;

	/* Check if this view has a selection set and ensure it is visible */
	selection=esdashboard_focusable_get_selection(ESDASHBOARD_FOCUSABLE(self));
	if(selection)
	{
		/* Ensure selection is visible */
		esdashboard_view_child_ensure_visible(ESDASHBOARD_VIEW(self), selection);
	}

	/* Do not call this callback again */
	priv->repaintID=0;
	return(G_SOURCE_REMOVE);
}

/* Create search term data for a search string */
static EsdashboardSearchViewSearchTerms* _esdashboard_search_view_search_terms_new(const gchar *inSearchString)
{
	EsdashboardSearchViewSearchTerms	*data;

	/* Create data for provider */
	data=g_new0(EsdashboardSearchViewSearchTerms, 1);
	data->refCount=1;
	data->termString=g_strdup(inSearchString);
	data->termList=esdashboard_search_manager_get_search_terms_from_string(inSearchString, NULL);

	return(data);
}

/* Free search term data */
static void _esdashboard_search_view_search_terms_free(EsdashboardSearchViewSearchTerms *inData)
{
	g_return_if_fail(inData);

#if DEBUG
	/* Print a critical warning if more than one references to this object exist.
	 * This is a debug message and should not be translated.
	 */
	if(inData->refCount>1)
	{
		g_critical("Freeing EsdashboardSearchViewSearchTerms at %p with %d references",
					inData,
					inData->refCount);
	}
#endif

	/* Release allocated resources */
	if(inData->termList) g_strfreev(inData->termList);
	if(inData->termString) g_free(inData->termString);
	g_free(inData);
}

/* Increase/decrease reference count for search term data */
static EsdashboardSearchViewSearchTerms* _esdashboard_search_view_search_terms_ref(EsdashboardSearchViewSearchTerms *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;

	return(inData);
}

static void _esdashboard_search_view_search_terms_unref(EsdashboardSearchViewSearchTerms *inData)
{
	g_return_if_fail(inData);
	g_return_if_fail(inData->refCount>0);

	inData->refCount--;
	if(inData->refCount==0) _esdashboard_search_view_search_terms_free(inData);
}

/* Create data for provider */
static EsdashboardSearchViewProviderData* _esdashboard_search_view_provider_data_new(EsdashboardSearchView *self,
																						const gchar *inProviderID)
{
	EsdashboardSearchViewPrivate		*priv;
	EsdashboardSearchViewProviderData	*data;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self), NULL);
	g_return_val_if_fail(*inProviderID && *inProviderID, NULL);

	priv=self->priv;

	/* Create data for provider */
	data=g_new0(EsdashboardSearchViewProviderData, 1);
	data->refCount=1;
	data->provider=ESDASHBOARD_SEARCH_PROVIDER(esdashboard_search_manager_create_provider(priv->searchManager, inProviderID));
	data->view=self;
	data->lastTerms=NULL;
	data->lastResultSet=NULL;
	data->container=NULL;

	return(data);
}

/* Free data for provider */
static void _esdashboard_search_view_provider_data_free(EsdashboardSearchViewProviderData *inData)
{
	g_return_if_fail(inData);

#if DEBUG
	/* Print a critical warning if more than one references to this object exist.
	 * This is a debug message and should not be translated.
	 */
	if(inData->refCount>1)
	{
		g_critical("Freeing EsdashboardSearchViewProviderData at %p with %d references",
					inData,
					inData->refCount);
	}
#endif

	/* Destroy container */
	if(inData->container)
	{
		/* First disconnect signal handlers from actor before destroying container */
		g_signal_handlers_disconnect_by_data(inData->container, inData);

		/* Destroy container */
		esdashboard_actor_destroy(inData->container);
		inData->container=NULL;
	}

	/* Release allocated resources */
	if(inData->lastResultSet) g_object_unref(inData->lastResultSet);
	if(inData->lastTerms) _esdashboard_search_view_search_terms_unref(inData->lastTerms);
	if(inData->provider) g_object_unref(inData->provider);
	g_free(inData);
}

/* Increase/decrease reference count for data of requested provider */
static EsdashboardSearchViewProviderData* _esdashboard_search_view_provider_data_ref(EsdashboardSearchViewProviderData *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;

	return(inData);
}

static void _esdashboard_search_view_provider_data_unref(EsdashboardSearchViewProviderData *inData)
{
	g_return_if_fail(inData);
	g_return_if_fail(inData->refCount>0);

	inData->refCount--;
	if(inData->refCount==0) _esdashboard_search_view_provider_data_free(inData);
}

/* Find data for requested provider type */
static EsdashboardSearchViewProviderData* _esdashboard_search_view_get_provider_data(EsdashboardSearchView *self,
																						const gchar *inProviderID)
{
	EsdashboardSearchViewPrivate		*priv;
	EsdashboardSearchViewProviderData	*data;
	GList								*iter;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self), NULL);
	g_return_val_if_fail(inProviderID && *inProviderID, NULL);

	priv=self->priv;

	/* Iterate through list of provider data and lookup requested one */
	for(iter=priv->providers; iter; iter=g_list_next(iter))
	{
		data=(EsdashboardSearchViewProviderData*)iter->data;

		if(data->provider &&
			esdashboard_search_provider_has_id(data->provider, inProviderID))
		{
			return(_esdashboard_search_view_provider_data_ref(data));
		}
	}

	/* If we get here we did not find data for requested provider */
	return(NULL);
}

/* Find data of provider by its child actor */
static EsdashboardSearchViewProviderData* _esdashboard_search_view_get_provider_data_by_actor(EsdashboardSearchView *self,
																								ClutterActor *inChild)
{
	EsdashboardSearchViewPrivate		*priv;
	EsdashboardSearchViewProviderData	*data;
	GList								*iter;
	ClutterActor						*container;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inChild), NULL);

	priv=self->priv;

	/* Find container for requested child */
	container=inChild;
	while(container && !ESDASHBOARD_IS_SEARCH_RESULT_CONTAINER(container))
	{
		/* Current child is not a container so try next with parent actor */
		container=clutter_actor_get_parent(container);
	}

	if(!container)
	{
		/* Container for requested child was not found */
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Did not find container for actor %p of type %s",
							inChild,
							G_OBJECT_TYPE_NAME(inChild));

		return(NULL);
	}

	/* Iterate through list of provider data and lookup found container */
	for(iter=priv->providers; iter; iter=g_list_next(iter))
	{
		data=(EsdashboardSearchViewProviderData*)iter->data;

		if(data->provider &&
			data->container==container)
		{
			return(_esdashboard_search_view_provider_data_ref(data));
		}
	}

	/* If we get here we did not find data of provider for requested actor */
	return(NULL);
}

/* A search provider was registered */
static void _esdashboard_search_view_on_search_provider_registered(EsdashboardSearchView *self,
																	const gchar *inProviderID,
																	gpointer inUserData)
{
	EsdashboardSearchViewPrivate		*priv;
	EsdashboardSearchViewProviderData	*data;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self));
	g_return_if_fail(*inProviderID && *inProviderID);

	priv=self->priv;

	/* Register search provider if not already registered */
	data=_esdashboard_search_view_get_provider_data(self, inProviderID);
	if(!data)
	{
		/* Create data for new search provider registered
		 * and add to list of active search providers.
		 */
		data=_esdashboard_search_view_provider_data_new(self, inProviderID);
		priv->providers=g_list_append(priv->providers, data);

		ESDASHBOARD_DEBUG(self, MISC,
							"Created search provider %s of type %s in %s",
							esdashboard_search_provider_get_name(data->provider),
							G_OBJECT_TYPE_NAME(data->provider),
							G_OBJECT_TYPE_NAME(self));
	}
		else _esdashboard_search_view_provider_data_unref(data);
}

/* A search provider was unregistered */
static void _esdashboard_search_view_on_search_provider_unregistered(EsdashboardSearchView *self,
																		const gchar *inProviderID,
																		gpointer inUserData)
{
	EsdashboardSearchViewPrivate		*priv;
	EsdashboardSearchViewProviderData	*data;
	GList								*iter;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self));
	g_return_if_fail(*inProviderID && *inProviderID);

	priv=self->priv;

	/* Unregister search provider if it was registered before */
	data=_esdashboard_search_view_get_provider_data(self, inProviderID);
	if(data)
	{
		ESDASHBOARD_DEBUG(self, MISC,
							"Unregistering search provider %s of type %s in %s",
							esdashboard_search_provider_get_name(data->provider),
							G_OBJECT_TYPE_NAME(data->provider),
							G_OBJECT_TYPE_NAME(self));

		/* Find data of unregistered search provider in list of
		 * active search providers to remove it from that list.
		 */
		iter=g_list_find(priv->providers, data);
		if(iter) priv->providers=g_list_delete_link(priv->providers, iter);

		/* Free provider data */
		_esdashboard_search_view_provider_data_unref(data);
	}
}

/* A result item actor was clicked */
static void _esdashboard_search_view_on_result_item_clicked(EsdashboardSearchResultContainer *inContainer,
															GVariant *inItem,
															ClutterActor *inActor,
															gpointer inUserData)
{
	EsdashboardSearchView				*self;
	EsdashboardSearchViewPrivate		*priv;
	EsdashboardSearchViewProviderData	*providerData;
	const gchar							**searchTerms;
	gboolean							success;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inContainer));
	g_return_if_fail(inItem);
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inUserData);

	providerData=(EsdashboardSearchViewProviderData*)inUserData;

	/* Get search view and private data of view */
	self=providerData->view;
	priv=self->priv;

	/* Get search terms to pass them to search provider */
	searchTerms=NULL;
	if(priv->lastTerms) searchTerms=(const gchar**)priv->lastTerms->termList;

	/* Tell provider to launch search */
	success=esdashboard_search_provider_activate_result(providerData->provider,
														inItem,
														inActor,
														searchTerms);
	if(success)
	{
		/* Activating result item seems to be successfuly so quit application */
		esdashboard_application_suspend_or_quit(NULL);
	}
}

/* A provider icon was clicked */
static void _esdashboard_search_view_on_provider_icon_clicked(EsdashboardSearchResultContainer *inContainer,
																gpointer inUserData)
{
	EsdashboardSearchView				*self;
	EsdashboardSearchViewPrivate		*priv;
	EsdashboardSearchViewProviderData	*providerData;
	const gchar							**searchTerms;
	gboolean							success;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inContainer));
	g_return_if_fail(inUserData);

	providerData=(EsdashboardSearchViewProviderData*)inUserData;

	/* Get search view and private data of view */
	self=providerData->view;
	priv=self->priv;

	/* Get search terms to pass them to search provider */
	searchTerms=NULL;
	if(priv->lastTerms) searchTerms=(const gchar**)priv->lastTerms->termList;

	/* Tell provider to launch search */
	success=esdashboard_search_provider_launch_search(providerData->provider, searchTerms);
	if(success)
	{
		/* Activating result item seems to be successfuly so quit application */
		esdashboard_application_suspend_or_quit(NULL);
	}
}

/* A container of a provider is going to be destroyed */
static void _esdashboard_search_view_on_provider_container_destroyed(ClutterActor *inActor, gpointer inUserData)
{
	EsdashboardSearchView				*self;
	EsdashboardSearchViewPrivate		*priv;
	EsdashboardSearchViewProviderData	*providerData;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inActor));
	g_return_if_fail(inUserData);

	providerData=(EsdashboardSearchViewProviderData*)inUserData;

	/* Get search view and private data of view */
	self=providerData->view;
	priv=self->priv;

	/* Move selection to first selectable actor at next available container
	 * if this provider whose container to destroy is the currently selected one.
	 * This avoids reselecting the next available actor in container when
	 * the container's children will get destroyed (one of the actors is the
	 * current selection and from then on reselection will happen.)
	 */
	if(priv->selectionProvider==providerData)
	{
		ClutterActor						*oldSelection;
		ClutterActor						*newSelection;
		EsdashboardSearchViewProviderData	*newSelectionProvider;
		GList								*currentProviderIter;
		GList								*iter;
		EsdashboardSearchViewProviderData	*iterProviderData;

		newSelection=NULL;
		newSelectionProvider=NULL;

		/* Find position of currently selected provider in the list of providers */
		currentProviderIter=NULL;
		for(iter=priv->providers; iter && !currentProviderIter; iter=g_list_next(iter))
		{
			iterProviderData=(EsdashboardSearchViewProviderData*)iter->data;

			/* Check if provider at iterator is the one we want to find */
			if(iterProviderData &&
				iterProviderData->provider==priv->selectionProvider->provider)
			{
				currentProviderIter=iter;
			}
		}

		/* To find next provider with existing container and a selectable actor
		 * after the currently selected one iterate forwards from the found position.
		 * If we find a match set the new selection to first selectable actor
		 * at found provider.
		 */
		for(iter=g_list_next(currentProviderIter); iter && !newSelection; iter=g_list_next(iter))
		{
			iterProviderData=(EsdashboardSearchViewProviderData*)iter->data;

			/* Check if provider at iterator has a container and a selectable actor */
			if(iterProviderData &&
				iterProviderData->container)
			{
				ClutterActor				*selectableActor;

				selectableActor=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(iterProviderData->container),
																					NULL,
																					ESDASHBOARD_SELECTION_TARGET_FIRST,
																					ESDASHBOARD_VIEW(self),
																					FALSE);
				if(selectableActor)
				{
					newSelection=selectableActor;
					newSelectionProvider=iterProviderData;
				}
			}
		}

		/* If we did not find a match when iterating forwards from found position,
		 * then do the same but iterate backwards from the found position.
		 */
		for(iter=g_list_previous(currentProviderIter); iter && !newSelection; iter=g_list_previous(iter))
		{
			iterProviderData=(EsdashboardSearchViewProviderData*)iter->data;

			/* Check if provider at iterator has a container and a selectable actor */
			if(iterProviderData &&
				iterProviderData->container)
			{
				ClutterActor				*selectableActor;

				selectableActor=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(iterProviderData->container),
																					NULL,
																					ESDASHBOARD_SELECTION_TARGET_FIRST,
																					ESDASHBOARD_VIEW(self),
																					FALSE);
				if(selectableActor)
				{
					newSelection=selectableActor;
					newSelectionProvider=iterProviderData;
				}
			}
		}

		/* If we still do not find a match the new selection is NULL because it
		 * was initialized with. So new selection will set to NULL which means
		 * nothing is selected anymore. Otherwise new selection contains the
		 * new selection found and will be set.
		 */
		oldSelection=esdashboard_focusable_get_selection(ESDASHBOARD_FOCUSABLE(self));
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Container of provider %s is destroyed but holds current selection %p of type %s - so selecting %p of type %s of provider %s",
							providerData->provider ? G_OBJECT_TYPE_NAME(providerData->provider) : "<nil>",
							oldSelection, oldSelection ? G_OBJECT_TYPE_NAME(oldSelection) : "<nil>",
							newSelection, newSelection ? G_OBJECT_TYPE_NAME(newSelection) : "<nil>",
							newSelectionProvider && newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<nil>");

		esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), newSelection);
	}

	/* Container will be destroyed so unset pointer to it at provider */
	providerData->container=NULL;
}

/* Updates container of provider with new result set from a last search.
 * Also creates or destroys the container for search provider if needed.
 */
static void _esdashboard_search_view_update_provider_container(EsdashboardSearchView *self,
																EsdashboardSearchViewProviderData *inProviderData,
																EsdashboardSearchResultSet *inNewResultSet)
{
	g_return_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self));
	g_return_if_fail(inProviderData);
	g_return_if_fail(!inNewResultSet || ESDASHBOARD_IS_SEARCH_RESULT_SET(inNewResultSet));

	/* If result set for provider is given then check if we need to create a container
	 * or if we have to update one ...
	 */
	if(inNewResultSet &&
		esdashboard_search_result_set_get_size(inNewResultSet)>0)
	{
		/* Create container for search provider if it does not exist yet */
		if(!inProviderData->container)
		{
			/* Create container for search provider */
			inProviderData->container=esdashboard_search_result_container_new(inProviderData->provider);
			if(!inProviderData->container) return;

			/* Add new container to search view */
			clutter_actor_add_child(CLUTTER_ACTOR(self), inProviderData->container);

			/* Connect signals */
			g_signal_connect(inProviderData->container,
								"icon-clicked",
								G_CALLBACK(_esdashboard_search_view_on_provider_icon_clicked),
								inProviderData);

			g_signal_connect(inProviderData->container,
								"item-clicked",
								G_CALLBACK(_esdashboard_search_view_on_result_item_clicked),
								inProviderData);

			g_signal_connect(inProviderData->container,
								"destroy",
								G_CALLBACK(_esdashboard_search_view_on_provider_container_destroyed),
								inProviderData);
		}

		esdashboard_search_result_container_update(ESDASHBOARD_SEARCH_RESULT_CONTAINER(inProviderData->container), inNewResultSet);
	}
		/* ... but if no result set for provider is given then destroy existing container */
		else
		{
			/* Destroy container */
			if(inProviderData->container)
			{
				/* First disconnect signal handlers from actor before destroying container */
				g_signal_handlers_disconnect_by_data(inProviderData->container, inProviderData);

				/* Destroy container */
				esdashboard_actor_destroy(inProviderData->container);
				inProviderData->container=NULL;
			}
		}

	/* Remember new result set for search provider */
	if(inProviderData->lastResultSet)
	{
		g_object_unref(inProviderData->lastResultSet);
		inProviderData->lastResultSet=NULL;
	}

	if(inNewResultSet) inProviderData->lastResultSet=g_object_ref(inNewResultSet);
}

/* Check if we can perform an incremental search at search provider for requested search terms */
static gboolean _esdashboard_search_view_can_do_incremental_search(EsdashboardSearchViewSearchTerms *inProviderLastTerms,
																	EsdashboardSearchViewSearchTerms *inCurrentSearchTerms)
{
	gchar						**iterProvider;
	gchar						**iterCurrent;

	g_return_val_if_fail(inCurrentSearchTerms, FALSE);

	/* If no last search terms for search provider was provided
	 * then return FALSE to perform full search.
	 */
	if(!inProviderLastTerms) return(FALSE);

	/* Check for incremental search. An incremental search can be done
	 * if the last search terms for a search provider is given, the order
	 * in last search terms of search provider and the current search terms
	 * has not changed and each term in both search terms is a case-sensitive
	 * prefix of the term previously used.
	 */
	iterProvider=inProviderLastTerms->termList;
	iterCurrent=inCurrentSearchTerms->termList;
	while(*iterProvider && *iterCurrent)
	{
		if(g_strcmp0(*iterProvider, *iterCurrent)>0) return(FALSE);

		iterProvider++;
		iterCurrent++;
	}

	/* If we are at end of list of terms in both search term
	 * then both terms list are equal and return TRUE here.
	 */
	if(!(*iterProvider) && !(*iterCurrent)) return(TRUE);

	/* If we get here both terms list the criteria do not match
	 * and an incremental search cannot be done. Return FALSE
	 * to indicate that a full search is needed.
	 */
	return(FALSE);
}

/* Perform search */
static guint _esdashboard_search_view_perform_search(EsdashboardSearchView *self, EsdashboardSearchViewSearchTerms *inSearchTerms)
{
	EsdashboardSearchViewPrivate				*priv;
	GList										*providers;
	GList										*iter;
	guint										numberResults;
	ClutterActor								*reselectOldSelection;
	EsdashboardSearchViewProviderData			*reselectProvider;
	EsdashboardSelectionTarget					reselectDirection;
#ifdef DEBUG
	GTimer										*timer=NULL;
#endif

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self), 0);
	g_return_val_if_fail(inSearchTerms, 0);

	priv=self->priv;
	numberResults=0;

#ifdef DEBUG
	/* Start timer for debug search performance */
	timer=g_timer_new();
#endif

	/* Check if this view has a selection and this one is the first item at
	 * provider's container so we have to reselect the first item at that
	 * result container if selection gets lost while updating results for
	 * this search.
	 */
	reselectProvider=NULL;
	reselectDirection=ESDASHBOARD_SELECTION_TARGET_NEXT;
	reselectOldSelection=esdashboard_focusable_get_selection(ESDASHBOARD_FOCUSABLE(self));
	if(reselectOldSelection)
	{
		EsdashboardSearchViewProviderData	*providerData;

		/* Find data of provider for requested selection and check if current
		 * selection is the first item at provider's result container.
		 */
		providerData=_esdashboard_search_view_get_provider_data_by_actor(self, reselectOldSelection);
		if(providerData)
		{
			ClutterActor					*item;

			/* Get last item of provider's result container */
			item=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																	NULL,
																	ESDASHBOARD_SELECTION_TARGET_LAST,
																	ESDASHBOARD_VIEW(self),
																	FALSE);

			/* Check if it the same as the current selection then remember
			 * the provider to reselect last item if selection changes
			 * while updating search results.
			 */
			if(reselectOldSelection==item)
			{
				reselectProvider=providerData;
				reselectDirection=ESDASHBOARD_SELECTION_TARGET_LAST;
			}

			/* Get first item of provider's result container */
			item=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																	NULL,
																	ESDASHBOARD_SELECTION_TARGET_FIRST,
																	ESDASHBOARD_VIEW(self),
																	FALSE);

			/* Check if it the same as the current selection then remember
			 * the provider to reselect first item if selection changes
			 * while updating search results.
			 */
			if(reselectOldSelection==item)
			{
				reselectProvider=providerData;
				reselectDirection=ESDASHBOARD_SELECTION_TARGET_FIRST;
			}
		}
	}

	/* Perform a search at all registered search providers */
	providers=g_list_copy(priv->providers);
	g_list_foreach(providers, (GFunc)(void*)_esdashboard_search_view_provider_data_ref, NULL);
	for(iter=providers; iter; iter=g_list_next(iter))
	{
		EsdashboardSearchViewProviderData		*providerData;
		gboolean								canDoIncrementalSearch;
		EsdashboardSearchResultSet				*providerNewResultSet;
		EsdashboardSearchResultSet				*providerLastResultSet;

		/* Get data for provider to perform search at */
		providerData=((EsdashboardSearchViewProviderData*)(iter->data));

		/* Check if we can do an incremental search based on previous
		 * results or if we have to do a full search.
		 */
		canDoIncrementalSearch=FALSE;
		providerLastResultSet=NULL;
		if(providerData->lastTerms &&
			_esdashboard_search_view_can_do_incremental_search(providerData->lastTerms, inSearchTerms))
		{
			canDoIncrementalSearch=TRUE;
			if(providerData->lastResultSet) providerLastResultSet=g_object_ref(providerData->lastResultSet);
		}

		/* Perform search */
		providerNewResultSet=esdashboard_search_provider_get_result_set(providerData->provider,
																		(const gchar**)inSearchTerms->termList,
																		providerLastResultSet);
		ESDASHBOARD_DEBUG(self, MISC,
							"Performed %s search at search provider %s and got %u result items",
							canDoIncrementalSearch==TRUE ? "incremental" : "full",
							G_OBJECT_TYPE_NAME(providerData->provider),
							providerNewResultSet ? esdashboard_search_result_set_get_size(providerNewResultSet) : 0);

		/* Count number of results */
		if(providerNewResultSet) numberResults+=esdashboard_search_result_set_get_size(providerNewResultSet);

		/* Remember new search term as last one at search provider */
		if(providerData->lastTerms) _esdashboard_search_view_search_terms_unref(providerData->lastTerms);
		providerData->lastTerms=_esdashboard_search_view_search_terms_ref(inSearchTerms);

		/* Update view of search provider for new result set */
		_esdashboard_search_view_update_provider_container(self, providerData, providerNewResultSet);

		/* Release allocated resources */
		if(providerLastResultSet) g_object_unref(providerLastResultSet);
		if(providerNewResultSet) g_object_unref(providerNewResultSet);
	}
	g_list_free_full(providers, (GDestroyNotify)_esdashboard_search_view_provider_data_unref);

	/* Remember new search terms as last one */
	if(priv->lastTerms) _esdashboard_search_view_search_terms_unref(priv->lastTerms);
	priv->lastTerms=_esdashboard_search_view_search_terms_ref(inSearchTerms);

#ifdef DEBUG
	/* Get time for this search for debug performance */
	ESDASHBOARD_DEBUG(self, MISC,
						"Updating search for '%s' took %f seconds",
						inSearchTerms->termString,
						g_timer_elapsed(timer, NULL));
	g_timer_destroy(timer);
#endif

	/* Reselect first or last item at provider if we remembered the provider where
	 * the item should be reselected and if selection has changed while updating results.
	 */
	if(reselectProvider &&
		reselectProvider->container)
	{
		ClutterActor							*selection;

		/* Get current selection as it may have changed because the selected actor
		 * was destroyed or hidden while updating results.
		 */
		selection=esdashboard_focusable_get_selection(ESDASHBOARD_FOCUSABLE(self));

		/* If selection has changed then re-select first or last item of provider */
		if(selection!=reselectOldSelection)
		{
			/* Get new selection which is the first or last item of provider's
			 * result container.
			 */
			selection=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(reselectProvider->container),
																			NULL,
																			reselectDirection,
																			ESDASHBOARD_VIEW(self),
																			FALSE);

			/* Set new selection */
			esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), selection);
			ESDASHBOARD_DEBUG(self, ACTOR,
								"Reselecting selectable item in direction %d at provider %s as old selection vanished",
								reselectDirection,
								esdashboard_search_provider_get_name(reselectProvider->provider));
		}
	}

	/* If this view has the focus then check if this view has a selection set currently.
	 * If not select the first selectable actor otherwise just ensure the current
	 * selection is visible.
	 */
	if(esdashboard_focus_manager_has_focus(priv->focusManager, ESDASHBOARD_FOCUSABLE(self)))
	{
		ClutterActor							*selection;

		/* Check if this view has a selection set */
		selection=esdashboard_focusable_get_selection(ESDASHBOARD_FOCUSABLE(self));
		if(!selection)
		{
			/* Select first selectable item */
			selection=esdashboard_focusable_find_selection(ESDASHBOARD_FOCUSABLE(self),
															NULL,
															ESDASHBOARD_SELECTION_TARGET_FIRST);
			esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), selection);
		}

		/* Ensure selection is visible. But we have to have for a repaint because
		 * allocation of this view has not changed yet.
		 */
		if(selection &&
			priv->repaintID==0)
		{
			priv->repaintID=clutter_threads_add_repaint_func_full(CLUTTER_REPAINT_FLAGS_QUEUE_REDRAW_ON_ADD | CLUTTER_REPAINT_FLAGS_POST_PAINT,
																	_esdashboard_search_view_on_repaint_after_update_callback,
																	self,
																	NULL);
		}
	}

	/* Emit signal that search was updated */
	g_signal_emit(self, EsdashboardSearchViewSignals[SIGNAL_SEARCH_UPDATED], 0);

	/* Return number of results */
	return(numberResults);
}

/* Delay timeout was reached so perform initial search now */
static gboolean _esdashboard_search_view_on_perform_search_delayed_timeout(gpointer inUserData)
{
	EsdashboardSearchView						*self;
	EsdashboardSearchViewPrivate				*priv;
	guint										numberResults;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(inUserData), G_SOURCE_REMOVE);

	self=ESDASHBOARD_SEARCH_VIEW(inUserData);
	priv=self->priv;

	/* Perform search */
	numberResults=_esdashboard_search_view_perform_search(self, priv->delaySearchTerms);
	if(numberResults==0)
	{
		esdashboard_notify(CLUTTER_ACTOR(self),
							esdashboard_view_get_icon(ESDASHBOARD_VIEW(self)),
							_("No results found for '%s'"),
							priv->delaySearchTerms->termString);
	}

	/* Release allocated resources */
	if(priv->delaySearchTerms)
	{
		_esdashboard_search_view_search_terms_unref(priv->delaySearchTerms);
		priv->delaySearchTerms=NULL;
	}

	/* Do not delay next searches */
	priv->delaySearch=FALSE;

	/* This source will be removed so unset source ID */
	priv->delaySearchTimeoutID=0;

	return(G_SOURCE_REMOVE);
}

/* IMPLEMENTATION: Interface EsdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _esdashboard_search_view_focusable_can_focus(EsdashboardFocusable *inFocusable)
{
	EsdashboardSearchView			*self;
	EsdashboardFocusableInterface	*selfIface;
	EsdashboardFocusableInterface	*parentIface;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(inFocusable), FALSE);

	self=ESDASHBOARD_SEARCH_VIEW(inFocusable);

	/* Call parent class interface function */
	selfIface=ESDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->can_focus)
	{
		if(!parentIface->can_focus(inFocusable)) return(FALSE);
	}

	/* If this view is not enabled it is not focusable */
	if(!esdashboard_view_get_enabled(ESDASHBOARD_VIEW(self))) return(FALSE);

	/* If we get here this actor can be focused */
	return(TRUE);
}

/* Determine if this actor supports selection */
static gboolean _esdashboard_search_view_focusable_supports_selection(EsdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}

/* Get current selection */
static ClutterActor* _esdashboard_search_view_focusable_get_selection(EsdashboardFocusable *inFocusable)
{
	EsdashboardSearchView			*self;
	EsdashboardSearchViewPrivate	*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(inFocusable), NULL);

	self=ESDASHBOARD_SEARCH_VIEW(inFocusable);
	priv=self->priv;

	/* If we have no provider selected (the selection for this view) or
	 * if no container exists then return NULL for no selection.
	 */
	if(!priv->selectionProvider ||
		!priv->selectionProvider->container)
	{
		return(NULL);
	}

	/* Return current selection of selected provider's container */
	return(esdashboard_search_result_container_get_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container)));
}

/* Set new selection */
static gboolean _esdashboard_search_view_focusable_set_selection(EsdashboardFocusable *inFocusable,
																	ClutterActor *inSelection)
{
	EsdashboardSearchView					*self;
	EsdashboardSearchViewPrivate			*priv;
	EsdashboardSearchViewProviderData		*data;
	gboolean								success;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_SEARCH_VIEW(inFocusable);
	priv=self->priv;
	success=FALSE;

	/* If selection to set is NULL, reset internal variables and selection at current selected
	 * container and return TRUE.
	 */
	if(!inSelection)
	{
		/* Reset selection at container of currently selected provider */
		if(priv->selectionProvider &&
			priv->selectionProvider->container)
		{
			esdashboard_search_result_container_set_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container), NULL);
		}

		/* Reset internal variables */
		if(priv->selectionProvider)
		{
			_esdashboard_search_view_provider_data_unref(priv->selectionProvider);
			priv->selectionProvider=NULL;
		}

		/* Return success */
		return(TRUE);
	}

	/* Find data of provider for requested selected actor */
	data=_esdashboard_search_view_get_provider_data_by_actor(self, inSelection);
	if(!data)
	{
		g_warning("%s is not a child of any provider at %s and cannot be selected",
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Set selection at container of provider */
	if(data->container)
	{
		success=esdashboard_search_result_container_set_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(data->container),
																	inSelection);

		/* If we could set selection successfully remember its provider and ensure
		 * that selection is visible.
		 */
		if(success)
		{
			if(priv->selectionProvider)
			{
				_esdashboard_search_view_provider_data_unref(priv->selectionProvider);
				priv->selectionProvider=NULL;
			}

			priv->selectionProvider=_esdashboard_search_view_provider_data_ref(data);

			/* Ensure new selection is visible */
			esdashboard_view_child_ensure_visible(ESDASHBOARD_VIEW(self), inSelection);
		}
	}

	/* Release allocated resources */
	_esdashboard_search_view_provider_data_unref(data);

	/* Return success result */
	return(success);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _esdashboard_search_view_focusable_find_selection_internal_backwards(EsdashboardSearchView *self,
																							EsdashboardSearchResultContainer *inContainer,
																							ClutterActor *inSelection,
																							EsdashboardSelectionTarget inDirection,
																							GList *inCurrentProviderIter,
																							EsdashboardSelectionTarget inNextContainerDirection)
{
	ClutterActor							*newSelection;
	GList									*iter;
	EsdashboardSearchViewProviderData		*providerData;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inContainer), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);
	g_return_val_if_fail(inCurrentProviderIter, NULL);
	g_return_val_if_fail(inNextContainerDirection>=0 && inNextContainerDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	/* Ask current provider to find selection for requested direction */
	newSelection=esdashboard_search_result_container_find_selection(inContainer,
																	inSelection,
																	inDirection,
																	ESDASHBOARD_VIEW(self),
																	FALSE);

	/* If current provider does not return a matching selection for requested,
	 * iterate backwards through providers beginning at current provider and
	 * return the last actor of first provider having an existing container
	 * while iterating.
	 */
	if(!newSelection)
	{
		for(iter=g_list_previous(inCurrentProviderIter); iter && !newSelection; iter=g_list_previous(iter))
		{
			providerData=(EsdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				NULL,
																				inNextContainerDirection,
																				ESDASHBOARD_VIEW(self),
																				FALSE);
			}
		}
	}

	/* If we still have no new selection found, do the same as above but
	 * iterate from end of list of providers backwards to current provider.
	 */
	if(!newSelection)
	{
		for(iter=g_list_last(inCurrentProviderIter); iter && iter!=inCurrentProviderIter && !newSelection; iter=g_list_previous(iter))
		{
			providerData=(EsdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				NULL,
																				inNextContainerDirection,
																				ESDASHBOARD_VIEW(self),
																				FALSE);
			}
		}
	}

	/* If we still have no selection the last resort is to find a selection
	 * at current provider but this time allow wrapping.
	 */
	if(!newSelection)
	{
		newSelection=esdashboard_search_result_container_find_selection(inContainer,
																		inSelection,
																		inDirection,
																		ESDASHBOARD_VIEW(self),
																		TRUE);
	}

	/* Return selection found which may be NULL */
	return(newSelection);
}

static ClutterActor* _esdashboard_search_view_focusable_find_selection_internal_forwards(EsdashboardSearchView *self,
																							EsdashboardSearchResultContainer *inContainer,
																							ClutterActor *inSelection,
																							EsdashboardSelectionTarget inDirection,
																							GList *inCurrentProviderIter,
																							EsdashboardSelectionTarget inNextContainerDirection)
{
	ClutterActor							*newSelection;
	GList									*iter;
	EsdashboardSearchViewProviderData		*providerData;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inContainer), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);
	g_return_val_if_fail(inCurrentProviderIter, NULL);
	g_return_val_if_fail(inNextContainerDirection>=0 && inNextContainerDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	/* Ask current provider to find selection for requested direction */
	newSelection=esdashboard_search_result_container_find_selection(inContainer,
																	inSelection,
																	inDirection,
																	ESDASHBOARD_VIEW(self),
																	FALSE);

	/* If current provider does not return a matching selection for requested,
	 * iterate forwards through providers beginning at current provider and
	 * return the last actor of first provider having an existing container
	 * while iterating.
	 */
	if(!newSelection)
	{
		for(iter=g_list_next(inCurrentProviderIter); iter && !newSelection; iter=g_list_next(iter))
		{
			providerData=(EsdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				NULL,
																				inNextContainerDirection,
																				ESDASHBOARD_VIEW(self),
																				FALSE);
			}
		}
	}

	/* If we still have no new selection found, do the same as above but
	 * iterate from start of list of providers forwards to current provider.
	 */
	if(!newSelection)
	{
		for(iter=g_list_first(inCurrentProviderIter); iter && iter!=inCurrentProviderIter && !newSelection; iter=g_list_next(iter))
		{
			providerData=(EsdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				NULL,
																				inNextContainerDirection,
																				ESDASHBOARD_VIEW(self),
																				FALSE);
			}
		}
	}

	/* If we still have no selection the last resort is to find a selection
	 * at current provider but this time allow wrapping.
	 */
	if(!newSelection)
	{
		newSelection=esdashboard_search_result_container_find_selection(inContainer,
																		inSelection,
																		inDirection,
																		ESDASHBOARD_VIEW(self),
																		TRUE);
	}

	/* Return selection found which may be NULL */
	return(newSelection);
}

static ClutterActor* _esdashboard_search_view_focusable_find_selection(EsdashboardFocusable *inFocusable,
																				ClutterActor *inSelection,
																				EsdashboardSelectionTarget inDirection)
{
	EsdashboardSearchView					*self;
	EsdashboardSearchViewPrivate			*priv;
	ClutterActor							*newSelection;
	EsdashboardSearchViewProviderData		*newSelectionProvider;
	GList									*currentProviderIter;
	GList									*iter;
	EsdashboardSearchViewProviderData		*providerData;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(inFocusable), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	self=ESDASHBOARD_SEARCH_VIEW(inFocusable);
	priv=self->priv;
	newSelection=NULL;
	newSelectionProvider=NULL;

	/* If nothing is selected, select the first selectable actor of the first provider
	 * having an existing container.
	 */
	if(!inSelection)
	{
		/* Find first provider having an existing container and having a selectable
		 * actor in its container.
		 */
		for(iter=priv->providers; iter && !newSelection; iter=g_list_next(iter))
		{
			providerData=(EsdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				NULL,
																				ESDASHBOARD_SELECTION_TARGET_FIRST,
																				ESDASHBOARD_VIEW(self),
																				FALSE);
				if(newSelection) newSelectionProvider=providerData;
			}
		}

		ESDASHBOARD_DEBUG(self, ACTOR,
							"No selection for %s, so select first selectable actor of provider %s",
							G_OBJECT_TYPE_NAME(self),
							newSelectionProvider && newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>");

		return(newSelection);
	}

	/* If first selection is request, select the first selectable actor of the first provider
	 * having an existing container.
	 */
	if(inDirection==ESDASHBOARD_SELECTION_TARGET_FIRST)
	{
		/* Find first provider having an existing container and having a selectable
		 * actor in its container.
		 */
		for(iter=priv->providers; iter && !newSelection; iter=g_list_next(iter))
		{
			providerData=(EsdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				inSelection,
																				ESDASHBOARD_SELECTION_TARGET_FIRST,
																				ESDASHBOARD_VIEW(self),
																				FALSE);
				if(newSelection) newSelectionProvider=providerData;
			}
		}

		ESDASHBOARD_DEBUG(self, ACTOR,
							"First selection requested at %s, so select first selectable actor of provider %s",
							G_OBJECT_TYPE_NAME(self),
							newSelectionProvider && newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>");

		return(newSelection);
	}

	/* If last selection is request, select the last selectable actor of the last provider
	 * having an existing container.
	 */
	if(inDirection==ESDASHBOARD_SELECTION_TARGET_LAST)
	{
		/* Find last provider having an existing container and having a selectable
		 * actor in its container.
		 */
		for(iter=g_list_last(priv->providers); iter && !newSelection; iter=g_list_previous(iter))
		{
			providerData=(EsdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=esdashboard_search_result_container_find_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				inSelection,
																				ESDASHBOARD_SELECTION_TARGET_LAST,
																				ESDASHBOARD_VIEW(self),
																				FALSE);
				if(newSelection) newSelectionProvider=providerData;
			}
		}

		ESDASHBOARD_DEBUG(self, ACTOR,
							"Last selection requested at %s, so select last selectable actor of provider %s",
							G_OBJECT_TYPE_NAME(self),
							newSelectionProvider && newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>");

		return(newSelection);
	}

	/* Find provider data for selection requested. If we do not find provider data
	 * for requested selection then we cannot perform find request.
	 */
	newSelectionProvider=_esdashboard_search_view_get_provider_data_by_actor(self, inSelection);
	if(!newSelectionProvider)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Could not find provider for selection %p of type %s",
							inSelection,
							inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>");
		return(NULL);
	}

	currentProviderIter=g_list_find(priv->providers, newSelectionProvider);
	if(!currentProviderIter)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Could not find position of provider %s",
							newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>");

		/* Release allocated resources */
		_esdashboard_search_view_provider_data_unref(newSelectionProvider);

		return(NULL);
	}

	/* Ask current provider to find selection for requested direction. If a matching
	 * selection could not be found then ask next providers depending on direction.
	 */
	switch(inDirection)
	{
		case ESDASHBOARD_SELECTION_TARGET_LEFT:
		case ESDASHBOARD_SELECTION_TARGET_UP:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_UP:
			newSelection=_esdashboard_search_view_focusable_find_selection_internal_backwards(self,
																								ESDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																								inSelection,
																								inDirection,
																								currentProviderIter,
																								ESDASHBOARD_SELECTION_TARGET_LAST);
			break;

		case ESDASHBOARD_SELECTION_TARGET_RIGHT:
		case ESDASHBOARD_SELECTION_TARGET_DOWN:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			newSelection=_esdashboard_search_view_focusable_find_selection_internal_forwards(self,
																								ESDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																								inSelection,
																								inDirection,
																								currentProviderIter,
																								ESDASHBOARD_SELECTION_TARGET_FIRST);
			break;

		case ESDASHBOARD_SELECTION_TARGET_NEXT:
			newSelection=_esdashboard_search_view_focusable_find_selection_internal_forwards(self,
																								ESDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																								inSelection,
																								inDirection,
																								currentProviderIter,
																								ESDASHBOARD_SELECTION_TARGET_FIRST);
			break;

		case ESDASHBOARD_SELECTION_TARGET_FIRST:
		case ESDASHBOARD_SELECTION_TARGET_LAST:
			/* These directions should be handled at beginning of this function
			 * and therefore should never be reached!
			 */
			g_assert_not_reached();
			break;

		default:
			{
				gchar					*valueName;

				valueName=esdashboard_get_enum_value_name(ESDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical("Focusable object %s and provider %s do not handle selection direction of type %s.",
							G_OBJECT_TYPE_NAME(self),
							newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>",
							valueName);
				g_free(valueName);

				/* Ensure new selection is invalid */
				newSelection=NULL;
			}
			break;
	}

	/* Release allocated resources */
	_esdashboard_search_view_provider_data_unref(newSelectionProvider);

	/* Return new selection found in other search providers */
	return(newSelection);
}

/* Activate selection */
static gboolean _esdashboard_search_view_focusable_activate_selection(EsdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	EsdashboardSearchView					*self;
	EsdashboardSearchViewProviderData		*providerData;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(inFocusable), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_SEARCH_VIEW(inFocusable);

	/* Find data of provider for requested selected actor */
	providerData=_esdashboard_search_view_get_provider_data_by_actor(self, inSelection);
	if(!providerData)
	{
		g_warning("%s is not a child of any provider at %s and cannot be activated",
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Activate selection */
	esdashboard_search_result_container_activate_selection(ESDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
															inSelection);

	/* Release allocated resources */
	_esdashboard_search_view_provider_data_unref(providerData);

	return(TRUE);
}

/* Interface initialization
 * Set up default functions
 */
void _esdashboard_search_view_focusable_iface_init(EsdashboardFocusableInterface *iface)
{
	iface->can_focus=_esdashboard_search_view_focusable_can_focus;
	iface->supports_selection=_esdashboard_search_view_focusable_supports_selection;
	iface->get_selection=_esdashboard_search_view_focusable_get_selection;
	iface->set_selection=_esdashboard_search_view_focusable_set_selection;
	iface->find_selection=_esdashboard_search_view_focusable_find_selection;
	iface->activate_selection=_esdashboard_search_view_focusable_activate_selection;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_search_view_dispose(GObject *inObject)
{
	EsdashboardSearchView			*self=ESDASHBOARD_SEARCH_VIEW(inObject);
	EsdashboardSearchViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->esconfChannel) priv->esconfChannel=NULL;

	if(priv->repaintID)
	{
		g_source_remove(priv->repaintID);
		priv->repaintID=0;
	}

	if(priv->delaySearchTimeoutID)
	{
		g_source_remove(priv->delaySearchTimeoutID);
		priv->delaySearchTimeoutID=0;
	}

	if(priv->delaySearchTerms)
	{
		_esdashboard_search_view_search_terms_unref(priv->delaySearchTerms);
		priv->delaySearchTerms=NULL;
	}

	if(priv->searchManager)
	{
		g_signal_handlers_disconnect_by_data(priv->searchManager, self);
		g_object_unref(priv->searchManager);
		priv->searchManager=NULL;
	}

	if(priv->providers)
	{
		g_list_free_full(priv->providers, (GDestroyNotify)_esdashboard_search_view_provider_data_unref);
		priv->providers=NULL;
	}

	if(priv->lastTerms)
	{
		_esdashboard_search_view_search_terms_unref(priv->lastTerms);
		priv->lastTerms=NULL;
	}

	if(priv->selectionProvider)
	{
		_esdashboard_search_view_provider_data_unref(priv->selectionProvider);
		priv->selectionProvider=NULL;
	}

	if(priv->focusManager)
	{
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_search_view_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void esdashboard_search_view_class_init(EsdashboardSearchViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_search_view_dispose;

	/* Define signals */
	/**
	 * EsdashboardSearchView::search-reset:
	 * @self: The #EsdashboardSearchView whose search was resetted
	 *
	 * The ::search-reset signal is emitted when the current search is cancelled
	 * and resetted.
	 */
	EsdashboardSearchViewSignals[SIGNAL_SEARCH_RESET]=
		g_signal_new("search-reset",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardSearchViewClass, search_reset),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						0);

	/**
	 * EsdashboardSearchView::search-updated:
	 * @self: The #EsdashboardSearchView whose search was updated
	 *
	 * The ::search-updated signal is emitted each time the search term has changed
	 * and all search providers have returned their result which are shown by @self.
	 */
	EsdashboardSearchViewSignals[SIGNAL_SEARCH_UPDATED]=
		g_signal_new("search-updated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardSearchViewClass, search_updated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_search_view_init(EsdashboardSearchView *self)
{
	EsdashboardSearchViewPrivate	*priv;
	GList							*providers, *providerEntry;
	ClutterLayoutManager			*layout;

	self->priv=priv=esdashboard_search_view_get_instance_private(self);

	/* Set up default values */
	priv->searchManager=esdashboard_search_manager_get_default();
	priv->providers=NULL;
	priv->lastTerms=NULL;
	priv->delaySearch=TRUE;
	priv->delaySearchTerms=NULL;
	priv->delaySearchTimeoutID=0;
	priv->selectionProvider=NULL;
	priv->focusManager=esdashboard_focus_manager_get_default();
	priv->repaintID=0;
	priv->esconfChannel=esdashboard_application_get_esconf_channel(NULL);

	/* Set up view (Note: Search view is disabled by default!) */
	esdashboard_view_set_name(ESDASHBOARD_VIEW(self), _("Search"));
	esdashboard_view_set_icon(ESDASHBOARD_VIEW(self), "edit-find");
	esdashboard_view_set_enabled(ESDASHBOARD_VIEW(self), FALSE);

	/* Set up actor */
	esdashboard_actor_set_can_focus(ESDASHBOARD_ACTOR(self), TRUE);

	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), layout);

	esdashboard_view_set_view_fit_mode(ESDASHBOARD_VIEW(self), ESDASHBOARD_VIEW_FIT_MODE_HORIZONTAL);

	/* Create instance of each registered view type and add it to this actor
	 * and connect signals
	 */
	providers=providerEntry=esdashboard_search_manager_get_registered(priv->searchManager);
	for(; providerEntry; providerEntry=g_list_next(providerEntry))
	{
		const gchar				*providerID;

		providerID=(const gchar*)providerEntry->data;
		_esdashboard_search_view_on_search_provider_registered(self, providerID, priv->searchManager);
	}
	g_list_free_full(providers, g_free);

	g_signal_connect_swapped(priv->searchManager,
								"registered",
								G_CALLBACK(_esdashboard_search_view_on_search_provider_registered),
								self);
	g_signal_connect_swapped(priv->searchManager,
								"unregistered",
								G_CALLBACK(_esdashboard_search_view_on_search_provider_unregistered),
								self);
}

/* IMPLEMENTATION: Public API */

/**
 * esdashboard_search_view_reset_search:
 * @self: A #EsdashboardSearchView
 *
 * Cancels and resets the current search at @self. All results will be cleared
 * and usually the view switches back to the one before the search was started.
 */
void esdashboard_search_view_reset_search(EsdashboardSearchView *self)
{
	EsdashboardSearchViewPrivate	*priv;
	GList							*providers;
	GList							*iter;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self));

	priv=self->priv;

	/* Remove timeout source if set */
	if(priv->delaySearchTimeoutID)
	{
		g_source_remove(priv->delaySearchTimeoutID);
		priv->delaySearchTimeoutID=0;
	}

	/* Reset all search providers by destroying actors, destroying containers,
	 * clearing mappings and release all other allocated resources used.
	 */
	providers=g_list_copy(priv->providers);
	g_list_foreach(providers, (GFunc)(void*)_esdashboard_search_view_provider_data_ref, NULL);
	for(iter=providers; iter; iter=g_list_next(iter))
	{
		EsdashboardSearchViewProviderData		*providerData;

		/* Get data for provider to reset */
		providerData=((EsdashboardSearchViewProviderData*)(iter->data));

		/* Destroy container */
		if(providerData->container)
		{
			/* First disconnect signal handlers from actor before destroying container */
			g_signal_handlers_disconnect_by_data(providerData->container, providerData);

			/* Destroy container */
			esdashboard_actor_destroy(providerData->container);
			providerData->container=NULL;
		}

		/* Release last result set as provider has no results anymore */
		if(providerData->lastResultSet)
		{
			g_object_unref(providerData->lastResultSet);
			providerData->lastResultSet=NULL;
		}

		/* Release last terms used in last search of provider */
		if(providerData->lastTerms)
		{
			_esdashboard_search_view_search_terms_unref(providerData->lastTerms);
			providerData->lastTerms=NULL;
		}
	}
	g_list_free_full(providers, (GDestroyNotify)_esdashboard_search_view_provider_data_unref);

	/* Reset last search terms used in this view */
	if(priv->lastTerms)
	{
		_esdashboard_search_view_search_terms_unref(priv->lastTerms);
		priv->lastTerms=NULL;
	}

	/* Set flag to delay next search again */
	priv->delaySearch=TRUE;

	/* Emit signal that search was resetted */
	g_signal_emit(self, EsdashboardSearchViewSignals[SIGNAL_SEARCH_RESET], 0);
}

/**
 * esdashboard_search_view_update_search:
 * @self: A #EsdashboardSearchView
 * @inSearchString: The search term to use for new search or to update current one
 *
 * Starts a new search or update the current search at @self with the updated
 * search terms in @inSearchString. All search providers will be asked to provide
 * a initial result set for @inSearchString if a new search is started or to
 * return an updated result set for the new search term in @inSearchString which
 * are then shown by @self.
 */
void esdashboard_search_view_update_search(EsdashboardSearchView *self, const gchar *inSearchString)
{
	EsdashboardSearchViewPrivate				*priv;
	EsdashboardSearchViewSearchTerms			*searchTerms;
	guint										delaySearchTimeout;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_VIEW(self));

	priv=self->priv;

	/* Only perform a search if new search term differs from old one */
	if(priv->lastTerms &&
		g_strcmp0(inSearchString, priv->lastTerms->termString)==0)
	{
		return;
	}

	/* Searching for NULL or an empty string is like resetting search */
	if(!inSearchString || strlen(inSearchString)==0)
	{
		esdashboard_search_view_reset_search(self);
		return;
	}

	/* Get search terms for search string. If we could not split search
	 * string into terms then reset current search.
	 */
	searchTerms=_esdashboard_search_view_search_terms_new(inSearchString);
	if(!searchTerms)
	{
		/* Splitting search string into terms failed so reset search */
		esdashboard_search_view_reset_search(self);
		return;
	}

	/* Check if search should be delayed ... */
	delaySearchTimeout=esconf_channel_get_uint(priv->esconfChannel,
												DELAY_SEARCH_TIMEOUT_ESCONF_PROP,
												DEFAULT_DELAY_SEARCH_TIMEOUT);
	if(delaySearchTimeout>0 && priv->delaySearch)
	{
		/* Remember search terms for delayed search */
		if(priv->delaySearchTerms)
		{
			_esdashboard_search_view_search_terms_unref(priv->delaySearchTerms);
			priv->delaySearchTerms=NULL;
		}
		priv->delaySearchTerms=_esdashboard_search_view_search_terms_ref(searchTerms);

		/* Create timeout source to delay search if no one exists */
		if(!priv->delaySearchTimeoutID)
		{
			priv->delaySearchTimeoutID=g_timeout_add(delaySearchTimeout,
														_esdashboard_search_view_on_perform_search_delayed_timeout,
														self);
		}
	}
		/* ... otherwise perform search immediately */
		else
		{
			_esdashboard_search_view_perform_search(self, searchTerms);
		}

	/* Release allocated resources */
	if(searchTerms) _esdashboard_search_view_search_terms_unref(searchTerms);
}
