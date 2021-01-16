/*
 * popup-menu-item-separator: A separator menu item
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

#ifndef __LIBESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR__
#define __LIBESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/background.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR				(esdashboard_popup_menu_item_separator_get_type())
#define ESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR, EsdashboardPopupMenuItemSeparator))
#define ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR))
#define ESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR, EsdashboardPopupMenuItemSeparatorClass))
#define ESDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR))
#define ESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR, EsdashboardPopupMenuItemSeparatorClass))

typedef struct _EsdashboardPopupMenuItemSeparator				EsdashboardPopupMenuItemSeparator;
typedef struct _EsdashboardPopupMenuItemSeparatorClass			EsdashboardPopupMenuItemSeparatorClass;
typedef struct _EsdashboardPopupMenuItemSeparatorPrivate		EsdashboardPopupMenuItemSeparatorPrivate;

/**
 * EsdashboardPopupMenuItemSeparator:
 *
 * The #EsdashboardPopupMenuItemSeparator structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardPopupMenuItemSeparator
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground							parent_instance;

	/* Private structure */
	EsdashboardPopupMenuItemSeparatorPrivate		*priv;
};

/**
 * EsdashboardPopupMenuItemSeparatorClass:
 *
 * The #EsdashboardPopupMenuItemSeparatorClass structure contains only private data
 */
struct _EsdashboardPopupMenuItemSeparatorClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_popup_menu_item_separator_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_popup_menu_item_separator_new(void);

gfloat esdashboard_popup_menu_item_separator_get_minimum_height(EsdashboardPopupMenuItemSeparator *self);
void esdashboard_popup_menu_item_separator_set_minimum_height(EsdashboardPopupMenuItemSeparator *self, const gfloat inMinimumHeight);

gfloat esdashboard_popup_menu_item_separator_get_line_horizontal_alignment(EsdashboardPopupMenuItemSeparator *self);
void esdashboard_popup_menu_item_separator_set_line_horizontal_alignment(EsdashboardPopupMenuItemSeparator *self, const gfloat inAlignment);

gfloat esdashboard_popup_menu_item_separator_get_line_vertical_alignment(EsdashboardPopupMenuItemSeparator *self);
void esdashboard_popup_menu_item_separator_set_line_vertical_alignment(EsdashboardPopupMenuItemSeparator *self, const gfloat inAlignment);

gfloat esdashboard_popup_menu_item_separator_get_line_length(EsdashboardPopupMenuItemSeparator *self);
void esdashboard_popup_menu_item_separator_set_line_length(EsdashboardPopupMenuItemSeparator *self, const gfloat inLength);

gfloat esdashboard_popup_menu_item_separator_get_line_width(EsdashboardPopupMenuItemSeparator *self);
void esdashboard_popup_menu_item_separator_set_line_width(EsdashboardPopupMenuItemSeparator *self, const gfloat inWidth);

const ClutterColor* esdashboard_popup_menu_item_separator_get_line_color(EsdashboardPopupMenuItemSeparator *self);
void esdashboard_popup_menu_item_separator_set_line_color(EsdashboardPopupMenuItemSeparator *self, const ClutterColor *inColor);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_POPUP_MENU_ITEM_SEPARATOR__ */
