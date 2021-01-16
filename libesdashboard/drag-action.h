/*
 * drag-action: Drag action for actors
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

#ifndef __LIBESDASHBOARD_DRAG_ACTION__
#define __LIBESDASHBOARD_DRAG_ACTION__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_DRAG_ACTION				(esdashboard_drag_action_get_type())
#define ESDASHBOARD_DRAG_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_DRAG_ACTION, EsdashboardDragAction))
#define ESDASHBOARD_IS_DRAG_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_DRAG_ACTION))
#define ESDASHBOARD_DRAG_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_DRAG_ACTION, EsdashboardDragActionClass))
#define ESDASHBOARD_IS_DRAG_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_DRAG_ACTION))
#define ESDASHBOARD_DRAG_ACTION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_DRAG_ACTION, EsdashboardDragActionClass))

typedef struct _EsdashboardDragAction				EsdashboardDragAction;
typedef struct _EsdashboardDragActionClass			EsdashboardDragActionClass;
typedef struct _EsdashboardDragActionPrivate		EsdashboardDragActionPrivate;

struct _EsdashboardDragAction
{
	/*< private >*/
	/* Parent instance */
	ClutterDragAction				parent_instance;

	/* Private structure */
	EsdashboardDragActionPrivate	*priv;
};

struct _EsdashboardDragActionClass
{
	/*< private >*/
	/* Parent class */
	ClutterDragActionClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*drag_cancel)(EsdashboardDragAction *self, ClutterActor *inActor, gfloat inX, gfloat inY);
};

/* Public API */
GType esdashboard_drag_action_get_type(void) G_GNUC_CONST;

ClutterAction* esdashboard_drag_action_new();
ClutterAction* esdashboard_drag_action_new_with_source(ClutterActor *inSource);

ClutterActor* esdashboard_drag_action_get_source(EsdashboardDragAction *self);
ClutterActor* esdashboard_drag_action_get_actor(EsdashboardDragAction *self);

void esdashboard_drag_action_get_motion_delta(EsdashboardDragAction *self, gfloat *outDeltaX, gfloat *outDeltaY);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_DRAG_ACTION__ */
