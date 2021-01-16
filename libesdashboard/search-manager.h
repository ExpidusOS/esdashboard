/*
 * search-manager: Single-instance managing search providers and
 *                 handles search requests
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

#ifndef __LIBESDASHBOARD_SEARCH_MANAGER__
#define __LIBESDASHBOARD_SEARCH_MANAGER__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libesdashboard/search-provider.h>
#include <libesdashboard/search-result-set.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SEARCH_MANAGER				(esdashboard_search_manager_get_type())
#define ESDASHBOARD_SEARCH_MANAGER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SEARCH_MANAGER, EsdashboardSearchManager))
#define ESDASHBOARD_IS_SEARCH_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SEARCH_MANAGER))
#define ESDASHBOARD_SEARCH_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SEARCH_MANAGER, EsdashboardSearchManagerClass))
#define ESDASHBOARD_IS_SEARCH_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SEARCH_MANAGER))
#define ESDASHBOARD_SEARCH_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SEARCH_MANAGER, EsdashboardSearchManagerClass))

typedef struct _EsdashboardSearchManager			EsdashboardSearchManager;
typedef struct _EsdashboardSearchManagerClass		EsdashboardSearchManagerClass;
typedef struct _EsdashboardSearchManagerPrivate		EsdashboardSearchManagerPrivate;

struct _EsdashboardSearchManager
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardSearchManagerPrivate		*priv;
};

struct _EsdashboardSearchManagerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*registered)(EsdashboardSearchManager *self, const gchar *inID);
	void (*unregistered)(EsdashboardSearchManager *self, const gchar *inID);
};

/* Public API */
GType esdashboard_search_manager_get_type(void) G_GNUC_CONST;

EsdashboardSearchManager* esdashboard_search_manager_get_default(void);

gboolean esdashboard_search_manager_register(EsdashboardSearchManager *self, const gchar *inID, GType inProviderType);
gboolean esdashboard_search_manager_unregister(EsdashboardSearchManager *self, const gchar *inID);
GList* esdashboard_search_manager_get_registered(EsdashboardSearchManager *self);
gboolean esdashboard_search_manager_has_registered_id(EsdashboardSearchManager *self, const gchar *inID);

GObject* esdashboard_search_manager_create_provider(EsdashboardSearchManager *self, const gchar *inID);

gchar** esdashboard_search_manager_get_search_terms_from_string(const gchar *inString, const gchar *inDelimiters);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_SEARCH_MANAGER__ */
