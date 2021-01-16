/*
 * middle-click-window-close: Closes windows in window by middle-click
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

#ifndef __ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE__
#define __ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE__

#include <libesdashboard/libesdashboard.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE				(esdashboard_middle_click_window_close_get_type())
#define ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE, EsdashboardMiddleClickWindowClose))
#define ESDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE))
#define ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE, EsdashboardMiddleClickWindowCloseClass))
#define ESDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE))
#define ESDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE, EsdashboardMiddleClickWindowCloseClass))

typedef struct _EsdashboardMiddleClickWindowClose				EsdashboardMiddleClickWindowClose; 
typedef struct _EsdashboardMiddleClickWindowClosePrivate		EsdashboardMiddleClickWindowClosePrivate;
typedef struct _EsdashboardMiddleClickWindowCloseClass			EsdashboardMiddleClickWindowCloseClass;

struct _EsdashboardMiddleClickWindowClose
{
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	EsdashboardMiddleClickWindowClosePrivate	*priv;
};

struct _EsdashboardMiddleClickWindowCloseClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;
};

/* Public API */
GType esdashboard_middle_click_window_close_get_type(void) G_GNUC_CONST;

ESDASHBOARD_DECLARE_PLUGIN_TYPE(esdashboard_middle_click_window_close);

EsdashboardMiddleClickWindowClose* esdashboard_middle_click_window_close_new(void);

G_END_DECLS

#endif
