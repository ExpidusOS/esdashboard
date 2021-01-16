/*
 * stage: Global stage of application
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

#ifndef __LIBESDASHBOARD_STAGE__
#define __LIBESDASHBOARD_STAGE__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/types.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_STAGE				(esdashboard_stage_get_type())
#define ESDASHBOARD_STAGE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_STAGE, EsdashboardStage))
#define ESDASHBOARD_IS_STAGE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_STAGE))
#define ESDASHBOARD_STAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_STAGE, EsdashboardStageClass))
#define ESDASHBOARD_IS_STAGE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_STAGE))
#define ESDASHBOARD_STAGE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_STAGE, EsdashboardStageClass))

typedef struct _EsdashboardStage			EsdashboardStage;
typedef struct _EsdashboardStageClass		EsdashboardStageClass;
typedef struct _EsdashboardStagePrivate		EsdashboardStagePrivate;

struct _EsdashboardStage
{
	/*< private >*/
	/* Parent instance */
	ClutterStage							parent_instance;

	/* Private structure */
	EsdashboardStagePrivate					*priv;
};

struct _EsdashboardStageClass
{
	/*< private >*/
	/* Parent class */
	ClutterStageClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*actor_created)(EsdashboardStage *self, ClutterActor *inActor);

	void (*search_started)(EsdashboardStage *self);
	void (*search_changed)(EsdashboardStage *self, gchar *inText);
	void (*search_ended)(EsdashboardStage *self);

	void (*show_tooltip)(EsdashboardStage *self, ClutterAction *inAction);
	void (*hide_tooltip)(EsdashboardStage *self, ClutterAction *inAction);
};

/* Public API */
GType esdashboard_stage_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_stage_new(void);

EsdashboardStageBackgroundImageType esdashboard_stage_get_background_image_type(EsdashboardStage *self);
void esdashboard_stage_set_background_image_type(EsdashboardStage *self, EsdashboardStageBackgroundImageType inType);

ClutterColor* esdashboard_stage_get_background_color(EsdashboardStage *self);
void esdashboard_stage_set_background_color(EsdashboardStage *self, const ClutterColor *inColor);

const gchar* esdashboard_stage_get_switch_to_view(EsdashboardStage *self);
void esdashboard_stage_set_switch_to_view(EsdashboardStage *self, const gchar *inViewInternalName);

void esdashboard_stage_show_notification(EsdashboardStage *self, const gchar *inIconName, const gchar *inText);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_STAGE__ */
