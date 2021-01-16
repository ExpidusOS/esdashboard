/*
 * live-workspace: An actor showing the content of a workspace which will
 *                 be updated if changed.
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

#ifndef __LIBESDASHBOARD_LIVE_WORKSPACE__
#define __LIBESDASHBOARD_LIVE_WORKSPACE__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/background.h>
#include <libesdashboard/window-tracker-workspace.h>
#include <libesdashboard/window-tracker-monitor.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_LIVE_WORKSPACE				(esdashboard_live_workspace_get_type())
#define ESDASHBOARD_LIVE_WORKSPACE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_LIVE_WORKSPACE, EsdashboardLiveWorkspace))
#define ESDASHBOARD_IS_LIVE_WORKSPACE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_LIVE_WORKSPACE))
#define ESDASHBOARD_LIVE_WORKSPACE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_LIVE_WORKSPACE, EsdashboardLiveWorkspaceClass))
#define ESDASHBOARD_IS_LIVE_WORKSPACE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_LIVE_WORKSPACE))
#define ESDASHBOARD_LIVE_WORKSPACE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_LIVE_WORKSPACE, EsdashboardLiveWorkspaceClass))

typedef struct _EsdashboardLiveWorkspace			EsdashboardLiveWorkspace;
typedef struct _EsdashboardLiveWorkspaceClass		EsdashboardLiveWorkspaceClass;
typedef struct _EsdashboardLiveWorkspacePrivate		EsdashboardLiveWorkspacePrivate;

struct _EsdashboardLiveWorkspace
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground			parent_instance;

	/* Private structure */
	EsdashboardLiveWorkspacePrivate	*priv;
};

struct _EsdashboardLiveWorkspaceClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(EsdashboardLiveWorkspace *self);
};

/* Public API */
GType esdashboard_live_workspace_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_live_workspace_new(void);
ClutterActor* esdashboard_live_workspace_new_for_workspace(EsdashboardWindowTrackerWorkspace *inWorkspace);

EsdashboardWindowTrackerWorkspace* esdashboard_live_workspace_get_workspace(EsdashboardLiveWorkspace *self);
void esdashboard_live_workspace_set_workspace(EsdashboardLiveWorkspace *self, EsdashboardWindowTrackerWorkspace *inWorkspace);

EsdashboardWindowTrackerMonitor* esdashboard_live_workspace_get_monitor(EsdashboardLiveWorkspace *self);
void esdashboard_live_workspace_set_monitor(EsdashboardLiveWorkspace *self, EsdashboardWindowTrackerMonitor *inMonitor);

EsdashboardStageBackgroundImageType esdashboard_live_workspace_get_background_image_type(EsdashboardLiveWorkspace *self);
void esdashboard_live_workspace_set_background_image_type(EsdashboardLiveWorkspace *self, EsdashboardStageBackgroundImageType inType);

gboolean esdashboard_live_workspace_get_show_workspace_name(EsdashboardLiveWorkspace *self);
void esdashboard_live_workspace_set_show_workspace_name(EsdashboardLiveWorkspace *self, gboolean inIsVisible);

gfloat esdashboard_live_workspace_get_workspace_name_padding(EsdashboardLiveWorkspace *self);
void esdashboard_live_workspace_set_workspace_name_padding(EsdashboardLiveWorkspace *self, gfloat inPadding);

G_END_DECLS

#endif
