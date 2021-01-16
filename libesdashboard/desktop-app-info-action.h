/*
 * desktop-app-info-action: An application action defined at desktop entry
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

#ifndef __LIBESDASHBOARD_DESKTOP_APP_INFO_ACTION__
#define __LIBESDASHBOARD_DESKTOP_APP_INFO_ACTION__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <markon/markon.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION			(esdashboard_desktop_app_info_action_get_type())
#define ESDASHBOARD_DESKTOP_APP_INFO_ACTION(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION, EsdashboardDesktopAppInfoAction))
#define ESDASHBOARD_IS_DESKTOP_APP_INFO_ACTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION))
#define ESDASHBOARD_DESKTOP_APP_INFO_ACTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION, EsdashboardDesktopAppInfoActionClass))
#define ESDASHBOARD_IS_DESKTOP_APP_INFO_ACTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION))
#define ESDASHBOARD_DESKTOP_APP_INFO_ACTION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION, EsdashboardDesktopAppInfoActionClass))

typedef struct _EsdashboardDesktopAppInfoAction				EsdashboardDesktopAppInfoAction;
typedef struct _EsdashboardDesktopAppInfoActionClass		EsdashboardDesktopAppInfoActionClass;
typedef struct _EsdashboardDesktopAppInfoActionPrivate		EsdashboardDesktopAppInfoActionPrivate;

struct _EsdashboardDesktopAppInfoAction
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	EsdashboardDesktopAppInfoActionPrivate	*priv;
};

struct _EsdashboardDesktopAppInfoActionClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*activate)(EsdashboardDesktopAppInfoAction *self);
};

/* Public API */
GType esdashboard_desktop_app_info_action_get_type(void) G_GNUC_CONST;

const gchar* esdashboard_desktop_app_info_action_get_name(EsdashboardDesktopAppInfoAction *self);
void esdashboard_desktop_app_info_action_set_name(EsdashboardDesktopAppInfoAction *self,
													const gchar *inName);

const gchar* esdashboard_desktop_app_info_action_get_icon_name(EsdashboardDesktopAppInfoAction *self);
void esdashboard_desktop_app_info_action_set_icon_name(EsdashboardDesktopAppInfoAction *self,
														const gchar *inIconName);

const gchar* esdashboard_desktop_app_info_action_get_command(EsdashboardDesktopAppInfoAction *self);
void esdashboard_desktop_app_info_action_set_command(EsdashboardDesktopAppInfoAction *self,
														const gchar *inCommand);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_DESKTOP_APP_INFO_ACTION__ */
