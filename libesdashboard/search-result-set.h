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

#ifndef __LIBESDASHBOARD_SEARCH_RESULT_SET__
#define __LIBESDASHBOARD_SEARCH_RESULT_SET__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SEARCH_RESULT_SET				(esdashboard_search_result_set_get_type())
#define ESDASHBOARD_SEARCH_RESULT_SET(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SEARCH_RESULT_SET, EsdashboardSearchResultSet))
#define ESDASHBOARD_IS_SEARCH_RESULT_SET(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SEARCH_RESULT_SET))
#define ESDASHBOARD_SEARCH_RESULT_SET_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SEARCH_RESULT_SET, EsdashboardSearchResultSetClass))
#define ESDASHBOARD_IS_SEARCH_RESULT_SET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SEARCH_RESULT_SET))
#define ESDASHBOARD_SEARCH_RESULT_SET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SEARCH_RESULT_SET, EsdashboardSearchResultSetClass))

typedef struct _EsdashboardSearchResultSet				EsdashboardSearchResultSet;
typedef struct _EsdashboardSearchResultSetClass			EsdashboardSearchResultSetClass;
typedef struct _EsdashboardSearchResultSetPrivate		EsdashboardSearchResultSetPrivate;

struct _EsdashboardSearchResultSet
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardSearchResultSetPrivate	*priv;
};

struct _EsdashboardSearchResultSetClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_search_result_set_get_type(void) G_GNUC_CONST;

EsdashboardSearchResultSet* esdashboard_search_result_set_new(void);

guint esdashboard_search_result_set_get_size(EsdashboardSearchResultSet *self);

void esdashboard_search_result_set_add_item(EsdashboardSearchResultSet *self, GVariant *inItem);
gboolean esdashboard_search_result_set_has_item(EsdashboardSearchResultSet *self, GVariant *inItem);
GList* esdashboard_search_result_set_get_all(EsdashboardSearchResultSet *self);

GList* esdashboard_search_result_set_intersect(EsdashboardSearchResultSet *self, EsdashboardSearchResultSet *inOtherSet);
GList* esdashboard_search_result_set_complement(EsdashboardSearchResultSet *self, EsdashboardSearchResultSet *inOtherSet);

typedef gint (*EsdashboardSearchResultSetCompareFunc)(GVariant *inLeft, GVariant *inRight, gpointer inUserData);
void esdashboard_search_result_set_set_sort_func(EsdashboardSearchResultSet *self,
													EsdashboardSearchResultSetCompareFunc inCallbackFunc,
													gpointer inUserData);
void esdashboard_search_result_set_set_sort_func_full(EsdashboardSearchResultSet *self,
														EsdashboardSearchResultSetCompareFunc inCallbackFunc,
														gpointer inUserData,
														GDestroyNotify inUserDataDestroyFunc);

/* Result set item related functions */
gfloat esdashboard_search_result_set_get_item_score(EsdashboardSearchResultSet *self, GVariant *inItem);
gboolean esdashboard_search_result_set_set_item_score(EsdashboardSearchResultSet *self, GVariant *inItem, gfloat inScore);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_SEARCH_RESULT_SET__ */
