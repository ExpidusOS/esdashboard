/*
 * window-content: A content to share texture of a window
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

#ifndef __LIBESDASHBOARD_WINDOW_CONTENT__
#define __LIBESDASHBOARD_WINDOW_CONTENT__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/window-tracker-window.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_WINDOW_CONTENT				(esdashboard_window_content_get_type())
#define ESDASHBOARD_WINDOW_CONTENT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOW_CONTENT, EsdashboardWindowContent))
#define ESDASHBOARD_IS_WINDOW_CONTENT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOW_CONTENT))
#define ESDASHBOARD_WINDOW_CONTENT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_WINDOW_CONTENT, EsdashboardWindowContentClass))
#define ESDASHBOARD_IS_WINDOW_CONTENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_WINDOW_CONTENT))
#define ESDASHBOARD_WINDOW_CONTENT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_WINDOW_CONTENT, EsdashboardWindowContentClass))

typedef struct _EsdashboardWindowContent			EsdashboardWindowContent;
typedef struct _EsdashboardWindowContentClass		EsdashboardWindowContentClass;
typedef struct _EsdashboardWindowContentPrivate		EsdashboardWindowContentPrivate;

struct _EsdashboardWindowContent
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	EsdashboardWindowContentPrivate			*priv;
};

struct _EsdashboardWindowContentClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_window_content_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif
