/*
 * search-result-container: An container for results from a search provider
 *                          which has a header and container for items
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

#ifndef __LIBESDASHBOARD_SEARCH_RESULT_CONTAINER__
#define __LIBESDASHBOARD_SEARCH_RESULT_CONTAINER__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/actor.h>
#include <libesdashboard/search-provider.h>
#include <libesdashboard/types.h>
#include <libesdashboard/view.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER				(esdashboard_search_result_container_get_type())
#define ESDASHBOARD_SEARCH_RESULT_CONTAINER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER, EsdashboardSearchResultContainer))
#define ESDASHBOARD_IS_SEARCH_RESULT_CONTAINER(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER))
#define ESDASHBOARD_SEARCH_RESULT_CONTAINER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER, EsdashboardSearchResultContainerClass))
#define ESDASHBOARD_IS_SEARCH_RESULT_CONTAINER_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER))
#define ESDASHBOARD_SEARCH_RESULT_CONTAINER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER, EsdashboardSearchResultContainerClass))

typedef struct _EsdashboardSearchResultContainer				EsdashboardSearchResultContainer;
typedef struct _EsdashboardSearchResultContainerClass			EsdashboardSearchResultContainerClass;
typedef struct _EsdashboardSearchResultContainerPrivate			EsdashboardSearchResultContainerPrivate;

struct _EsdashboardSearchResultContainer
{
	/*< private >*/
	/* Parent instance */
	EsdashboardActor							parent_instance;

	/* Private structure */
	EsdashboardSearchResultContainerPrivate		*priv;
};

struct _EsdashboardSearchResultContainerClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardActorClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*icon_clicked)(EsdashboardSearchResultContainer *self);
	void (*item_clicked)(EsdashboardSearchResultContainer *self, GVariant *inItem, ClutterActor *inActor);
};

/* Public API */
GType esdashboard_search_result_container_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_search_result_container_new(EsdashboardSearchProvider *inProvider);

const gchar* esdashboard_search_result_container_get_icon(EsdashboardSearchResultContainer *self);
void esdashboard_search_result_container_set_icon(EsdashboardSearchResultContainer *self, const gchar *inIcon);

const gchar* esdashboard_search_result_container_get_title_format(EsdashboardSearchResultContainer *self);
void esdashboard_search_result_container_set_title_format(EsdashboardSearchResultContainer *self, const gchar *inFormat);

EsdashboardViewMode esdashboard_search_result_container_get_view_mode(EsdashboardSearchResultContainer *self);
void esdashboard_search_result_container_set_view_mode(EsdashboardSearchResultContainer *self, const EsdashboardViewMode inMode);

gfloat esdashboard_search_result_container_get_spacing(EsdashboardSearchResultContainer *self);
void esdashboard_search_result_container_set_spacing(EsdashboardSearchResultContainer *self, const gfloat inSpacing);

gfloat esdashboard_search_result_container_get_padding(EsdashboardSearchResultContainer *self);
void esdashboard_search_result_container_set_padding(EsdashboardSearchResultContainer *self, const gfloat inPadding);

gint esdashboard_search_result_container_get_initial_result_size(EsdashboardSearchResultContainer *self);
void esdashboard_search_result_container_set_initial_result_size(EsdashboardSearchResultContainer *self, const gint inSize);

gint esdashboard_search_result_container_get_more_result_size(EsdashboardSearchResultContainer *self);
void esdashboard_search_result_container_set_more_result_size(EsdashboardSearchResultContainer *self, const gint inSize);

void esdashboard_search_result_container_set_focus(EsdashboardSearchResultContainer *self, gboolean inSetFocus);

ClutterActor* esdashboard_search_result_container_get_selection(EsdashboardSearchResultContainer *self);
gboolean esdashboard_search_result_container_set_selection(EsdashboardSearchResultContainer *self,
																	ClutterActor *inSelection);
ClutterActor* esdashboard_search_result_container_find_selection(EsdashboardSearchResultContainer *self,
																	ClutterActor *inSelection,
																	EsdashboardSelectionTarget inDirection,
																	EsdashboardView *inView,
																	gboolean inAllowWrap);
void esdashboard_search_result_container_activate_selection(EsdashboardSearchResultContainer *self,
																	ClutterActor *inSelection);

void esdashboard_search_result_container_update(EsdashboardSearchResultContainer *self, EsdashboardSearchResultSet *inResults);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_SEARCH_RESULT_CONTAINER__ */
