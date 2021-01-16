/*
 * stage-interface: A top-level actor for a monitor at stage
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

#ifndef __LIBESDASHBOARD_STAGE_INTERFACE__
#define __LIBESDASHBOARD_STAGE_INTERFACE__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/stage.h>
#include <libesdashboard/types.h>
#include <libesdashboard/window-tracker-monitor.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_STAGE_INTERFACE			(esdashboard_stage_interface_get_type())
#define ESDASHBOARD_STAGE_INTERFACE(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_STAGE_INTERFACE, EsdashboardStageInterface))
#define ESDASHBOARD_IS_STAGE_INTERFACE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_STAGE_INTERFACE))
#define ESDASHBOARD_STAGE_INTERFACE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_STAGE_INTERFACE, EsdashboardStageInterfaceClass))
#define ESDASHBOARD_IS_STAGE_INTERFACE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_STAGE_INTERFACE))
#define ESDASHBOARD_STAGE_INTERFACE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_STAGE_INTERFACE, EsdashboardStageInterfaceClass))

typedef struct _EsdashboardStageInterface			EsdashboardStageInterface;
typedef struct _EsdashboardStageInterfaceClass		EsdashboardStageInterfaceClass;
typedef struct _EsdashboardStageInterfacePrivate	EsdashboardStageInterfacePrivate;

struct _EsdashboardStageInterface
{
	/*< private >*/
	/* Parent instance */
	EsdashboardStage						parent_instance;

	/* Private structure */
	EsdashboardStageInterfacePrivate		*priv;
};

struct _EsdashboardStageInterfaceClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardStageClass					parent_class;
};

/* Public API */
GType esdashboard_stage_interface_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_stage_interface_new(void);

EsdashboardWindowTrackerMonitor* esdashboard_stage_interface_get_monitor(EsdashboardStageInterface *self);
void esdashboard_stage_interface_set_monitor(EsdashboardStageInterface *self, EsdashboardWindowTrackerMonitor *inMonitor);

EsdashboardStageBackgroundImageType esdashboard_stage_interface_get_background_image_type(EsdashboardStageInterface *self);
void esdashboard_stage_interface_set_background_image_type(EsdashboardStageInterface *self, EsdashboardStageBackgroundImageType inType);

ClutterColor* esdashboard_stage_interface_get_background_color(EsdashboardStageInterface *self);
void esdashboard_stage_interface_set_background_color(EsdashboardStageInterface *self, const ClutterColor *inColor);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_STAGE_INTERFACE__ */
