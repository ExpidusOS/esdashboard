/*
 * clock-view: A view showing a clock
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

#ifndef __ESDASHBOARD_CLOCK_VIEW__
#define __ESDASHBOARD_CLOCK_VIEW__

#include <libesdashboard/libesdashboard.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_CLOCK_VIEW				(esdashboard_clock_view_get_type())
#define ESDASHBOARD_CLOCK_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_CLOCK_VIEW, EsdashboardClockView))
#define ESDASHBOARD_IS_CLOCK_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_CLOCK_VIEW))
#define ESDASHBOARD_CLOCK_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_CLOCK_VIEW, EsdashboardClockViewClass))
#define ESDASHBOARD_IS_CLOCK_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_CLOCK_VIEW))
#define ESDASHBOARD_CLOCK_VIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_CLOCK_VIEW, EsdashboardClockViewClass))

typedef struct _EsdashboardClockView			EsdashboardClockView; 
typedef struct _EsdashboardClockViewPrivate		EsdashboardClockViewPrivate;
typedef struct _EsdashboardClockViewClass		EsdashboardClockViewClass;

struct _EsdashboardClockView
{
	/* Parent instance */
	EsdashboardView						parent_instance;

	/* Private structure */
	EsdashboardClockViewPrivate			*priv;
};

struct _EsdashboardClockViewClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardViewClass				parent_class;
};

/* Public API */
GType esdashboard_clock_view_get_type(void) G_GNUC_CONST;

ESDASHBOARD_DECLARE_PLUGIN_TYPE(esdashboard_clock_view);

G_END_DECLS

#endif
