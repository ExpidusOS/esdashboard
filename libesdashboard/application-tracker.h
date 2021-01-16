/*
 * application-tracker: A singleton managing states of applications
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

#ifndef __LIBESDASHBOARD_APPLICATION_TRACKER__
#define __LIBESDASHBOARD_APPLICATION_TRACKER__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_APPLICATION_TRACKER				(esdashboard_application_tracker_get_type())
#define ESDASHBOARD_APPLICATION_TRACKER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_APPLICATION_TRACKER, EsdashboardApplicationTracker))
#define ESDASHBOARD_IS_APPLICATION_TRACKER(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_APPLICATION_TRACKER))
#define ESDASHBOARD_APPLICATION_TRACKER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_APPLICATION_TRACKER, EsdashboardApplicationTrackerClass))
#define ESDASHBOARD_IS_APPLICATION_TRACKER_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_APPLICATION_TRACKER))
#define ESDASHBOARD_APPLICATION_TRACKER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_APPLICATION_TRACKER, EsdashboardApplicationTrackerClass))

typedef struct _EsdashboardApplicationTracker				EsdashboardApplicationTracker;
typedef struct _EsdashboardApplicationTrackerClass			EsdashboardApplicationTrackerClass;
typedef struct _EsdashboardApplicationTrackerPrivate		EsdashboardApplicationTrackerPrivate;

struct _EsdashboardApplicationTracker
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	EsdashboardApplicationTrackerPrivate	*priv;
};

struct _EsdashboardApplicationTrackerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*state_changed)(EsdashboardApplicationTracker *self, const gchar *inDesktopID, gboolean inRunning);
};

/* Public API */
GType esdashboard_application_tracker_get_type(void) G_GNUC_CONST;

EsdashboardApplicationTracker* esdashboard_application_tracker_get_default(void);

gboolean esdashboard_application_tracker_is_running_by_desktop_id(EsdashboardApplicationTracker *self,
																	const gchar *inDesktopID);
gboolean esdashboard_application_tracker_is_running_by_app_info(EsdashboardApplicationTracker *self,
																GAppInfo *inAppInfo);

const GList* esdashboard_application_tracker_get_window_list_by_desktop_id(EsdashboardApplicationTracker *self,
																			const gchar *inDesktopID);
const GList*  esdashboard_application_tracker_get_window_list_by_app_info(EsdashboardApplicationTracker *self,
																			GAppInfo *inAppInfo);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_APPLICATION_TRACKER__ */
