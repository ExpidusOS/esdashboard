/*
 * dynamic-table-layout: Layouts children in a dynamic table grid
 *                       (rows and columns are inserted and deleted
 *                       automatically depending on the number of
 *                       child actors).
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

#ifndef __LIBESDASHBOARD_DYNAMIC_TABLE_LAYOUT__
#define __LIBESDASHBOARD_DYNAMIC_TABLE_LAYOUT__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT				(esdashboard_dynamic_table_layout_get_type())
#define ESDASHBOARD_DYNAMIC_TABLE_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT, EsdashboardDynamicTableLayout))
#define ESDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT))
#define ESDASHBOARD_DYNAMIC_TABLE_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT, EsdashboardDynamicTableLayoutClass))
#define ESDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT))
#define ESDASHBOARD_DYNAMIC_TABLE_LAYOUT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT, EsdashboardDynamicTableLayoutClass))

typedef struct _EsdashboardDynamicTableLayout				EsdashboardDynamicTableLayout;
typedef struct _EsdashboardDynamicTableLayoutPrivate		EsdashboardDynamicTableLayoutPrivate;
typedef struct _EsdashboardDynamicTableLayoutClass			EsdashboardDynamicTableLayoutClass;

struct _EsdashboardDynamicTableLayout
{
	/*< private >*/
	/* Parent instance */
	ClutterLayoutManager 					parent_instance;

	/* Private structure */
	EsdashboardDynamicTableLayoutPrivate	*priv;
};

struct _EsdashboardDynamicTableLayoutClass
{
	/*< private >*/
	/* Parent class */
	ClutterLayoutManagerClass				parent_class;
};

/* Public API */
GType esdashboard_dynamic_table_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* esdashboard_dynamic_table_layout_new(void);

gint esdashboard_dynamic_table_layout_get_number_children(EsdashboardDynamicTableLayout *self);
gint esdashboard_dynamic_table_layout_get_rows(EsdashboardDynamicTableLayout *self);
gint esdashboard_dynamic_table_layout_get_columns(EsdashboardDynamicTableLayout *self);

void esdashboard_dynamic_table_layout_set_spacing(EsdashboardDynamicTableLayout *self, gfloat inSpacing);

gfloat esdashboard_dynamic_table_layout_get_row_spacing(EsdashboardDynamicTableLayout *self);
void esdashboard_dynamic_table_layout_set_row_spacing(EsdashboardDynamicTableLayout *self, gfloat inSpacing);

gfloat esdashboard_dynamic_table_layout_get_column_spacing(EsdashboardDynamicTableLayout *self);
void esdashboard_dynamic_table_layout_set_column_spacing(EsdashboardDynamicTableLayout *self, gfloat inSpacing);

gint esdashboard_dynamic_table_layout_get_fixed_columns(EsdashboardDynamicTableLayout *self);
void esdashboard_dynamic_table_layout_set_fixed_columns(EsdashboardDynamicTableLayout *self, gint inColumns);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_DYNAMIC_TABLE_LAYOUT__ */
