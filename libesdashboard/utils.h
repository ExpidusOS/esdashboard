/*
 * utils: Common functions, helpers and definitions
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

#ifndef __LIBESDASHBOARD_UTILS__
#define __LIBESDASHBOARD_UTILS__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>
#include <gio/gio.h>

#include <libesdashboard/window-tracker-workspace.h>
#include <libesdashboard/stage-interface.h>
#include <libesdashboard/stage.h>
#include <libesdashboard/css-selector.h>

G_BEGIN_DECLS

/* Debug macros */
#define _DEBUG_OBJECT_NAME(x) \
	((x)!=NULL ? G_OBJECT_TYPE_NAME((x)) : "<nil>")

#define _DEBUG_BOX(msg, box) \
	g_message("%s: %s: x1=%.2f, y1=%.2f, x2=%.2f, y2=%.2f [%.2fx%.2f]", \
				__func__, \
				msg, \
				(box).x1, (box).y1, (box).x2, (box).y2, \
				(box).x2-(box).x1, (box).y2-(box).y1)

#define _DEBUG_NOTIFY(self, property, format, ...) \
	g_message("%s: Property '%s' of %p (%s) changed to "format, \
				__func__, \
				property, \
				(self), (self) ? G_OBJECT_TYPE_NAME((self)) : "<nil>", \
				## __VA_ARGS__)

/* Gobject type of pointer array (GPtrArray) */
#define ESDASHBOARD_TYPE_POINTER_ARRAY		(esdashboard_pointer_array_get_type())

/* Public API */

/**
 * GTYPE_TO_POINTER:
 * @gtype: A #GType to stuff into the pointer
 *
 * Stuffs the #GType specified at @gtype into a pointer type.
 */
#define GTYPE_TO_POINTER(gtype) \
	(GSIZE_TO_POINTER(gtype))

/**
 * GPOINTER_TO_GTYPE:
 * @pointer: A pointer to extract a #GType from
 *
 * Extracts a #GType from a pointer. The #GType must have been stored in the pointer with GTYPE_TO_POINTER().
 */
#define GPOINTER_TO_GTYPE(pointer) \
	((GType)GPOINTER_TO_SIZE(pointer))

GType esdashboard_pointer_array_get_type(void);

void esdashboard_notify(ClutterActor *inSender,
							const gchar *inIconName,
							const gchar *inFormat, ...)
							G_GNUC_PRINTF(3, 4);

GAppLaunchContext* esdashboard_create_app_context(EsdashboardWindowTrackerWorkspace *inWorkspace);

void esdashboard_register_gvalue_transformation_funcs(void);

ClutterActor* esdashboard_find_actor_by_name(ClutterActor *inActor, const gchar *inName);

/**
 * EsdashboardTraversalCallback:
 * @inActor: The actor currently processed and has matched the selector in traversal
 * @inUserData: Data passed to the function, set with esdashboard_traverse_actor()
 *
 * A callback called each time an actor matches the provided css selector
 * in esdashboard_traverse_actor().
 *
 * Returns: %FALSE if the traversal should be stopped. #ESDASHBOARD_TRAVERSAL_STOP
 * and #ESDASHBOARD_TRAVERSAL_CONTINUE are more memorable names for the return value.
 */
typedef gboolean (*EsdashboardTraversalCallback)(ClutterActor *inActor, gpointer inUserData);

/**
 * ESDASHBOARD_TRAVERSAL_STOP:
 *
 * Use this macro as the return value of a #EsdashboardTraversalCallback to stop
 * further traversal in esdashboard_traverse_actor().
 */
#define ESDASHBOARD_TRAVERSAL_STOP			(FALSE)

/**
 * ESDASHBOARD_TRAVERSAL_CONTINUE:
 *
 * Use this macro as the return value of a #EsdashboardTraversalCallback to continue
 * further traversal in esdashboard_traverse_actor().
 */
#define ESDASHBOARD_TRAVERSAL_CONTINUE		(TRUE)

void esdashboard_traverse_actor(ClutterActor *inRootActor,
								EsdashboardCssSelector *inSelector,
								EsdashboardTraversalCallback inCallback,
								gpointer inUserData);

EsdashboardStageInterface* esdashboard_get_stage_of_actor(ClutterActor *inActor);

gchar** esdashboard_split_string(const gchar *inString, const gchar *inDelimiters);

gboolean esdashboard_is_valid_id(const gchar *inString);

gchar* esdashboard_get_enum_value_name(GType inEnumClass, gint inValue);
gint esdashboard_get_enum_value_from_nickname(GType inEnumClass, const gchar *inNickname);

void esdashboard_dump_actor(ClutterActor *inActor);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_UTILS__ */
