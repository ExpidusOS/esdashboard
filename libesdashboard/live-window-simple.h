/*
 * live-window-simple: An actor showing the content of a window which will
 *                     be updated if changed and visible on active workspace.
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

#ifndef __LIBESDASHBOARD_LIVE_WINDOW_SIMPLE__
#define __LIBESDASHBOARD_LIVE_WINDOW_SIMPLE__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/background.h>
#include <libesdashboard/window-tracker.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * EsdashboardLiveWindowSimpleDisplayType:
 * @ESDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_LIVE_PREVIEW: The actor will show a live preview of window
 * @ESDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_ICON: The actor will show the window's icon in size of window
 *
 * Determines how the window will be displayed.
 */
typedef enum /*< prefix=ESDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE >*/
{
	ESDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_LIVE_PREVIEW=0,
	ESDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_ICON,
} EsdashboardLiveWindowSimpleDisplayType;


/* Object declaration */
#define ESDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE				(esdashboard_live_window_simple_get_type())
#define ESDASHBOARD_LIVE_WINDOW_SIMPLE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE, EsdashboardLiveWindowSimple))
#define ESDASHBOARD_IS_LIVE_WINDOW_SIMPLE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE))
#define ESDASHBOARD_LIVE_WINDOW_SIMPLE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE, EsdashboardLiveWindowSimpleClass))
#define ESDASHBOARD_IS_LIVE_WINDOW_SIMPLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE))
#define ESDASHBOARD_LIVE_WINDOW_SIMPLE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE, EsdashboardLiveWindowSimpleClass))

typedef struct _EsdashboardLiveWindowSimple				EsdashboardLiveWindowSimple;
typedef struct _EsdashboardLiveWindowSimpleClass		EsdashboardLiveWindowSimpleClass;
typedef struct _EsdashboardLiveWindowSimplePrivate		EsdashboardLiveWindowSimplePrivate;

struct _EsdashboardLiveWindowSimple
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground			parent_instance;

	/* Private structure */
	EsdashboardLiveWindowSimplePrivate	*priv;
};

struct _EsdashboardLiveWindowSimpleClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*geometry_changed)(EsdashboardLiveWindowSimple *self);
	void (*visibility_changed)(EsdashboardLiveWindowSimple *self, gboolean inVisible);
	void (*workspace_changed)(EsdashboardLiveWindowSimple *self);
};

/* Public API */
GType esdashboard_live_window_simple_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_live_window_simple_new(void);
ClutterActor* esdashboard_live_window_simple_new_for_window(EsdashboardWindowTrackerWindow *inWindow);

EsdashboardWindowTrackerWindow* esdashboard_live_window_simple_get_window(EsdashboardLiveWindowSimple *self);
void esdashboard_live_window_simple_set_window(EsdashboardLiveWindowSimple *self, EsdashboardWindowTrackerWindow *inWindow);

EsdashboardLiveWindowSimpleDisplayType esdashboard_live_window_simple_get_display_type(EsdashboardLiveWindowSimple *self);
void esdashboard_live_window_simple_set_display_type(EsdashboardLiveWindowSimple *self, EsdashboardLiveWindowSimpleDisplayType inType);

gboolean esdashboard_live_window_simple_get_destroy_on_close(EsdashboardLiveWindowSimple *self);
void esdashboard_live_window_simple_set_destroy_on_close(EsdashboardLiveWindowSimple *self, gboolean inDestroyOnClose);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_LIVE_WINDOW_SIMPLE__ */
