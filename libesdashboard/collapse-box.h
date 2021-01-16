/*
 * collapse-box: A collapsable container for one actor
 *               with capability to expand
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

#ifndef __LIBESDASHBOARD_COLLAPSE_BOX__
#define __LIBESDASHBOARD_COLLAPSE_BOX__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/actor.h>
#include <libesdashboard/types.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_COLLAPSE_BOX				(esdashboard_collapse_box_get_type())
#define ESDASHBOARD_COLLAPSE_BOX(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_COLLAPSE_BOX, EsdashboardCollapseBox))
#define ESDASHBOARD_IS_COLLAPSE_BOX(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_COLLAPSE_BOX))
#define ESDASHBOARD_COLLAPSE_BOX_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_COLLAPSE_BOX, EsdashboardCollapseBoxClass))
#define ESDASHBOARD_IS_COLLAPSE_BOX_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_COLLAPSE_BOX))
#define ESDASHBOARD_COLLAPSE_BOX_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_COLLAPSE_BOX, EsdashboardCollapseBoxClass))

typedef struct _EsdashboardCollapseBox				EsdashboardCollapseBox; 
typedef struct _EsdashboardCollapseBoxPrivate		EsdashboardCollapseBoxPrivate;
typedef struct _EsdashboardCollapseBoxClass			EsdashboardCollapseBoxClass;

struct _EsdashboardCollapseBox
{
	/*< private >*/
	/* Parent instance */
	EsdashboardActor				parent_instance;

	/* Private structure */
	EsdashboardCollapseBoxPrivate	*priv;
};

struct _EsdashboardCollapseBoxClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardActorClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*collapse_changed)(EsdashboardCollapseBox *self, gboolean isCollapsed);
};

/* Public API */
GType esdashboard_collapse_box_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_collapse_box_new(void);

gboolean esdashboard_collapse_box_get_collapsed(EsdashboardCollapseBox *self);
void esdashboard_collapse_box_set_collapsed(EsdashboardCollapseBox *self, gboolean inCollapsed);

gfloat esdashboard_collapse_box_get_collapsed_size(EsdashboardCollapseBox *self);
void esdashboard_collapse_box_set_collapsed_size(EsdashboardCollapseBox *self, gfloat inCollapsedSize);

EsdashboardOrientation esdashboard_collapse_box_get_collapse_orientation(EsdashboardCollapseBox *self);
void esdashboard_collapse_box_set_collapse_orientation(EsdashboardCollapseBox *self, EsdashboardOrientation inOrientation);

gfloat esdashboard_collapse_box_get_collapse_progress(EsdashboardCollapseBox *self);
void esdashboard_collapse_box_set_collapse_progress(EsdashboardCollapseBox *self, gfloat inProgress);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_COLLAPSE_BOX__ */
