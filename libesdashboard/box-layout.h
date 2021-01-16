/*
 * box-layout: A ClutterBoxLayout derived layout manager disregarding
 *             text direction and enforcing left-to-right layout in
 *             horizontal orientation
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

#ifndef __LIBESDASHBOARD_BOX_LAYOUT__
#define __LIBESDASHBOARD_BOX_LAYOUT__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_BOX_LAYOUT				(esdashboard_box_layout_get_type())
#define ESDASHBOARD_BOX_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_BOX_LAYOUT, EsdashboardBoxLayout))
#define ESDASHBOARD_IS_BOX_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_BOX_LAYOUT))
#define ESDASHBOARD_BOX_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_BOX_LAYOUT, EsdashboardBoxLayoutClass))
#define ESDASHBOARD_IS_BOX_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_BOX_LAYOUT))
#define ESDASHBOARD_BOX_LAYOUT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_BOX_LAYOUT, EsdashboardBoxLayoutClass))

typedef struct _EsdashboardBoxLayout			EsdashboardBoxLayout;
typedef struct _EsdashboardBoxLayoutClass		EsdashboardBoxLayoutClass;

/**
 * EsdashboardBoxLayout:
 *
 * The #EsdashboardBoxLayout structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardBoxLayout
{
	/*< private >*/
	/* Parent instance */
	ClutterBoxLayout 				parent_instance;
};

/**
 * EsdashboardBoxLayoutClass:
 *
 * The #EsdashboardBoxLayoutClass structure contains only private data
 */
struct _EsdashboardBoxLayoutClass
{
	/*< private >*/
	/* Parent class */
	ClutterBoxLayoutClass			parent_class;
};

/* Public API */
GType esdashboard_box_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* esdashboard_box_layout_new(void);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_BOX_LAYOUT__ */
