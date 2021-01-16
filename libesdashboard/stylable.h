/*
 * stylable: An interface which can be inherited by actor and objects
 *           to get styled by a theme
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

#ifndef __LIBESDASHBOARD_STYLABLE__
#define __LIBESDASHBOARD_STYLABLE__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_STYLABLE				(esdashboard_stylable_get_type())
#define ESDASHBOARD_STYLABLE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_STYLABLE, EsdashboardStylable))
#define ESDASHBOARD_IS_STYLABLE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_STYLABLE))
#define ESDASHBOARD_STYLABLE_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj), ESDASHBOARD_TYPE_STYLABLE, EsdashboardStylableInterface))

typedef struct _EsdashboardStylable				EsdashboardStylable;
typedef struct _EsdashboardStylableInterface	EsdashboardStylableInterface;

struct _EsdashboardStylableInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface				parent_interface;

	/*< public >*/
	/* Virtual functions */
	void (*get_stylable_properties)(EsdashboardStylable *self, GHashTable *ioStylableProperties);

	const gchar* (*get_name)(EsdashboardStylable *self);

	EsdashboardStylable* (*get_parent)(EsdashboardStylable *self);

	const gchar* (*get_classes)(EsdashboardStylable *self);
	void (*set_classes)(EsdashboardStylable *self, const gchar *inClasses);
	void (*class_added)(EsdashboardStylable *self, const gchar *inClass);
	void (*class_removed)(EsdashboardStylable *self, const gchar *inClass);

	const gchar* (*get_pseudo_classes)(EsdashboardStylable *self);
	void (*set_pseudo_classes)(EsdashboardStylable *self, const gchar *inClasses);
	void (*pseudo_class_added)(EsdashboardStylable *self, const gchar *inClass);
	void (*pseudo_class_removed)(EsdashboardStylable *self, const gchar *inClass);

	void (*invalidate)(EsdashboardStylable *self);
};

/* Public API */
GType esdashboard_stylable_get_type(void) G_GNUC_CONST;

GHashTable* esdashboard_stylable_get_stylable_properties(EsdashboardStylable *self);
gboolean esdashboard_stylable_add_stylable_property(EsdashboardStylable *self,
													GHashTable *ioStylableProperties,
													const gchar *inProperty);

const gchar* esdashboard_stylable_get_name(EsdashboardStylable *self);

EsdashboardStylable* esdashboard_stylable_get_parent(EsdashboardStylable *self);

const gchar* esdashboard_stylable_get_classes(EsdashboardStylable *self);
void esdashboard_stylable_set_classes(EsdashboardStylable *self, const gchar *inClasses);
gboolean esdashboard_stylable_has_class(EsdashboardStylable *self, const gchar *inClass);
void esdashboard_stylable_add_class(EsdashboardStylable *self, const gchar *inClass);
void esdashboard_stylable_remove_class(EsdashboardStylable *self, const gchar *inClass);

const gchar* esdashboard_stylable_get_pseudo_classes(EsdashboardStylable *self);
void esdashboard_stylable_set_pseudo_classes(EsdashboardStylable *self, const gchar *inClasses);
gboolean esdashboard_stylable_has_pseudo_class(EsdashboardStylable *self, const gchar *inClass);
void esdashboard_stylable_add_pseudo_class(EsdashboardStylable *self, const gchar *inClass);
void esdashboard_stylable_remove_pseudo_class(EsdashboardStylable *self, const gchar *inClass);

void esdashboard_stylable_invalidate(EsdashboardStylable *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_STYLABLE__ */
