/*
 * view-manager: Single-instance managing views
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

#ifndef __LIBESDASHBOARD_VIEW_MANAGER__
#define __LIBESDASHBOARD_VIEW_MANAGER__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_VIEW_MANAGER				(esdashboard_view_manager_get_type())
#define ESDASHBOARD_VIEW_MANAGER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_VIEW_MANAGER, EsdashboardViewManager))
#define ESDASHBOARD_IS_VIEW_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_VIEW_MANAGER))
#define ESDASHBOARD_VIEW_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_VIEW_MANAGER, EsdashboardViewManagerClass))
#define ESDASHBOARD_IS_VIEW_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_VIEW_MANAGER))
#define ESDASHBOARD_VIEW_MANAGER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_VIEW_MANAGER, EsdashboardViewManagerClass))

typedef struct _EsdashboardViewManager				EsdashboardViewManager;
typedef struct _EsdashboardViewManagerClass			EsdashboardViewManagerClass;
typedef struct _EsdashboardViewManagerPrivate		EsdashboardViewManagerPrivate;

struct _EsdashboardViewManager
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	EsdashboardViewManagerPrivate	*priv;
};

struct _EsdashboardViewManagerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*registered)(EsdashboardViewManager *self, const gchar *inID);
	void (*unregistered)(EsdashboardViewManager *self, const gchar *inID);
};

/* Public API */
GType esdashboard_view_manager_get_type(void) G_GNUC_CONST;

EsdashboardViewManager* esdashboard_view_manager_get_default(void);

gboolean esdashboard_view_manager_register(EsdashboardViewManager *self, const gchar *inID, GType inViewType);
gboolean esdashboard_view_manager_unregister(EsdashboardViewManager *self, const gchar *inID);
GList* esdashboard_view_manager_get_registered(EsdashboardViewManager *self);
gboolean esdashboard_view_manager_has_registered_id(EsdashboardViewManager *self, const gchar *inID);

GObject* esdashboard_view_manager_create_view(EsdashboardViewManager *self, const gchar *inID);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_VIEW_MANAGER__ */
