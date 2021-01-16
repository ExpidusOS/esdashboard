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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/popup-menu-item-button.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/popup-menu-item.h>
#include <libesdashboard/click-action.h>
#include <libesdashboard/compat.h>


/* Define this class in GObject system */
static void _esdashboard_popup_menu_item_button_popup_menu_item_iface_init(EsdashboardPopupMenuItemInterface *iface);

struct _EsdashboardPopupMenuItemButtonPrivate
{
	/* Instance related */
	ClutterAction				*clickAction;
	gboolean					enabled;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardPopupMenuItemButton,
						esdashboard_popup_menu_item_button,
						ESDASHBOARD_TYPE_LABEL,
						G_ADD_PRIVATE(EsdashboardPopupMenuItemButton)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_POPUP_MENU_ITEM, _esdashboard_popup_menu_item_button_popup_menu_item_iface_init))

/* IMPLEMENTATION: Private variables and methods */

/* Pop-up menu item button was clicked so activate this item */
static void _esdashboard_popup_menu_item_button_clicked(EsdashboardClickAction *inAction,
														ClutterActor *self,
														gpointer inUserData)
{
	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(inAction));
	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_BUTTON(self));

	/* Only emit any of these signals if click was perform with left button 
	 * or is a short touchscreen touch event.
	 */
	if(esdashboard_click_action_is_left_button_or_tap(inAction))
	{
		esdashboard_popup_menu_item_activate(ESDASHBOARD_POPUP_MENU_ITEM(self));
	}
}

/* IMPLEMENTATION: Interface EsdashboardPopupMenuItem */

/* Get enable state of this pop-up menu item */
static gboolean _esdashboard_popup_menu_item_button_popup_menu_item_get_enabled(EsdashboardPopupMenuItem *inMenuItem)
{
	EsdashboardPopupMenuItemButton			*self;
	EsdashboardPopupMenuItemButtonPrivate	*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_BUTTON(inMenuItem), FALSE);

	self=ESDASHBOARD_POPUP_MENU_ITEM_BUTTON(inMenuItem);
	priv=self->priv;

	/* Return enabled state */
	return(priv->enabled);
}

/* Set enable state of this pop-up menu item */
static void _esdashboard_popup_menu_item_button_popup_menu_item_set_enabled(EsdashboardPopupMenuItem *inMenuItem, gboolean inEnabled)
{
	EsdashboardPopupMenuItemButton			*self;
	EsdashboardPopupMenuItemButtonPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM_BUTTON(inMenuItem));

	self=ESDASHBOARD_POPUP_MENU_ITEM_BUTTON(inMenuItem);
	priv=self->priv;

	/* Set enabled state */
	priv->enabled=inEnabled;
}

/* Interface initialization
 * Set up default functions
 */
void _esdashboard_popup_menu_item_button_popup_menu_item_iface_init(EsdashboardPopupMenuItemInterface *iface)
{
	iface->get_enabled=_esdashboard_popup_menu_item_button_popup_menu_item_get_enabled;
	iface->set_enabled=_esdashboard_popup_menu_item_button_popup_menu_item_set_enabled;
}

/* IMPLEMENTATION: GObject */

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void esdashboard_popup_menu_item_button_class_init(EsdashboardPopupMenuItemButtonClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_popup_menu_item_button_init(EsdashboardPopupMenuItemButton *self)
{
	EsdashboardPopupMenuItemButtonPrivate	*priv;

	priv=self->priv=esdashboard_popup_menu_item_button_get_instance_private(self);

	/* Set up default values */
	priv->enabled=TRUE;

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Connect signals */
	priv->clickAction=esdashboard_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), priv->clickAction);
	g_signal_connect(priv->clickAction, "clicked", G_CALLBACK(_esdashboard_popup_menu_item_button_clicked), NULL);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* esdashboard_popup_menu_item_button_new(void)
{
	return(g_object_new(ESDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON,
						"text", N_(""),
						"label-style", ESDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}

ClutterActor* esdashboard_popup_menu_item_button_new_with_text(const gchar *inText)
{
	return(g_object_new(ESDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON,
						"text", inText,
						"label-style", ESDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}
