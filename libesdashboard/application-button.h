/*
 * application-button: A button representing an application
 *                     (either by menu item or desktop file)
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

#ifndef __LIBESDASHBOARD_APPLICATION_BUTTON__
#define __LIBESDASHBOARD_APPLICATION_BUTTON__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <garcon/garcon.h>

#include <libesdashboard/button.h>
#include <libesdashboard/desktop-app-info.h>
#include <libesdashboard/popup-menu.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_APPLICATION_BUTTON				(esdashboard_application_button_get_type())
#define ESDASHBOARD_APPLICATION_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_APPLICATION_BUTTON, EsdashboardApplicationButton))
#define ESDASHBOARD_IS_APPLICATION_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_APPLICATION_BUTTON))
#define ESDASHBOARD_APPLICATION_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_APPLICATION_BUTTON, EsdashboardApplicationButtonClass))
#define ESDASHBOARD_IS_APPLICATION_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_APPLICATION_BUTTON))
#define ESDASHBOARD_APPLICATION_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_APPLICATION_BUTTON, EsdashboardApplicationButtonClass))

typedef struct _EsdashboardApplicationButton			EsdashboardApplicationButton;
typedef struct _EsdashboardApplicationButtonClass		EsdashboardApplicationButtonClass;
typedef struct _EsdashboardApplicationButtonPrivate		EsdashboardApplicationButtonPrivate;

struct _EsdashboardApplicationButton
{
	/*< private >*/
	/* Parent instance */
	EsdashboardButton						parent_instance;

	/* Private structure */
	EsdashboardApplicationButtonPrivate		*priv;
};

struct _EsdashboardApplicationButtonClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardButtonClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_application_button_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_application_button_new(void);
ClutterActor* esdashboard_application_button_new_from_app_info(GAppInfo *inAppInfo);

GAppInfo* esdashboard_application_button_get_app_info(EsdashboardApplicationButton *self);
void esdashboard_application_button_set_app_info(EsdashboardApplicationButton *self,
													GAppInfo *inAppInfo);

gboolean esdashboard_application_button_get_show_description(EsdashboardApplicationButton *self);
void esdashboard_application_button_set_show_description(EsdashboardApplicationButton *self,
															gboolean inShowDescription);

const gchar* esdashboard_application_button_get_format_title_only(EsdashboardApplicationButton *self);
void esdashboard_application_button_set_format_title_only(EsdashboardApplicationButton *self,
															const gchar *inFormat);

const gchar* esdashboard_application_button_get_format_title_description(EsdashboardApplicationButton *self);
void esdashboard_application_button_set_format_title_description(EsdashboardApplicationButton *self,
																	const gchar *inFormat);

const gchar* esdashboard_application_button_get_display_name(EsdashboardApplicationButton *self);
const gchar* esdashboard_application_button_get_icon_name(EsdashboardApplicationButton *self);

gboolean esdashboard_application_button_execute(EsdashboardApplicationButton *self, GAppLaunchContext *inContext);

guint esdashboard_application_button_add_popup_menu_items_for_windows(EsdashboardApplicationButton *self,
																		EsdashboardPopupMenu *inMenu);
guint esdashboard_application_button_add_popup_menu_items_for_actions(EsdashboardApplicationButton *self,
																		EsdashboardPopupMenu *inMenu);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_APPLICATION_BUTTON__ */
