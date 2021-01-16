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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/window-tracker-backend.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/x11/window-tracker-backend-x11.h>
#ifdef HAVE_BACKEND_GDK
#include <libesdashboard/gdk/window-tracker-backend-gdk.h>
#endif
#include <libesdashboard/application.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
G_DEFINE_INTERFACE(EsdashboardWindowTrackerBackend,
					esdashboard_window_tracker_backend,
					G_TYPE_OBJECT)


/* IMPLEMENTATION: Private variables and methods */
#define ESDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, vfunc)  \
	g_warning("Object of type %s does not implement required virtual function EsdashboardWindowTrackerBackend::%s",\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

struct _EsdashboardWindowTrackerBackendMap
{
	const gchar							*backendID;
	const gchar							*clutterBackendID;
	EsdashboardWindowTrackerBackend*	(*createBackend)(void);
};
typedef struct _EsdashboardWindowTrackerBackendMap	EsdashboardWindowTrackerBackendMap;

static EsdashboardWindowTrackerBackendMap	_esdashboard_window_tracker_backend_map[]=
											{
#ifdef CLUTTER_WINDOWING_X11
												{ "x11", CLUTTER_WINDOWING_X11, esdashboard_window_tracker_backend_x11_new },
#endif
#ifdef HAVE_BACKEND_GDK
#ifdef   CLUTTER_WINDOWING_GDK
												{ "gdk", CLUTTER_WINDOWING_GDK, esdashboard_window_tracker_backend_gdk_new },
#endif   /* CLUTTER_WINDOWING_GDK */
#endif /* HAVE_BACKEND_GDK */
												{ NULL, NULL, NULL }
											};

static EsdashboardWindowTrackerBackend		*_esdashboard_window_tracker_backend_singleton=NULL;


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void esdashboard_window_tracker_backend_default_init(EsdashboardWindowTrackerBackendInterface *iface)
{
	static gboolean		initialized=FALSE;

	/* Define properties, signals and actions */
	if(!initialized)
	{
		/* Set flag that base initialization was done for this interface */
		initialized=TRUE;
	}
}


/* IMPLEMENTATION: Public API */

/**
 * esdashboard_window_tracker_backend_get_default:
 *
 * Retrieves the singleton instance of #EsdashboardWindowTrackerBackend. If not
 * needed anymore the caller must unreference the returned object instance.
 *
 * Return value: (transfer full): The instance of #EsdashboardWindowTrackerBackend.
 */
EsdashboardWindowTrackerBackend* esdashboard_window_tracker_backend_get_default(void)
{
	if(G_UNLIKELY(_esdashboard_window_tracker_backend_singleton==NULL))
	{
		EsdashboardWindowTrackerBackendMap	*iter;

		/* Iterate through list of available backends and check if any entry
		 * matches the backend Clutter is using. If we can find a matching entry
		 * then create our backend which interacts with Clutter's backend.
		 */
		for(iter=_esdashboard_window_tracker_backend_map; !_esdashboard_window_tracker_backend_singleton && iter->backendID; iter++)
		{
			/* If this entry does not match backend Clutter, try next one */
			if(!clutter_check_windowing_backend(iter->clutterBackendID)) continue;

			/* The entry matches so try to create our backend */
			ESDASHBOARD_DEBUG(NULL, WINDOWS,
								"Found window tracker backend ID '%s' for clutter backend '%s'",
								iter->backendID,
								iter->clutterBackendID);

			_esdashboard_window_tracker_backend_singleton=(iter->createBackend)();
			if(!_esdashboard_window_tracker_backend_singleton)
			{
				ESDASHBOARD_DEBUG(NULL, WINDOWS,
									"Could not create window tracker backend of ID '%s' for clutter backend '%s'",
									iter->backendID,
									iter->clutterBackendID);
			}
				else
				{
					ESDASHBOARD_DEBUG(_esdashboard_window_tracker_backend_singleton, WINDOWS,
										"Create window tracker backend of type %s with ID '%s' for clutter backend '%s'",
										G_OBJECT_TYPE_NAME(_esdashboard_window_tracker_backend_singleton),
										iter->backendID,
										iter->clutterBackendID);
				}
		}

		if(!_esdashboard_window_tracker_backend_singleton)
		{
			g_critical("Cannot find any usable window tracker backend");
			return(NULL);
		}
	}
		else g_object_ref(_esdashboard_window_tracker_backend_singleton);

	return(_esdashboard_window_tracker_backend_singleton);
}

/**
 * esdashboard_window_tracker_backend_set_backend:
 * @inBackend: the backend to use
 *
 * Sets the backend that esdashboard should try to use. It will also restrict
 * the backend Clutter should try to use. By default esdashboard will select
 * the backend automatically based on the backend Clutter uses.
 *
 * For example:
 *
 * |[<!-- language="C" -->
 *   esdashboard_window_tracker_backend_set_allowed_backends("x11");
 * ]|
 *
 * Will make esdashboard and Clutter use the X11 backend.
 *
 * Possible backends are: x11 and gdk.
 *
 * This function must be called before the first API call to esdashboard or any
 * library esdashboard depends on like Clutter, GTK+ etc. This function can also
 * be called only once.
 */
void esdashboard_window_tracker_backend_set_backend(const gchar *inBackend)
{
#if CLUTTER_CHECK_VERSION(1, 16, 0)
	EsdashboardWindowTrackerBackendMap	*iter;
	static gboolean						wasSet=FALSE;

	g_return_if_fail(inBackend && *inBackend);

	/* Warn if this function was called more than once */
	if(wasSet)
	{
		g_critical("Cannot set backend to '%s' because it the backend was already set",
					inBackend);
		return;
	}

	/* Set flag that this function was called regardless of the result of this
	 * function call.
	 */
	wasSet=TRUE;

	/* Backend can only be set if application was not already created */
	if(esdashboard_application_has_default())
	{
		g_critical("Cannot set backend to '%s' because application is already initialized",
					inBackend);
		return;
	}

	/* Iterate through list of available backends and lookup the requested
	 * backend. If this entry is found, restrict Clutter backend as listed in
	 * found entry and return.
	 */
	for(iter=_esdashboard_window_tracker_backend_map; iter->backendID; iter++)
	{
		/* If this entry does not match requested backend, try next one */
		if(g_strcmp0(iter->backendID, inBackend)!=0) continue;

		/* The entry matches so restrict allowed backends in Clutter to the one
		 * listed at this entry.
		 */
		clutter_set_windowing_backend(iter->clutterBackendID);

		return;
	}

	/* If we get here the requested backend is unknown */
	g_warning("Unknown backend '%s' - using default backend", inBackend);
#endif
}

/**
 * esdashboard_window_tracker_backend_get_name:
 * @self: A #EsdashboardWindowTrackerBackend
 *
 * Retrieves the name of #EsdashboardWindowTrackerBackend at @self.
 *
 * Return value: String containing the name of the window tracker backend.
 */
const gchar* esdashboard_window_tracker_backend_get_name(EsdashboardWindowTrackerBackend *self)
{
	EsdashboardWindowTrackerBackendInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_name)
	{
		return(iface->get_name(self));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "get_name");
	return(NULL);
}

/**
 * esdashboard_window_tracker_backend_get_window_tracker:
 * @self: A #EsdashboardWindowTrackerBackend
 *
 * Retrieves the #EsdashboardWindowTracker used by backend @self. If not needed
 * anymore the caller must unreference the returned object instance.
 *
 * Return value: (transfer full): The instance of #EsdashboardWindowTracker.
 */
EsdashboardWindowTracker* esdashboard_window_tracker_backend_get_window_tracker(EsdashboardWindowTrackerBackend *self)
{
	EsdashboardWindowTrackerBackendInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_window_tracker)
	{
		EsdashboardWindowTracker					*windowTracker;

		windowTracker=iface->get_window_tracker(self);
		if(windowTracker) g_object_ref(windowTracker);

		return(windowTracker);
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "get_window_tracker");
	return(NULL);
}

/**
 * esdashboard_window_tracker_backend_get_window_for_stage:
 * @self: A #EsdashboardWindowTrackerBackend
 * @inStage: A #ClutterStage
 *
 * Retrieves the window created for the requested stage @inStage from window
 * tracker backend @self.
 *
 * Return value: (transfer none): The #EsdashboardWindowTrackerWindow representing
 *   the window of requested stage or %NULL if not available. The returned object
 *   is owned by Esdashboard and it should not be referenced or unreferenced.
 */
EsdashboardWindowTrackerWindow* esdashboard_window_tracker_backend_get_window_for_stage(EsdashboardWindowTrackerBackend *self,
																						ClutterStage *inStage)
{
	EsdashboardWindowTrackerBackendInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_window_for_stage)
	{
		return(iface->get_window_for_stage(self, inStage));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "get_window_for_stage");
	return(NULL);
}

/**
 * esdashboard_window_tracker_backend_get_stage_from_window:
 * @self: A #EsdashboardWindowTrackerBackend
 * @inWindow: A #EsdashboardWindowTrackerWindow defining the stage window
 *
 * Asks the window tracker backend @self to find the #ClutterStage which uses
 * stage window @inWindow.
 *
 * Return value: (transfer none): The #ClutterStage for stage window @inWindow or
 *   %NULL if @inWindow is not a stage window or stage could not be found.
 */
ClutterStage* esdashboard_window_tracker_backend_get_stage_from_window(EsdashboardWindowTrackerBackend *self,
																		EsdashboardWindowTrackerWindow *inWindow)
{
	EsdashboardWindowTrackerBackendInterface		*iface;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	iface=ESDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_stage_from_window)
	{
		return(iface->get_stage_from_window(self, inWindow));
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "get_stage_from_window");
	return(NULL);
}

/**
 * esdashboard_window_tracker_backend_show_stage_window:
 * @self: A #EsdashboardWindowTrackerBackend
 * @inWindow: A #EsdashboardWindowTrackerWindow defining the stage window
 *
 * Asks the window tracker backend @self to set up and show the window @inWindow
 * for use as stage window.
 */
void esdashboard_window_tracker_backend_show_stage_window(EsdashboardWindowTrackerBackend *self,
															EsdashboardWindowTrackerWindow *inWindow)
{
	EsdashboardWindowTrackerBackendInterface		*iface;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	iface=ESDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->show_stage_window)
	{
		iface->show_stage_window(self, inWindow);
		return;
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "show_stage_window");
}

/**
 * esdashboard_window_tracker_backend_hide_stage_window:
 * @self: A #EsdashboardWindowTrackerBackend
 * @inWindow: A #EsdashboardWindowTrackerWindow defining the stage window
 *
 * Asks the window tracker backend @self to hide the stage window @inWindow.
 */
void esdashboard_window_tracker_backend_hide_stage_window(EsdashboardWindowTrackerBackend *self,
															EsdashboardWindowTrackerWindow *inWindow)
{
	EsdashboardWindowTrackerBackendInterface		*iface;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	iface=ESDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->hide_stage_window)
	{
		iface->hide_stage_window(self, inWindow);
		return;
	}

	/* If we get here the virtual function was not overridden */
	ESDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "hide_stage_window");
}
