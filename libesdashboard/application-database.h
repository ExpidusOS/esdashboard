/*
 * application-database: A singelton managing desktop files and menus
 *                       for installed applications
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

#ifndef __LIBESDASHBOARD_APPLICATION_DATABASE__
#define __LIBESDASHBOARD_APPLICATION_DATABASE__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <garcon/garcon.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_APPLICATION_DATABASE				(esdashboard_application_database_get_type())
#define ESDASHBOARD_APPLICATION_DATABASE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_APPLICATION_DATABASE, EsdashboardApplicationDatabase))
#define ESDASHBOARD_IS_APPLICATION_DATABASE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_APPLICATION_DATABASE))
#define ESDASHBOARD_APPLICATION_DATABASE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_APPLICATION_DATABASE, EsdashboardApplicationDatabaseClass))
#define ESDASHBOARD_IS_APPLICATION_DATABASE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_APPLICATION_DATABASE))
#define ESDASHBOARD_APPLICATION_DATABASE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_APPLICATION_DATABASE, EsdashboardApplicationDatabaseClass))

typedef struct _EsdashboardApplicationDatabase				EsdashboardApplicationDatabase;
typedef struct _EsdashboardApplicationDatabaseClass			EsdashboardApplicationDatabaseClass;
typedef struct _EsdashboardApplicationDatabasePrivate		EsdashboardApplicationDatabasePrivate;

struct _EsdashboardApplicationDatabase
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	EsdashboardApplicationDatabasePrivate	*priv;
};

struct _EsdashboardApplicationDatabaseClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*menu_reload_required)(EsdashboardApplicationDatabase *self);

	void (*application_added)(EsdashboardApplicationDatabase *self, GAppInfo *inAppInfo);
	void (*application_removed)(EsdashboardApplicationDatabase *self, GAppInfo *inAppInfo);
};

/* Public API */
GType esdashboard_application_database_get_type(void) G_GNUC_CONST;

EsdashboardApplicationDatabase* esdashboard_application_database_get_default(void);

gboolean esdashboard_application_database_is_loaded(const EsdashboardApplicationDatabase *self);
gboolean esdashboard_application_database_load(EsdashboardApplicationDatabase *self, GError **outError);

const GList* esdashboard_application_database_get_application_search_paths(const EsdashboardApplicationDatabase *self);

GarconMenu* esdashboard_application_database_get_application_menu(EsdashboardApplicationDatabase *self);
GList* esdashboard_application_database_get_all_applications(EsdashboardApplicationDatabase *self);

GAppInfo* esdashboard_application_database_lookup_desktop_id(EsdashboardApplicationDatabase *self,
																const gchar *inDesktopID);

gchar* esdashboard_application_database_get_file_from_desktop_id(const gchar *inDesktopID);
gchar* esdashboard_application_database_get_desktop_id_from_path(const gchar *inFilename);
gchar* esdashboard_application_database_get_desktop_id_from_file(GFile *inFile);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_APPLICATION_DATABASE__ */
