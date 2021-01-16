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

#ifndef __LIBESDASHBOARD_SEARCH_VIEW__
#define __LIBESDASHBOARD_SEARCH_VIEW__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/view.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SEARCH_VIEW			(esdashboard_search_view_get_type())
#define ESDASHBOARD_SEARCH_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SEARCH_VIEW, EsdashboardSearchView))
#define ESDASHBOARD_IS_SEARCH_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SEARCH_VIEW))
#define ESDASHBOARD_SEARCH_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SEARCH_VIEW, EsdashboardSearchViewClass))
#define ESDASHBOARD_IS_SEARCH_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SEARCH_VIEW))
#define ESDASHBOARD_SEARCH_VIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SEARCH_VIEW, EsdashboardSearchViewClass))

typedef struct _EsdashboardSearchView			EsdashboardSearchView; 
typedef struct _EsdashboardSearchViewPrivate	EsdashboardSearchViewPrivate;
typedef struct _EsdashboardSearchViewClass		EsdashboardSearchViewClass;

/**
 * EsdashboardSearchView:
 *
 * The #EsdashboardSearchView structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardSearchView
{
	/*< private >*/
	/* Parent instance */
	EsdashboardView					parent_instance;

	/* Private structure */
	EsdashboardSearchViewPrivate	*priv;
};

/**
 * EsdashboardSearchViewClass:
 * @search_reset: Class handler for the #EsdashboardSearchViewClass::search-reset signal
 * @search_updated: Class handler for the #EsdashboardSearchViewClass::search-updated signal
 *
 * The #EsdashboardSearchViewClass structure contains only private data
 */
struct _EsdashboardSearchViewClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardViewClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*search_reset)(EsdashboardSearchView *self);
	void (*search_updated)(EsdashboardSearchView *self);
};

/* Public API */
GType esdashboard_search_view_get_type(void) G_GNUC_CONST;

void esdashboard_search_view_reset_search(EsdashboardSearchView *self);
void esdashboard_search_view_update_search(EsdashboardSearchView *self, const gchar *inSearchString);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_SEARCH_VIEW__ */
