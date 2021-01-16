/*
 * hot-corner: Activates application when pointer is move to a corner
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

#ifndef __ESDASHBOARD_HOT_CORNER__
#define __ESDASHBOARD_HOT_CORNER__

#include <libesdashboard/libesdashboard.h>

G_BEGIN_DECLS

/* Public definitions */
typedef enum /*< prefix=ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER >*/
{
	ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_LEFT=0,
	ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_RIGHT,
	ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_LEFT,
	ESDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_RIGHT,
} EsdashboardHotCornerActivationCorner;

GType esdashboard_hot_corner_activation_corner_get_type(void) G_GNUC_CONST;
#define ESDASHBOARD_TYPE_HOT_CORNER_ACTIVATION_CORNER	(esdashboard_hot_corner_activation_corner_get_type())


/* Object declaration */
#define ESDASHBOARD_TYPE_HOT_CORNER				(esdashboard_hot_corner_get_type())
#define ESDASHBOARD_HOT_CORNER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_HOT_CORNER, EsdashboardHotCorner))
#define ESDASHBOARD_IS_HOT_CORNER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_HOT_CORNER))
#define ESDASHBOARD_HOT_CORNER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_HOT_CORNER, EsdashboardHotCornerClass))
#define ESDASHBOARD_IS_HOT_CORNER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_HOT_CORNER))
#define ESDASHBOARD_HOT_CORNER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_HOT_CORNER, EsdashboardHotCornerClass))

typedef struct _EsdashboardHotCorner			EsdashboardHotCorner; 
typedef struct _EsdashboardHotCornerPrivate		EsdashboardHotCornerPrivate;
typedef struct _EsdashboardHotCornerClass		EsdashboardHotCornerClass;

struct _EsdashboardHotCorner
{
	/* Parent instance */
	GObject						parent_instance;

	/* Private structure */
	EsdashboardHotCornerPrivate	*priv;
};

struct _EsdashboardHotCornerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass				parent_class;
};

/* Public API */
GType esdashboard_hot_corner_get_type(void) G_GNUC_CONST;

ESDASHBOARD_DECLARE_PLUGIN_TYPE(esdashboard_hot_corner);

EsdashboardHotCorner* esdashboard_hot_corner_new(void);

G_END_DECLS

#endif
