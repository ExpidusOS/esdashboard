/*
 * window-tracker-workspace: A workspace tracked by window tracker and
 *                           also a wrapper class around WnckWorkspace.
 *                           By wrapping libwnck objects we can use a 
 *                           virtual stable API while the API in libwnck
 *                           changes within versions. We only need to
 *                           use #ifdefs in window tracker object and
 *                           nowhere else in the code.
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

/**
 * SECTION:window-tracker-workspace-x11
 * @short_description: A workspace used by X11 window tracker
 * @include: esdashboard/x11/window-tracker-workspace-x11.h
 *
 * This is the X11 backend of #EsdashboardWindowTrackerWorkspace
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/x11/window-tracker-workspace-x11.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/x11/window-tracker-x11.h>
#include <libesdashboard/window-tracker.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
static void _esdashboard_window_tracker_workspace_x11_window_tracker_workspace_iface_init(EsdashboardWindowTrackerWorkspaceInterface *iface);

struct _EsdashboardWindowTrackerWorkspaceX11Private
{
	/* Properties related */
	WnckWorkspace							*workspace;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardWindowTrackerWorkspaceX11,
						esdashboard_window_tracker_workspace_x11,
						G_TYPE_OBJECT,
						G_ADD_PRIVATE(EsdashboardWindowTrackerWorkspaceX11)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE, _esdashboard_window_tracker_workspace_x11_window_tracker_workspace_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_WORKSPACE,

	PROP_LAST
};

static GParamSpec* EsdashboardWindowTrackerWorkspaceX11Properties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11_WARN_NO_WORKSPACE(self)       \
	g_critical("No wnck workspace wrapped at %s in called function %s",        \
				G_OBJECT_TYPE_NAME(self),                                      \
				__func__);

#define ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11_WARN_WRONG_WORKSPACE(self)    \
	g_critical("Got signal from wrong wnck workspace wrapped at %s in called function %s",\
				G_OBJECT_TYPE_NAME(self),                                      \
				__func__);

/* Proxy signal for mapped wnck window which changed name */
static void _esdashboard_window_tracker_workspace_x11_on_wnck_name_changed(EsdashboardWindowTrackerWorkspaceX11 *self,
																			gpointer inUserData)
{
	EsdashboardWindowTrackerWorkspaceX11Private		*priv;
	WnckWorkspace									*workspace;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(self));
	g_return_if_fail(WNCK_IS_WORKSPACE(inUserData));

	priv=self->priv;
	workspace=WNCK_WORKSPACE(inUserData);

	/* Check that workspace emitting this signal is the mapped workspace of this object */
	if(priv->workspace!=workspace)
	{
		ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11_WARN_WRONG_WORKSPACE(self);
		return;
	}

	/* Proxy signal */
	g_signal_emit_by_name(self, "name-changed");
}

/* Set wnck workspace to map in this workspace object */
static void _esdashboard_window_tracker_workspace_x11_set_workspace(EsdashboardWindowTrackerWorkspaceX11 *self,
																	WnckWorkspace *inWorkspace)
{
	EsdashboardWindowTrackerWorkspaceX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(self));
	g_return_if_fail(!inWorkspace || WNCK_IS_WORKSPACE(inWorkspace));

	priv=self->priv;

	/* Set value if changed */
	if(priv->workspace!=inWorkspace)
	{
		/* Disconnect signals to old window (if available) and reset states */
		if(priv->workspace)
		{
			/* Remove weak reference at old workspace */
			g_object_remove_weak_pointer(G_OBJECT(priv->workspace), (gpointer*)&priv->workspace);

			/* Disconnect signal handlers */
			g_signal_handlers_disconnect_by_data(priv->workspace, self);
			priv->workspace=NULL;
		}

		/* Set new value */
		priv->workspace=inWorkspace;

		/* Initialize states and connect signals if window is set */
		if(priv->workspace)
		{
			/* Add weak reference at new workspace */
			g_object_add_weak_pointer(G_OBJECT(priv->workspace), (gpointer*)&priv->workspace);

			/* Connect signals */
			g_signal_connect_swapped(priv->workspace,
										"name-changed",
										G_CALLBACK(_esdashboard_window_tracker_workspace_x11_on_wnck_name_changed),
										self);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowTrackerWorkspaceX11Properties[PROP_WORKSPACE]);
	}
}


/* IMPLEMENTATION: Interface EsdashboardWindowTrackerWorkspace */

/* Get number of workspace */
static gint _esdashboard_window_tracker_workspace_x11_window_tracker_workspace_get_number(EsdashboardWindowTrackerWorkspace *inWorkspace)
{
	EsdashboardWindowTrackerWorkspaceX11			*self;
	EsdashboardWindowTrackerWorkspaceX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace), -1);

	self=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace);
	priv=self->priv;

	/* A wnck workspace must be wrapped by this object */
	if(!priv->workspace)
	{
		ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11_WARN_NO_WORKSPACE(self);
		return(-1);
	}

	/* Return number of workspace */
	return(wnck_workspace_get_number(priv->workspace));
}

/* Get name of workspace */
static const gchar* _esdashboard_window_tracker_workspace_x11_window_tracker_workspace_get_name(EsdashboardWindowTrackerWorkspace *inWorkspace)
{
	EsdashboardWindowTrackerWorkspaceX11			*self;
	EsdashboardWindowTrackerWorkspaceX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace), NULL);

	self=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace);
	priv=self->priv;

	/* A wnck workspace must be wrapped by this object */
	if(!priv->workspace)
	{
		ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11_WARN_NO_WORKSPACE(self);
		return(NULL);
	}

	/* Return name of workspace */
	return(wnck_workspace_get_name(priv->workspace));
}

/* Get size of workspace */
static void _esdashboard_window_tracker_workspace_x11_window_tracker_workspace_get_size(EsdashboardWindowTrackerWorkspace *inWorkspace,
																						gint *outWidth,
																						gint *outHeight)
{
	EsdashboardWindowTrackerWorkspaceX11			*self;
	EsdashboardWindowTrackerWorkspaceX11Private		*priv;
	gint											width, height;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));

	self=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace);
	priv=self->priv;

	/* A wnck workspace must be wrapped by this object */
	if(!priv->workspace)
	{
		ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11_WARN_NO_WORKSPACE(self);
		return;
	}

	/* Get width and height of workspace */
	width=wnck_workspace_get_width(priv->workspace);
	height=wnck_workspace_get_height(priv->workspace);

	/* Set values */
	if(outWidth) *outWidth=width;
	if(outHeight) *outHeight=height;
}

/* Determine if this workspace is the active one */
static gboolean _esdashboard_window_tracker_workspace_x11_window_tracker_workspace_is_active(EsdashboardWindowTrackerWorkspace *inWorkspace)
{
	EsdashboardWindowTrackerWorkspaceX11			*self;
	EsdashboardWindowTrackerWorkspaceX11Private		*priv;
	EsdashboardWindowTracker						*windowTracker;
	EsdashboardWindowTrackerWorkspace				*activeWorkspace;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace), FALSE);

	self=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace);
	priv=self->priv;

	/* A wnck workspace must be wrapped by this object */
	if(!priv->workspace)
	{
		ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11_WARN_NO_WORKSPACE(self);
		return(FALSE);
	}

	/* Get current active workspace */
	windowTracker=esdashboard_window_tracker_get_default();
	activeWorkspace=esdashboard_window_tracker_get_active_workspace(windowTracker);
	g_object_unref(windowTracker);

	/* Return TRUE if current active workspace is this workspace */
	return(esdashboard_window_tracker_workspace_is_equal(inWorkspace, activeWorkspace));
}

/* Activate workspace */
static void _esdashboard_window_tracker_workspace_x11_window_tracker_workspace_activate(EsdashboardWindowTrackerWorkspace *inWorkspace)
{
	EsdashboardWindowTrackerWorkspaceX11			*self;
	EsdashboardWindowTrackerWorkspaceX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));

	self=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace);
	priv=self->priv;

	/* A wnck workspace must be wrapped by this object */
	if(!priv->workspace)
	{
		ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11_WARN_NO_WORKSPACE(self);
		return;
	}

	/* Activate workspace */
	wnck_workspace_activate(priv->workspace, esdashboard_window_tracker_x11_get_time());
}

/* Interface initialization
 * Set up default functions
 */
static void _esdashboard_window_tracker_workspace_x11_window_tracker_workspace_iface_init(EsdashboardWindowTrackerWorkspaceInterface *iface)
{
	iface->get_number=_esdashboard_window_tracker_workspace_x11_window_tracker_workspace_get_number;
	iface->get_name=_esdashboard_window_tracker_workspace_x11_window_tracker_workspace_get_name;

	iface->get_size=_esdashboard_window_tracker_workspace_x11_window_tracker_workspace_get_size;

	iface->is_active=_esdashboard_window_tracker_workspace_x11_window_tracker_workspace_is_active;
	iface->activate=_esdashboard_window_tracker_workspace_x11_window_tracker_workspace_activate;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_window_tracker_workspace_x11_dispose(GObject *inObject)
{
	EsdashboardWindowTrackerWorkspaceX11			*self=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inObject);
	EsdashboardWindowTrackerWorkspaceX11Private		*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->workspace)
	{
		/* Remove weak reference at current workspace */
		g_object_remove_weak_pointer(G_OBJECT(priv->workspace), (gpointer*)&priv->workspace);

		/* Disconnect signal handlers */
		g_signal_handlers_disconnect_by_data(priv->workspace, self);
		priv->workspace=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_window_tracker_workspace_x11_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_window_tracker_workspace_x11_set_property(GObject *inObject,
																	guint inPropID,
																	const GValue *inValue,
																	GParamSpec *inSpec)
{
	EsdashboardWindowTrackerWorkspaceX11		*self=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inObject);

	switch(inPropID)
	{
		case PROP_WORKSPACE:
			_esdashboard_window_tracker_workspace_x11_set_workspace(self, WNCK_WORKSPACE(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_window_tracker_workspace_x11_get_property(GObject *inObject,
																	guint inPropID,
																	GValue *outValue,
																	GParamSpec *inSpec)
{
	EsdashboardWindowTrackerWorkspaceX11		*self=ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inObject);

	switch(inPropID)
	{
		case PROP_WORKSPACE:
			g_value_set_object(outValue, self->priv->workspace);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_window_tracker_workspace_x11_class_init(EsdashboardWindowTrackerWorkspaceX11Class *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_window_tracker_workspace_x11_dispose;
	gobjectClass->set_property=_esdashboard_window_tracker_workspace_x11_set_property;
	gobjectClass->get_property=_esdashboard_window_tracker_workspace_x11_get_property;

	/* Define properties */
	EsdashboardWindowTrackerWorkspaceX11Properties[PROP_WORKSPACE]=
		g_param_spec_object("workspace",
							"Window",
							"The mapped wnck workspace",
							WNCK_TYPE_WORKSPACE,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardWindowTrackerWorkspaceX11Properties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_window_tracker_workspace_x11_init(EsdashboardWindowTrackerWorkspaceX11 *self)
{
	EsdashboardWindowTrackerWorkspaceX11Private	*priv;

	priv=self->priv=esdashboard_window_tracker_workspace_x11_get_instance_private(self);

	/* Set default values */
	priv->workspace=NULL;
}


/* IMPLEMENTATION: Public API */

/**
 * esdashboard_window_tracker_workspace_x11_get_workspace:
 * @self: A #EsdashboardWindowTrackerWorkspaceX11
 *
 * Returns the wrapped workspace of libwnck.
 *
 * Return value: (transfer none): the #WnckWorkspace wrapped by @self. The returned
 *   #WnckWorkspace is owned by libwnck and must not be referenced or unreferenced.
 */
WnckWorkspace* esdashboard_window_tracker_workspace_x11_get_workspace(EsdashboardWindowTrackerWorkspaceX11 *self)
{
	EsdashboardWindowTrackerWorkspaceX11Private		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(self), NULL);

	priv=self->priv;

	/* A wnck workspace must be wrapped by this object */
	if(!priv->workspace)
	{
		ESDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11_WARN_NO_WORKSPACE(self);
		return(NULL);
	}

	/* Return wrapped libwnck workspace */
	return(priv->workspace);
}
