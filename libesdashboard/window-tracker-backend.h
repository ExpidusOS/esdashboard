/*
 * window-tracker-backend: Window tracker backend providing special functions
 *                         for different windowing and clutter backends.
 * 
 * Copyright 2012-2016 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBESDASHBOARD_WINDOW_TRACKER_BACKEND__
#define __LIBESDASHBOARD_WINDOW_TRACKER_BACKEND__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libesdashboard/window-tracker.h>

G_BEGIN_DECLS

/* Object declaration */
#define ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND				(esdashboard_window_tracker_backend_get_type())
#define ESDASHBOARD_WINDOW_TRACKER_BACKEND(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND, EsdashboardWindowTrackerBackend))
#define ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND))
#define ESDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj), ESDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND, EsdashboardWindowTrackerBackendInterface))

typedef struct _EsdashboardWindowTrackerBackend				EsdashboardWindowTrackerBackend;
typedef struct _EsdashboardWindowTrackerBackendInterface	EsdashboardWindowTrackerBackendInterface;

/**
 * EsdashboardWindowTrackerBackendInterface:
 * @get_name: Name of window tracker backend
 * @get_window_tracker: Get window tracker instance used by backend
 * @get_stage_from_window: Get stage using requested window
 * @show_stage_window: Setup and show stage window
 * @hide_stage_window: Hide (and maybe deconfigure) stage window
 */
struct _EsdashboardWindowTrackerBackendInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface						parent_interface;

	/*< public >*/
	/* Virtual functions */
	const gchar* (*get_name)(EsdashboardWindowTrackerBackend *self);

	EsdashboardWindowTracker* (*get_window_tracker)(EsdashboardWindowTrackerBackend *self);

	EsdashboardWindowTrackerWindow* (*get_window_for_stage)(EsdashboardWindowTrackerBackend *self,
															ClutterStage *inStage);
	ClutterStage* (*get_stage_from_window)(EsdashboardWindowTrackerBackend *self,
											EsdashboardWindowTrackerWindow *inWindow);
	void (*show_stage_window)(EsdashboardWindowTrackerBackend *self,
								EsdashboardWindowTrackerWindow *inWindow);
	void (*hide_stage_window)(EsdashboardWindowTrackerBackend *self,
								EsdashboardWindowTrackerWindow *inWindow);
};


/* Public API */
GType esdashboard_window_tracker_backend_get_type(void) G_GNUC_CONST;

EsdashboardWindowTrackerBackend* esdashboard_window_tracker_backend_get_default(void);

void esdashboard_window_tracker_backend_set_backend(const gchar *inBackend);

const gchar* esdashboard_window_tracker_backend_get_name(EsdashboardWindowTrackerBackend *self);

EsdashboardWindowTracker* esdashboard_window_tracker_backend_get_window_tracker(EsdashboardWindowTrackerBackend *self);

EsdashboardWindowTrackerWindow* esdashboard_window_tracker_backend_get_window_for_stage(EsdashboardWindowTrackerBackend *self,
																						ClutterStage *inStage);
ClutterStage* esdashboard_window_tracker_backend_get_stage_from_window(EsdashboardWindowTrackerBackend *self,
																		EsdashboardWindowTrackerWindow *inWindow);
void esdashboard_window_tracker_backend_show_stage_window(EsdashboardWindowTrackerBackend *self,
															EsdashboardWindowTrackerWindow *inWindow);
void esdashboard_window_tracker_backend_hide_stage_window(EsdashboardWindowTrackerBackend *self,
															EsdashboardWindowTrackerWindow *inWindow);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_WINDOW_TRACKER_BACKEND__ */
