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

#ifndef __LIBESDASHBOARD_WINDOW_CONTENT_X11__
#define __LIBESDASHBOARD_WINDOW_CONTENT_X11__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/x11/window-tracker-window-x11.h>
#include <libesdashboard/window-tracker-window.h>
#include <libesdashboard/types.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_WINDOW_CONTENT_X11				(esdashboard_window_content_x11_get_type())
#define ESDASHBOARD_WINDOW_CONTENT_X11(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOW_CONTENT_X11, EsdashboardWindowContentX11))
#define ESDASHBOARD_IS_WINDOW_CONTENT_X11(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOW_CONTENT_X11))
#define ESDASHBOARD_WINDOW_CONTENT_X11_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_WINDOW_CONTENT_X11, EsdashboardWindowContentX11Class))
#define ESDASHBOARD_IS_WINDOW_CONTENT_X11_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_WINDOW_CONTENT_X11))
#define ESDASHBOARD_WINDOW_CONTENT_X11_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_WINDOW_CONTENT_X11, EsdashboardWindowContentX11Class))

typedef struct _EsdashboardWindowContentX11				EsdashboardWindowContentX11;
typedef struct _EsdashboardWindowContentX11Class		EsdashboardWindowContentX11Class;
typedef struct _EsdashboardWindowContentX11Private		EsdashboardWindowContentX11Private;

struct _EsdashboardWindowContentX11
{
	/*< private >*/
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	EsdashboardWindowContentX11Private			*priv;
};

struct _EsdashboardWindowContentX11Class
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_window_content_x11_get_type(void) G_GNUC_CONST;

ClutterContent* esdashboard_window_content_x11_new_for_window(EsdashboardWindowTrackerWindowX11 *inWindow);

EsdashboardWindowTrackerWindow* esdashboard_window_content_x11_get_window(EsdashboardWindowContentX11 *self);

gboolean esdashboard_window_content_x11_is_suspended(EsdashboardWindowContentX11 *self);

const ClutterColor* esdashboard_window_content_x11_get_outline_color(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_outline_color(EsdashboardWindowContentX11 *self, const ClutterColor *inColor);

gfloat esdashboard_window_content_x11_get_outline_width(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_outline_width(EsdashboardWindowContentX11 *self, const gfloat inWidth);

gboolean esdashboard_window_content_x11_get_include_window_frame(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_include_window_frame(EsdashboardWindowContentX11 *self, const gboolean inIncludeFrame);

gboolean esdashboard_window_content_x11_get_unmapped_window_icon_x_fill(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_unmapped_window_icon_x_fill(EsdashboardWindowContentX11 *self, const gboolean inFill);

gboolean esdashboard_window_content_x11_get_unmapped_window_icon_y_fill(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_unmapped_window_icon_y_fill(EsdashboardWindowContentX11 *self, const gboolean inFill);

gfloat esdashboard_window_content_x11_get_unmapped_window_icon_x_align(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_unmapped_window_icon_x_align(EsdashboardWindowContentX11 *self, const gfloat inAlign);

gfloat esdashboard_window_content_x11_get_unmapped_window_icon_y_align(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_unmapped_window_icon_y_align(EsdashboardWindowContentX11 *self, const gfloat inAlign);

gfloat esdashboard_window_content_x11_get_unmapped_window_icon_x_scale(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_unmapped_window_icon_x_scale(EsdashboardWindowContentX11 *self, const gfloat inScale);

gfloat esdashboard_window_content_x11_get_unmapped_window_icon_y_scale(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_unmapped_window_icon_y_scale(EsdashboardWindowContentX11 *self, const gfloat inScale);

EsdashboardAnchorPoint esdashboard_window_content_x11_get_unmapped_window_icon_anchor_point(EsdashboardWindowContentX11 *self);
void esdashboard_window_content_x11_set_unmapped_window_icon_anchor_point(EsdashboardWindowContentX11 *self, const EsdashboardAnchorPoint inAnchorPoint);

G_END_DECLS

#endif
