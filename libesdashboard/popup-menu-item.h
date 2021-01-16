/*
 * popup-menu-item: An interface implemented by actors used as pop-up menu item
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

#ifndef __LIBESDASHBOARD_POPUP_MENU_ITEM__
#define __LIBESDASHBOARD_POPUP_MENU_ITEM__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_POPUP_MENU_ITEM				(esdashboard_popup_menu_item_get_type())
#define ESDASHBOARD_POPUP_MENU_ITEM(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_POPUP_MENU_ITEM, EsdashboardPopupMenuItem))
#define ESDASHBOARD_IS_POPUP_MENU_ITEM(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_POPUP_MENU_ITEM))
#define ESDASHBOARD_POPUP_MENU_ITEM_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj), ESDASHBOARD_TYPE_POPUP_MENU_ITEM, EsdashboardPopupMenuItemInterface))

typedef struct _EsdashboardPopupMenuItem				EsdashboardPopupMenuItem;
typedef struct _EsdashboardPopupMenuItemInterface		EsdashboardPopupMenuItemInterface;

/**
 * EsdashboardPopupMenuItemInterface:
 * @parent_interface: The parent interface.
 * @get_enabled: Retrieve state if pop-up menu item is enabled or disabled
 * @set_enabled: Set state if pop-up menu item is enabled or disabled
 *
 * Provides an interface implemented by actors which will be used as pop-up menu
 * items in a #EsdashboardPopupMenu.
 */
struct _EsdashboardPopupMenuItemInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface				parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*get_enabled)(EsdashboardPopupMenuItem *self);
	void (*set_enabled)(EsdashboardPopupMenuItem *self, gboolean inEnabled);
};

/* Public API */
GType esdashboard_popup_menu_item_get_type(void) G_GNUC_CONST;

gboolean esdashboard_popup_menu_item_get_enabled(EsdashboardPopupMenuItem *self);
void esdashboard_popup_menu_item_set_enabled(EsdashboardPopupMenuItem *self, gboolean inEnabled);

void esdashboard_popup_menu_item_activate(EsdashboardPopupMenuItem *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_POPUP_MENU_ITEM__ */
