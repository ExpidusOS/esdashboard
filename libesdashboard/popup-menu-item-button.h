/*
 * popup-menu-item-button: A button pop-up menu item
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

#ifndef __LIBESDASHBOARD_POPUP_MENU_ITEM_BUTTON__
#define __LIBESDASHBOARD_POPUP_MENU_ITEM_BUTTON__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/label.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON				(esdashboard_popup_menu_item_button_get_type())
#define ESDASHBOARD_POPUP_MENU_ITEM_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON, EsdashboardPopupMenuItemButton))
#define ESDASHBOARD_IS_POPUP_MENU_ITEM_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON))
#define ESDASHBOARD_POPUP_MENU_ITEM_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON, EsdashboardPopupMenuItemButtonClass))
#define ESDASHBOARD_IS_POPUP_MENU_ITEM_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON))
#define ESDASHBOARD_POPUP_MENU_ITEM_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON, EsdashboardPopupMenuItemButtonClass))

typedef struct _EsdashboardPopupMenuItemButton				EsdashboardPopupMenuItemButton;
typedef struct _EsdashboardPopupMenuItemButtonClass			EsdashboardPopupMenuItemButtonClass;
typedef struct _EsdashboardPopupMenuItemButtonPrivate		EsdashboardPopupMenuItemButtonPrivate;

/**
 * EsdashboardPopupMenuItemButton:
 *
 * The #EsdashboardPopupMenuItemButton structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardPopupMenuItemButton
{
	/*< private >*/
	/* Parent instance */
	EsdashboardLabel							parent_instance;

	/* Private structure */
	EsdashboardPopupMenuItemButtonPrivate		*priv;
};

/**
 * EsdashboardPopupMenuItemButtonClass:
 *
 * The #EsdashboardPopupMenuItemButtonClass structure contains only private data
 */
struct _EsdashboardPopupMenuItemButtonClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardLabelClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_popup_menu_item_button_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_popup_menu_item_button_new(void);
ClutterActor* esdashboard_popup_menu_item_button_new_with_text(const gchar *inText);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_POPUP_MENU_ITEM_BUTTON__ */
