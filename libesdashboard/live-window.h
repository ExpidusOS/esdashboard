/*
 * live-window: An actor showing the content of a window which will
 *              be updated if changed and visible on active workspace.
 *              It also provides controls to manipulate it.
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

#ifndef __LIBESDASHBOARD_LIVE_WINDOW__
#define __LIBESDASHBOARD_LIVE_WINDOW__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/live-window-simple.h>
#include <libesdashboard/window-tracker.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_LIVE_WINDOW				(esdashboard_live_window_get_type())
#define ESDASHBOARD_LIVE_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_LIVE_WINDOW, EsdashboardLiveWindow))
#define ESDASHBOARD_IS_LIVE_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_LIVE_WINDOW))
#define ESDASHBOARD_LIVE_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_LIVE_WINDOW, EsdashboardLiveWindowClass))
#define ESDASHBOARD_IS_LIVE_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_LIVE_WINDOW))
#define ESDASHBOARD_LIVE_WINDOW_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_LIVE_WINDOW, EsdashboardLiveWindowClass))

typedef struct _EsdashboardLiveWindow				EsdashboardLiveWindow;
typedef struct _EsdashboardLiveWindowClass			EsdashboardLiveWindowClass;
typedef struct _EsdashboardLiveWindowPrivate		EsdashboardLiveWindowPrivate;

struct _EsdashboardLiveWindow
{
	/*< private >*/
	/* Parent instance */
	EsdashboardLiveWindowSimple			parent_instance;

	/* Private structure */
	EsdashboardLiveWindowPrivate		*priv;
};

struct _EsdashboardLiveWindowClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardLiveWindowSimpleClass	parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(EsdashboardLiveWindow *self);
	void (*close)(EsdashboardLiveWindow *self);
};

/* Public API */
GType esdashboard_live_window_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_live_window_new(void);
ClutterActor* esdashboard_live_window_new_for_window(EsdashboardWindowTrackerWindow *inWindow);

gfloat esdashboard_live_window_get_title_actor_padding(EsdashboardLiveWindow *self);
void esdashboard_live_window_set_title_actor_padding(EsdashboardLiveWindow *self, gfloat inPadding);

gfloat esdashboard_live_window_get_close_button_padding(EsdashboardLiveWindow *self);
void esdashboard_live_window_set_close_button_padding(EsdashboardLiveWindow *self, gfloat inPadding);

gboolean esdashboard_live_window_get_show_subwindows(EsdashboardLiveWindow *self);
void esdashboard_live_window_set_show_subwindows(EsdashboardLiveWindow *self, gboolean inShowSubwindows);

gboolean esdashboard_live_window_get_allow_subwindows(EsdashboardLiveWindow *self);
void esdashboard_live_window_set_allow_subwindows(EsdashboardLiveWindow *self, gboolean inAllowSubwindows);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_LIVE_WINDOW__ */
