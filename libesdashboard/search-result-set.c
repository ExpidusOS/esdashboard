/*
 * search-result-set: Contains and manages set of identifiers of a search
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

#include <libesdashboard/search-result-set.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/compat.h>


/* Define this class in GObject system */
struct _EsdashboardSearchResultSetPrivate
{
	/* Instance related */
	GHashTable								*set;

	EsdashboardSearchResultSetCompareFunc	sortCallback;
	gpointer								sortUserData;
	GDestroyNotify							sortUserDataDestroyFunc;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardSearchResultSet,
							esdashboard_search_result_set,
							G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
typedef struct _EsdashboardSearchResultSetItemData		EsdashboardSearchResultSetItemData;
struct _EsdashboardSearchResultSetItemData
{
	gint									refCount;

	/* Item related */
	gfloat									score;
};

/* Create, destroy, ref and unref item data for an item */
static EsdashboardSearchResultSetItemData* _esdashboard_search_result_set_item_data_new(void)
{
	EsdashboardSearchResultSetItemData	*data;

	/* Create statistics data */
	data=g_new0(EsdashboardSearchResultSetItemData, 1);
	if(!data) return(NULL);

	/* Set up statistics data */
	data->refCount=1;

	return(data);
}

static void _esdashboard_search_result_set_item_data_free(EsdashboardSearchResultSetItemData *inData)
{
	g_return_if_fail(inData);

	/* Release common allocated resources */
	g_free(inData);
}

static EsdashboardSearchResultSetItemData* _esdashboard_search_result_set_item_data_ref(EsdashboardSearchResultSetItemData *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _esdashboard_search_result_set_item_data_unref(EsdashboardSearchResultSetItemData *inData)
{
	g_return_if_fail(inData);

	inData->refCount--;
	if(inData->refCount==0) _esdashboard_search_result_set_item_data_free(inData);
}

static EsdashboardSearchResultSetItemData* _esdashboard_search_result_set_item_data_get(EsdashboardSearchResultSet *self, GVariant *inItem)
{
	EsdashboardSearchResultSetPrivate		*priv;
	EsdashboardSearchResultSetItemData		*itemData;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self), NULL);
	g_return_val_if_fail(inItem, NULL);

	priv=self->priv;
	itemData=NULL;

	/* Lookup item in result and take reference on item data if found
	 * before returning it,
	 */
	if(g_hash_table_lookup_extended(priv->set, inItem, NULL, (gpointer*)&itemData))
	{
		_esdashboard_search_result_set_item_data_ref(itemData);
	}

	/* Return item data found for item in result set if any */
	return(itemData);
}

/* Internal callback function for calling callback functions for sorting */
static gint _esdashboard_search_result_set_sort_internal(gconstpointer inLeft,
															gconstpointer inRight,
															gpointer inUserData)
{
	EsdashboardSearchResultSet				*self=ESDASHBOARD_SEARCH_RESULT_SET(inUserData);
	EsdashboardSearchResultSetPrivate		*priv=self->priv;
	GVariant								*left;
	EsdashboardSearchResultSetItemData		*leftData;
	GVariant								*right;
	EsdashboardSearchResultSetItemData		*rightData;
	gint									result;

	result=0;

	/* Get items to compare */
	left=(GVariant*)inLeft;
	right=(GVariant*)inRight;

	/* Get score for each item to compare for sorting if for both items available */
	leftData=_esdashboard_search_result_set_item_data_get(self, left);
	rightData=_esdashboard_search_result_set_item_data_get(self, right);
	if(leftData && rightData)
	{
		/* Set result to corresponding value and other than null if the
		 * scores are not equal.
		 */
		if(leftData->score < rightData->score) result=1;
		if(leftData->score > rightData->score) result=-1;
	}
	if(leftData) _esdashboard_search_result_set_item_data_unref(leftData);
	if(rightData) _esdashboard_search_result_set_item_data_unref(rightData);

	/* If both items do not have the same score the result is set to value
	 * other than zero. So return it now.
	 */
	if(result!=0) return(result);

	/* Call sorting callback function now if both have the same score */
	return((priv->sortCallback)(left, right, priv->sortUserData));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_search_result_set_dispose(GObject *inObject)
{
	EsdashboardSearchResultSet			*self=ESDASHBOARD_SEARCH_RESULT_SET(inObject);
	EsdashboardSearchResultSetPrivate	*priv=self->priv;

	/* Release allocated resouces */
	if(priv->sortUserData)
	{
		/* Release old sort user data with destroy function if set previously
		 * and reset destroy function.
		 */
		if(priv->sortUserDataDestroyFunc)
		{
			priv->sortUserDataDestroyFunc(priv->sortUserData);
			priv->sortUserDataDestroyFunc=NULL;
		}

		/* Reset sort user data */
		priv->sortUserData=NULL;
	}

	priv->sortCallback=NULL;

	if(priv->set)
	{
		g_hash_table_unref(priv->set);
		priv->set=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_search_result_set_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void esdashboard_search_result_set_class_init(EsdashboardSearchResultSetClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_search_result_set_dispose;
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_search_result_set_init(EsdashboardSearchResultSet *self)
{
	EsdashboardSearchResultSetPrivate	*priv;

	priv=self->priv=esdashboard_search_result_set_get_instance_private(self);

	/* Set default values */
	priv->set=g_hash_table_new_full(g_variant_hash,
									g_variant_equal,
									(GDestroyNotify)g_variant_unref,
									(GDestroyNotify)_esdashboard_search_result_set_item_data_unref);
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
EsdashboardSearchResultSet* esdashboard_search_result_set_new(void)
{
	return((EsdashboardSearchResultSet*)g_object_new(ESDASHBOARD_TYPE_SEARCH_RESULT_SET, NULL));
}

/* Get size of result set */
guint esdashboard_search_result_set_get_size(EsdashboardSearchResultSet *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self), 0);

	return(g_hash_table_size(self->priv->set));
}

/* Add a result item to result set */
void esdashboard_search_result_set_add_item(EsdashboardSearchResultSet *self, GVariant *inItem)
{
	EsdashboardSearchResultSetPrivate		*priv;
	EsdashboardSearchResultSetItemData		*itemData;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self));
	g_return_if_fail(inItem);

	priv=self->priv;

	/* Add or replace item in hash table */
	if(!g_hash_table_lookup_extended(priv->set, inItem, NULL, (gpointer*)&itemData))
	{
		/* Create data for item to add */
		itemData=_esdashboard_search_result_set_item_data_new();

		/* Add new item to result set */
		g_hash_table_insert(priv->set, g_variant_ref_sink(inItem), itemData);
	}
}

/* Check if a result item exists already in result set */
gboolean esdashboard_search_result_set_has_item(EsdashboardSearchResultSet *self, GVariant *inItem)
{
	EsdashboardSearchResultSetPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self), FALSE);
	g_return_val_if_fail(inItem, FALSE);

	priv=self->priv;

	/* Return result indicating existence of item in this result set */
	return(g_hash_table_lookup_extended(priv->set, inItem, NULL, NULL));
}

/* Get list of all items in this result sets.
 * Returned list should be freed with g_list_free_full(result, g_variant_unref)
 */
GList* esdashboard_search_result_set_get_all(EsdashboardSearchResultSet *self)
{
	EsdashboardSearchResultSetPrivate		*priv;
	GHashTableIter							iter;
	GList									*list;
	GVariant								*key;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self), NULL);

	priv=self->priv;

	/* Iterate through hash table of this result set, take a reference of
	 * key of this result set's hash table item and add list to result list.
	 */
	list=NULL;
	g_hash_table_iter_init(&iter, priv->set);
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, NULL))
	{
		list=g_list_prepend(list, g_variant_ref(key));
	}

	/* If a sorting function was set then sort result list */
	if(list && priv->sortCallback)
	{
		list=g_list_sort_with_data(list, _esdashboard_search_result_set_sort_internal, self);
	}

	/* Return result */
	return(list);
}

/* Get list of all items existing in both result sets.
 * Returned list should be freed with g_list_free_full(result, g_variant_unref)
 */
GList* esdashboard_search_result_set_intersect(EsdashboardSearchResultSet *self, EsdashboardSearchResultSet *inOtherSet)
{
	EsdashboardSearchResultSetPrivate		*priv;
	GHashTableIter							iter;
	GList									*list;
	GVariant								*key;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(inOtherSet), NULL);

	priv=self->priv;

	/* Iterate through hash table of this result set and lookup its keys
	 * at other result set's hash table. If it exists take a reference of
	 * key of this result set's hash table item and add list to result list.
	 */
	list=NULL;
	g_hash_table_iter_init(&iter, priv->set);
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, NULL))
	{
		/* Lookup key in other result set's hash table */
		if(g_hash_table_lookup_extended(inOtherSet->priv->set, key, NULL, NULL))
		{
			list=g_list_prepend(list, g_variant_ref(key));
		}
	}

	/* If a sorting function was set then sort result list */
	if(list && priv->sortCallback)
	{
		list=g_list_sort_with_data(list, _esdashboard_search_result_set_sort_internal, self);
	}

	/* Return result */
	return(list);
}

/* Get list of all items existing in other result set but not in this result set.
 * Returned list should be freed with g_list_free_full(result, g_variant_unref)
 */
GList* esdashboard_search_result_set_complement(EsdashboardSearchResultSet *self, EsdashboardSearchResultSet *inOtherSet)
{
	EsdashboardSearchResultSetPrivate		*priv;
	GHashTableIter							iter;
	GList									*list;
	GVariant								*key;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(inOtherSet), NULL);

	priv=self->priv;

	/* Iterate through hash table of other result set and lookup its keys
	 * at this result set's hash table. If it does not exists take a reference
	 * of key of this result set's hash table item and add list to result list.
	 */
	list=NULL;
	g_hash_table_iter_init(&iter, inOtherSet->priv->set);
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, NULL))
	{
		/* Lookup key in other result set's hash table and in case it does not
		 * exist then add a reference of the key to result.
		 */
		if(!g_hash_table_lookup_extended(priv->set, key, NULL, NULL))
		{
			list=g_list_prepend(list, g_variant_ref(key));
		}
	}

	/* If a sorting function was set then sort result list */
	if(list && priv->sortCallback)
	{
		list=g_list_sort_with_data(list, _esdashboard_search_result_set_sort_internal, self);
	}

	/* Return result */
	return(list);
}

/* Sets a callback function for sorting all items in result set */
void esdashboard_search_result_set_set_sort_func(EsdashboardSearchResultSet *self,
													EsdashboardSearchResultSetCompareFunc inCallbackFunc,
													gpointer inUserData)
{
	esdashboard_search_result_set_set_sort_func_full(self, inCallbackFunc, inUserData, NULL);
}

void esdashboard_search_result_set_set_sort_func_full(EsdashboardSearchResultSet *self,
														EsdashboardSearchResultSetCompareFunc inCallbackFunc,
														gpointer inUserData,
														GDestroyNotify inUserDataDestroyFunc)
{
	EsdashboardSearchResultSetPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self));

	priv=self->priv;

	/* Release old sort callback data */
	if(priv->sortUserData)
	{
		/* Release old sort user data with destroy function if set previously
		 * and reset destroy function.
		 */
		if(priv->sortUserDataDestroyFunc)
		{
			priv->sortUserDataDestroyFunc(priv->sortUserData);
			priv->sortUserDataDestroyFunc=NULL;
		}

		/* Reset sort user data */
		priv->sortUserData=NULL;
	}
	priv->sortCallback=NULL;

	/* Set sort callback */
	if(inCallbackFunc)
	{
		priv->sortCallback=inCallbackFunc;
		priv->sortUserData=inUserData;
		priv->sortUserDataDestroyFunc=inUserDataDestroyFunc;
	}
}

/* Get/set score for a result item in result set */
gfloat esdashboard_search_result_set_get_item_score(EsdashboardSearchResultSet *self, GVariant *inItem)
{
	gfloat									score;
	EsdashboardSearchResultSetItemData		*itemData;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self), 0.0f);
	g_return_val_if_fail(inItem, 0.0f);

	score=0.0f;

	/* Check if requested item exists and get its score from item data */
	itemData=_esdashboard_search_result_set_item_data_get(self, inItem);
	if(itemData)
	{
		score=itemData->score;

		/* Release allocated resources */
		_esdashboard_search_result_set_item_data_unref(itemData);
	}

	/* Return score of item */
	return(score);
}

gboolean esdashboard_search_result_set_set_item_score(EsdashboardSearchResultSet *self, GVariant *inItem, gfloat inScore)
{
	EsdashboardSearchResultSetItemData		*itemData;
	gboolean								success;

	g_return_val_if_fail(ESDASHBOARD_IS_SEARCH_RESULT_SET(self), FALSE);
	g_return_val_if_fail(inItem, FALSE);
	g_return_val_if_fail(inScore>=0.0f && inScore<=1.0f, FALSE);

	success=FALSE;

	/* Check if requested item exists and set score at its item data */
	itemData=_esdashboard_search_result_set_item_data_get(self, inItem);
	if(itemData)
	{
		/* Set score */
		itemData->score=inScore;

		/* Release allocated resources */
		_esdashboard_search_result_set_item_data_unref(itemData);

		/* Set flag that item exists in result set and data could be set */
		success=TRUE;
	}

	/* Return success state for item */
	return(success);
}
