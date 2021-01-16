/*
 * window-tracker-backend: Window tracker backend providing special functions
 *                         for different windowing and clutter backends.
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

#ifndef __LIBESDASHBOARD_WINDOW_TRACKER_BACKEND_GDK__
#define __LIBESDASHBOARD_WINDOW_TRACKER_BACKEND_GDK__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libesdashboard/window-tracker-backend.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_GDK				(esdashboard_window_tracker_backend_gdk_get_type())
#define ESDASHBOARD_WINDOW_TRACKER_BACKEND_GDK(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_GDK, EsdashboardWindowTrackerBackendGDK))
#define ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND_GDK(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_GDK))
#define ESDASHBOARD_WINDOW_TRACKER_BACKEND_GDK_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_GDK, EsdashboardWindowTrackerBackendGDKClass))
#define ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND_GDK_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_GDK))
#define ESDASHBOARD_WINDOW_TRACKER_BACKEND_GDK_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_GDK, EsdashboardWindowTrackerBackendGDKClass))

typedef struct _EsdashboardWindowTrackerBackendGDK				EsdashboardWindowTrackerBackendGDK;
typedef struct _EsdashboardWindowTrackerBackendGDKClass			EsdashboardWindowTrackerBackendGDKClass;
typedef struct _EsdashboardWindowTrackerBackendGDKPrivate		EsdashboardWindowTrackerBackendGDKPrivate;

struct _EsdashboardWindowTrackerBackendGDK
{
	/*< private >*/
	/* Parent instance */
	GObject											parent_instance;

	/* Private structure */
	EsdashboardWindowTrackerBackendGDKPrivate		*priv;
};

struct _EsdashboardWindowTrackerBackendGDKClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass									parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_window_tracker_backend_gdk_get_type(void) G_GNUC_CONST;

EsdashboardWindowTrackerBackend* esdashboard_window_tracker_backend_gdk_new(void);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_WINDOW_TRACKER_BACKEND_GDK__ */
