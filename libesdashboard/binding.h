/*
 * binding: A keyboard or pointer binding
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

#ifndef __LIBESDASHBOARD_BINDING__
#define __LIBESDASHBOARD_BINDING__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_BINDING				(esdashboard_binding_get_type())
#define ESDASHBOARD_BINDING(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_BINDING, EsdashboardBinding))
#define ESDASHBOARD_IS_BINDING(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_BINDING))
#define ESDASHBOARD_BINDING_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_BINDING, EsdashboardBindingClass))
#define ESDASHBOARD_IS_BINDING_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_BINDING))
#define ESDASHBOARD_BINDING_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_BINDING, EsdashboardBindingClass))

typedef struct _EsdashboardBinding				EsdashboardBinding;
typedef struct _EsdashboardBindingClass			EsdashboardBindingClass;
typedef struct _EsdashboardBindingPrivate		EsdashboardBindingPrivate;

struct _EsdashboardBinding
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	EsdashboardBindingPrivate		*priv;
};

struct _EsdashboardBindingClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;
};

/* Public API */
#define ESDASHBOARD_BINDING_MODIFIERS_MASK		(CLUTTER_SHIFT_MASK | \
													CLUTTER_CONTROL_MASK | \
													CLUTTER_MOD1_MASK | \
													CLUTTER_MOD2_MASK | \
													CLUTTER_MOD3_MASK | \
													CLUTTER_MOD4_MASK | \
													CLUTTER_MOD5_MASK | \
													CLUTTER_SUPER_MASK | \
													CLUTTER_HYPER_MASK | \
													CLUTTER_META_MASK)

typedef enum /*< flags,prefix=ESDASHBOARD_BINDING_FLAGS >*/
{
	ESDASHBOARD_BINDING_FLAGS_NONE=0,
	ESDASHBOARD_BINDING_FLAGS_ALLOW_UNFOCUSABLE_TARGET=1 << 0,
} EsdashboardBindingFlags;

GType esdashboard_binding_get_type(void) G_GNUC_CONST;

EsdashboardBinding* esdashboard_binding_new(void);
EsdashboardBinding* esdashboard_binding_new_for_event(const ClutterEvent *inEvent);

guint esdashboard_binding_hash(gconstpointer inValue);
gboolean esdashboard_binding_compare(gconstpointer inLeft, gconstpointer inRight);

ClutterEventType esdashboard_binding_get_event_type(const EsdashboardBinding *self);
void esdashboard_binding_set_event_type(EsdashboardBinding *self, ClutterEventType inType);

const gchar* esdashboard_binding_get_class_name(const EsdashboardBinding *self);
void esdashboard_binding_set_class_name(EsdashboardBinding *self, const gchar *inClassName);

guint esdashboard_binding_get_key(const EsdashboardBinding *self);
void esdashboard_binding_set_key(EsdashboardBinding *self, guint inKey);

ClutterModifierType esdashboard_binding_get_modifiers(const EsdashboardBinding *self);
void esdashboard_binding_set_modifiers(EsdashboardBinding *self, ClutterModifierType inModifiers);

const gchar* esdashboard_binding_get_target(const EsdashboardBinding *self);
void esdashboard_binding_set_target(EsdashboardBinding *self, const gchar *inTarget);

const gchar* esdashboard_binding_get_action(const EsdashboardBinding *self);
void esdashboard_binding_set_action(EsdashboardBinding *self, const gchar *inAction);

EsdashboardBindingFlags esdashboard_binding_get_flags(const EsdashboardBinding *self);
void esdashboard_binding_set_flags(EsdashboardBinding *self, EsdashboardBindingFlags inFlags);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_BINDING__ */
