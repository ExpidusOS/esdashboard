/*
 * desktop-app-info: A GDesktopAppInfo like object for markon menu
 *                   items implementing and supporting GAppInfo
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

#ifndef __LIBESDASHBOARD_DESKTOP_APP_INFO__
#define __LIBESDASHBOARD_DESKTOP_APP_INFO__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/desktop-app-info-action.h>
#include <markon/markon.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_DESKTOP_APP_INFO				(esdashboard_desktop_app_info_get_type())
#define ESDASHBOARD_DESKTOP_APP_INFO(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_DESKTOP_APP_INFO, EsdashboardDesktopAppInfo))
#define ESDASHBOARD_IS_DESKTOP_APP_INFO(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_DESKTOP_APP_INFO))
#define ESDASHBOARD_DESKTOP_APP_INFO_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_DESKTOP_APP_INFO, EsdashboardDesktopAppInfoClass))
#define ESDASHBOARD_IS_DESKTOP_APP_INFO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_DESKTOP_APP_INFO))
#define ESDASHBOARD_DESKTOP_APP_INFO_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_DESKTOP_APP_INFO, EsdashboardDesktopAppInfoClass))

typedef struct _EsdashboardDesktopAppInfo				EsdashboardDesktopAppInfo;
typedef struct _EsdashboardDesktopAppInfoClass			EsdashboardDesktopAppInfoClass;
typedef struct _EsdashboardDesktopAppInfoPrivate		EsdashboardDesktopAppInfoPrivate;

struct _EsdashboardDesktopAppInfo
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardDesktopAppInfoPrivate	*priv;
};

struct _EsdashboardDesktopAppInfoClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*changed)(EsdashboardDesktopAppInfo *self);
};

/* Public API */
GType esdashboard_desktop_app_info_get_type(void) G_GNUC_CONST;

GAppInfo* esdashboard_desktop_app_info_new_from_desktop_id(const gchar *inDesktopID);
GAppInfo* esdashboard_desktop_app_info_new_from_path(const gchar *inPath);
GAppInfo* esdashboard_desktop_app_info_new_from_file(GFile *inFile);
GAppInfo* esdashboard_desktop_app_info_new_from_menu_item(MarkonMenuItem *inMenuItem);

gboolean esdashboard_desktop_app_info_is_valid(EsdashboardDesktopAppInfo *self);

GFile* esdashboard_desktop_app_info_get_file(EsdashboardDesktopAppInfo *self);
gboolean esdashboard_desktop_app_info_reload(EsdashboardDesktopAppInfo *self);

GList* esdashboard_desktop_app_info_get_actions(EsdashboardDesktopAppInfo *self);
gboolean esdashboard_desktop_app_info_launch_action(EsdashboardDesktopAppInfo *self,
													EsdashboardDesktopAppInfoAction *inAction,
													GAppLaunchContext *inContext,
													GError **outError);
gboolean esdashboard_desktop_app_info_launch_action_by_name(EsdashboardDesktopAppInfo *self,
															const gchar *inActionName,
															GAppLaunchContext *inContext,
															GError **outError);

GList* esdashboard_desktop_app_info_get_keywords(EsdashboardDesktopAppInfo *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_DESKTOP_APP_INFO__ */
