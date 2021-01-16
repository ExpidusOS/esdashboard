/*
 * plugins-manager: Single-instance managing plugins
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

#ifndef __LIBESDASHBOARD_PLUGINS_MANAGER__
#define __LIBESDASHBOARD_PLUGINS_MANAGER__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_PLUGINS_MANAGER			(esdashboard_plugins_manager_get_type())
#define ESDASHBOARD_PLUGINS_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_PLUGINS_MANAGER, EsdashboardPluginsManager))
#define ESDASHBOARD_IS_PLUGINS_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_PLUGINS_MANAGER))
#define ESDASHBOARD_PLUGINS_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_PLUGINS_MANAGER, EsdashboardPluginsManagerClass))
#define ESDASHBOARD_IS_PLUGINS_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_PLUGINS_MANAGER))
#define ESDASHBOARD_PLUGINS_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_PLUGINS_MANAGER, EsdashboardPluginsManagerClass))

typedef struct _EsdashboardPluginsManager			EsdashboardPluginsManager;
typedef struct _EsdashboardPluginsManagerClass		EsdashboardPluginsManagerClass;
typedef struct _EsdashboardPluginsManagerPrivate	EsdashboardPluginsManagerPrivate;

/**
 * EsdashboardPluginsManager:
 *
 * The #EsdashboardPluginsManager structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardPluginsManager
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardPluginsManagerPrivate	*priv;
};

/**
 * EsdashboardPluginsManagerClass:
 *
 * The #EsdashboardPluginsManagerClass structure contains only private data
 */
struct _EsdashboardPluginsManagerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;
};

/* Public API */
GType esdashboard_plugins_manager_get_type(void) G_GNUC_CONST;

EsdashboardPluginsManager* esdashboard_plugins_manager_get_default(void);

gboolean esdashboard_plugins_manager_setup(EsdashboardPluginsManager *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_PLUGINS_MANAGER__ */
