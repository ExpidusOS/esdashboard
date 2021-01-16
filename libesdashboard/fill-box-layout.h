/*
 * fill-box-layout: A box layout expanding actors in one direction
 *                  (fill to fit parent's size) and using natural
 *                  size in other direction
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

#ifndef __LIBESDASHBOARD_FILL_BOX_LAYOUT__
#define __LIBESDASHBOARD_FILL_BOX_LAYOUT__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_FILL_BOX_LAYOUT			(esdashboard_fill_box_layout_get_type())
#define ESDASHBOARD_FILL_BOX_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_FILL_BOX_LAYOUT, EsdashboardFillBoxLayout))
#define ESDASHBOARD_IS_FILL_BOX_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_FILL_BOX_LAYOUT))
#define ESDASHBOARD_FILL_BOX_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_FILL_BOX_LAYOUT, EsdashboardFillBoxLayoutClass))
#define ESDASHBOARD_IS_FILL_BOX_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_FILL_BOX_LAYOUT))
#define ESDASHBOARD_FILL_BOX_LAYOUT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_FILL_BOX_LAYOUT, EsdashboardFillBoxLayoutClass))

typedef struct _EsdashboardFillBoxLayout			EsdashboardFillBoxLayout;
typedef struct _EsdashboardFillBoxLayoutPrivate		EsdashboardFillBoxLayoutPrivate;
typedef struct _EsdashboardFillBoxLayoutClass		EsdashboardFillBoxLayoutClass;

struct _EsdashboardFillBoxLayout
{
	/*< private >*/
	/* Parent instance */
	ClutterLayoutManager 			parent_instance;

	/* Private structure */
	EsdashboardFillBoxLayoutPrivate	*priv;
};

struct _EsdashboardFillBoxLayoutClass
{
	/*< private >*/
	/* Parent class */
	ClutterLayoutManagerClass		parent_class;
};

/* Public API */
GType esdashboard_fill_box_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* esdashboard_fill_box_layout_new(void);
ClutterLayoutManager* esdashboard_fill_box_layout_new_with_orientation(ClutterOrientation inOrientation);

ClutterOrientation esdashboard_fill_box_layout_get_orientation(EsdashboardFillBoxLayout *self);
void esdashboard_fill_box_layout_set_orientation(EsdashboardFillBoxLayout *self, ClutterOrientation inOrientation);

gfloat esdashboard_fill_box_layout_get_spacing(EsdashboardFillBoxLayout *self);
void esdashboard_fill_box_layout_set_spacing(EsdashboardFillBoxLayout *self, gfloat inSpacing);

gboolean esdashboard_fill_box_layout_get_homogeneous(EsdashboardFillBoxLayout *self);
void esdashboard_fill_box_layout_set_homogeneous(EsdashboardFillBoxLayout *self, gboolean inIsHomogeneous);

gboolean esdashboard_fill_box_layout_get_keep_aspect(EsdashboardFillBoxLayout *self);
void esdashboard_fill_box_layout_set_keep_aspect(EsdashboardFillBoxLayout *self, gboolean inKeepAspect);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_FILL_BOX_LAYOUT__ */
