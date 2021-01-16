/*
 * drop-action: Drop action for drop targets
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

#ifndef __LIBESDASHBOARD_DROP_ACTION__
#define __LIBESDASHBOARD_DROP_ACTION__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/drag-action.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_DROP_ACTION				(esdashboard_drop_action_get_type())
#define ESDASHBOARD_DROP_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_DROP_ACTION, EsdashboardDropAction))
#define ESDASHBOARD_IS_DROP_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_DROP_ACTION))
#define ESDASHBOARD_DROP_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_DROP_ACTION, EsdashboardDropActionClass))
#define ESDASHBOARD_IS_DROP_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_DROP_ACTION))
#define ESDASHBOARD_DROP_ACTION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_DROP_ACTION, EsdashboardDropActionClass))

typedef struct _EsdashboardDropAction				EsdashboardDropAction;
typedef struct _EsdashboardDropActionClass			EsdashboardDropActionClass;
typedef struct _EsdashboardDropActionPrivate		EsdashboardDropActionPrivate;

struct _EsdashboardDropAction
{
	/*< private >*/
	/* Parent instance */
	ClutterAction					parent_instance;

	/* Private structure */
	EsdashboardDropActionPrivate	*priv;
};

struct _EsdashboardDropActionClass
{
	/*< private >*/
	/* Parent class */
	ClutterActionClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	gboolean (*begin)(EsdashboardDropAction *self, EsdashboardDragAction *inDragAction);
	gboolean (*can_drop)(EsdashboardDropAction *self, EsdashboardDragAction *inDragAction, gfloat inX, gfloat inY);
	void (*drop)(EsdashboardDropAction *self, EsdashboardDragAction *inDragAction, gfloat inX, gfloat inY);
	void (*end)(EsdashboardDropAction *self, EsdashboardDragAction *inDragAction);

	void (*drag_enter)(EsdashboardDropAction *self, EsdashboardDragAction *inDragAction);
	void (*drag_motion)(EsdashboardDropAction *self, EsdashboardDragAction *inDragAction, gfloat inX, gfloat inY);
	void (*drag_leave)(EsdashboardDropAction *self, EsdashboardDragAction *inDragAction);
};

/* Public API */
GType esdashboard_drop_action_get_type(void) G_GNUC_CONST;

ClutterAction* esdashboard_drop_action_new();

GSList* esdashboard_drop_action_get_targets(void);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_DROP_ACTION__ */
