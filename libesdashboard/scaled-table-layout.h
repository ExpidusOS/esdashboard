/*
 * scaled-table-layout: Layouts children in a dynamic table grid
 *                      (rows and columns are inserted and deleted
 *                      automatically depending on the number of
 *                      child actors) and scaled to fit the allocation
 *                      of the actor holding all child actors.
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

#ifndef __LIBESDASHBOARD_SCALED_TABLE_LAYOUT__
#define __LIBESDASHBOARD_SCALED_TABLE_LAYOUT__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SCALED_TABLE_LAYOUT			(esdashboard_scaled_table_layout_get_type())
#define ESDASHBOARD_SCALED_TABLE_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SCALED_TABLE_LAYOUT, EsdashboardScaledTableLayout))
#define ESDASHBOARD_IS_SCALED_TABLE_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SCALED_TABLE_LAYOUT))
#define ESDASHBOARD_SCALED_TABLE_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SCALED_TABLE_LAYOUT, EsdashboardScaledTableLayoutClass))
#define ESDASHBOARD_IS_SCALED_TABLE_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SCALED_TABLE_LAYOUT))
#define ESDASHBOARD_SCALED_TABLE_LAYOUT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SCALED_TABLE_LAYOUT, EsdashboardScaledTableLayoutClass))

typedef struct _EsdashboardScaledTableLayout			EsdashboardScaledTableLayout;
typedef struct _EsdashboardScaledTableLayoutPrivate		EsdashboardScaledTableLayoutPrivate;
typedef struct _EsdashboardScaledTableLayoutClass		EsdashboardScaledTableLayoutClass;

struct _EsdashboardScaledTableLayout
{
	/*< private >*/
	/* Parent instance */
	ClutterLayoutManager 				parent_instance;

	/* Private structure */
	EsdashboardScaledTableLayoutPrivate	*priv;
};

struct _EsdashboardScaledTableLayoutClass
{
	/*< private >*/
	/* Parent class */
	ClutterLayoutManagerClass			parent_class;
};

/* Public API */
GType esdashboard_scaled_table_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* esdashboard_scaled_table_layout_new(void);

gint esdashboard_scaled_table_layout_get_number_children(EsdashboardScaledTableLayout *self);
gint esdashboard_scaled_table_layout_get_rows(EsdashboardScaledTableLayout *self);
gint esdashboard_scaled_table_layout_get_columns(EsdashboardScaledTableLayout *self);

gboolean esdashboard_scaled_table_layout_get_relative_scale(EsdashboardScaledTableLayout *self);
void esdashboard_scaled_table_layout_set_relative_scale(EsdashboardScaledTableLayout *self, gboolean inScaling);

gboolean esdashboard_scaled_table_layout_get_prevent_upscaling(EsdashboardScaledTableLayout *self);
void esdashboard_scaled_table_layout_set_prevent_upscaling(EsdashboardScaledTableLayout *self, gboolean inPreventUpscaling);

void esdashboard_scaled_table_layout_set_spacing(EsdashboardScaledTableLayout *self, gfloat inSpacing);

gfloat esdashboard_scaled_table_layout_get_row_spacing(EsdashboardScaledTableLayout *self);
void esdashboard_scaled_table_layout_set_row_spacing(EsdashboardScaledTableLayout *self, gfloat inSpacing);

gfloat esdashboard_scaled_table_layout_get_column_spacing(EsdashboardScaledTableLayout *self);
void esdashboard_scaled_table_layout_set_column_spacing(EsdashboardScaledTableLayout *self, gfloat inSpacing);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_SCALED_TABLE_LAYOUT__ */
