/*
 * workspace-selector: Workspace selector box
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

#ifndef __LIBESDASHBOARD_WORKSPACE_SELECTOR__
#define __LIBESDASHBOARD_WORKSPACE_SELECTOR__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/background.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_WORKSPACE_SELECTOR				(esdashboard_workspace_selector_get_type())
#define ESDASHBOARD_WORKSPACE_SELECTOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WORKSPACE_SELECTOR, EsdashboardWorkspaceSelector))
#define ESDASHBOARD_IS_WORKSPACE_SELECTOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WORKSPACE_SELECTOR))
#define ESDASHBOARD_WORKSPACE_SELECTOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_WORKSPACE_SELECTOR, EsdashboardWorkspaceSelectorClass))
#define ESDASHBOARD_IS_WORKSPACE_SELECTOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_WORKSPACE_SELECTOR))
#define ESDASHBOARD_WORKSPACE_SELECTOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_WORKSPACE_SELECTOR, EsdashboardWorkspaceSelectorClass))

typedef struct _EsdashboardWorkspaceSelector			EsdashboardWorkspaceSelector;
typedef struct _EsdashboardWorkspaceSelectorClass		EsdashboardWorkspaceSelectorClass;
typedef struct _EsdashboardWorkspaceSelectorPrivate		EsdashboardWorkspaceSelectorPrivate;

struct _EsdashboardWorkspaceSelector
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground				parent_instance;

	/* Private structure */
	EsdashboardWorkspaceSelectorPrivate	*priv;
};

struct _EsdashboardWorkspaceSelectorClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass			parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_workspace_selector_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_workspace_selector_new(void);
ClutterActor* esdashboard_workspace_selector_new_with_orientation(ClutterOrientation inOrientation);

gfloat esdashboard_workspace_selector_get_spacing(EsdashboardWorkspaceSelector *self);
void esdashboard_workspace_selector_set_spacing(EsdashboardWorkspaceSelector *self, const gfloat inSpacing);

ClutterOrientation esdashboard_workspace_selector_get_orientation(EsdashboardWorkspaceSelector *self);
void esdashboard_workspace_selector_set_orientation(EsdashboardWorkspaceSelector *self, ClutterOrientation inOrientation);

gfloat esdashboard_workspace_selector_get_maximum_size(EsdashboardWorkspaceSelector *self);
void esdashboard_workspace_selector_set_maximum_size(EsdashboardWorkspaceSelector *self, const gfloat inSize);

gfloat esdashboard_workspace_selector_get_maximum_fraction(EsdashboardWorkspaceSelector *self);
void esdashboard_workspace_selector_set_maximum_fraction(EsdashboardWorkspaceSelector *self, const gfloat inFraction);

gboolean esdashboard_workspace_selector_is_using_fraction(EsdashboardWorkspaceSelector *self);

gboolean esdashboard_workspace_selector_get_show_current_monitor_only(EsdashboardWorkspaceSelector *self);
void esdashboard_workspace_selector_set_show_current_monitor_only(EsdashboardWorkspaceSelector *self, gboolean inShowCurrentMonitorOnly);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_WORKSPACE_SELECTOR__ */
