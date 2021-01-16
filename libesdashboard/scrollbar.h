/*
 * scrollbar: A scroll bar
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

#ifndef __LIBESDASHBOARD_SCROLLBAR__
#define __LIBESDASHBOARD_SCROLLBAR__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/background.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SCROLLBAR				(esdashboard_scrollbar_get_type())
#define ESDASHBOARD_SCROLLBAR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SCROLLBAR, EsdashboardScrollbar))
#define ESDASHBOARD_IS_SCROLLBAR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SCROLLBAR))
#define ESDASHBOARD_SCROLLBAR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SCROLLBAR, EsdashboardScrollbarClass))
#define ESDASHBOARD_IS_SCROLLBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SCROLLBAR))
#define ESDASHBOARD_SCROLLBAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SCROLLBAR, EsdashboardScrollbarClass))

typedef struct _EsdashboardScrollbar			EsdashboardScrollbar; 
typedef struct _EsdashboardScrollbarPrivate		EsdashboardScrollbarPrivate;
typedef struct _EsdashboardScrollbarClass		EsdashboardScrollbarClass;

struct _EsdashboardScrollbar
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground			parent_instance;

	/* Private structure */
	EsdashboardScrollbarPrivate		*priv;
};

struct _EsdashboardScrollbarClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*value_changed)(EsdashboardScrollbar *self, gfloat inValue);
};

/* Public API */
GType esdashboard_scrollbar_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_scrollbar_new(ClutterOrientation inOrientation);

gfloat esdashboard_scrollbar_get_orientation(EsdashboardScrollbar *self);
void esdashboard_scrollbar_set_orientation(EsdashboardScrollbar *self, ClutterOrientation inOrientation);

gfloat esdashboard_scrollbar_get_value(EsdashboardScrollbar *self);
void esdashboard_scrollbar_set_value(EsdashboardScrollbar *self, gfloat inValue);

gfloat esdashboard_scrollbar_get_value_range(EsdashboardScrollbar *self);

gfloat esdashboard_scrollbar_get_range(EsdashboardScrollbar *self);
void esdashboard_scrollbar_set_range(EsdashboardScrollbar *self, gfloat inRange);

gfloat esdashboard_scrollbar_get_page_size_factor(EsdashboardScrollbar *self);
void esdashboard_scrollbar_set_page_size_factor(EsdashboardScrollbar *self, gfloat inFactor);

gfloat esdashboard_scrollbar_get_spacing(EsdashboardScrollbar *self);
void esdashboard_scrollbar_set_spacing(EsdashboardScrollbar *self, gfloat inSpacing);

gfloat esdashboard_scrollbar_get_slider_width(EsdashboardScrollbar *self);
void esdashboard_scrollbar_set_slider_width(EsdashboardScrollbar *self, gfloat inWidth);

gfloat esdashboard_scrollbar_get_slider_radius(EsdashboardScrollbar *self);
void esdashboard_scrollbar_set_slider_radius(EsdashboardScrollbar *self, gfloat inRadius);

const ClutterColor* esdashboard_scrollbar_get_slider_color(EsdashboardScrollbar *self);
void esdashboard_scrollbar_set_slider_color(EsdashboardScrollbar *self, const ClutterColor *inColor);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_SCROLLBAR__ */
