/*
 * popup-menu: A pop-up menu with entries performing an action when an entry
 *             was clicked
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

#ifndef __LIBESDASHBOARD_POPUP_MENU__
#define __LIBESDASHBOARD_POPUP_MENU__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/background.h>
#include <libesdashboard/popup-menu-item.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_POPUP_MENU				(esdashboard_popup_menu_get_type())
#define ESDASHBOARD_POPUP_MENU(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_POPUP_MENU, EsdashboardPopupMenu))
#define ESDASHBOARD_IS_POPUP_MENU(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_POPUP_MENU))
#define ESDASHBOARD_POPUP_MENU_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_POPUP_MENU, EsdashboardPopupMenuClass))
#define ESDASHBOARD_IS_POPUP_MENU_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_POPUP_MENU))
#define ESDASHBOARD_POPUP_MENU_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_POPUP_MENU, EsdashboardPopupMenuClass))

typedef struct _EsdashboardPopupMenu			EsdashboardPopupMenu;
typedef struct _EsdashboardPopupMenuClass		EsdashboardPopupMenuClass;
typedef struct _EsdashboardPopupMenuPrivate		EsdashboardPopupMenuPrivate;

/**
 * EsdashboardPopupMenu:
 *
 * The #EsdashboardPopupMenu structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardPopupMenu
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground				parent_instance;

	/* Private structure */
	EsdashboardPopupMenuPrivate			*priv;
};

/**
 * EsdashboardPopupMenuClass:
 *
 * The #EsdashboardPopupMenuClass structure contains only private data
 */
struct _EsdashboardPopupMenuClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*activated)(EsdashboardPopupMenu *self);
	void (*cancelled)(EsdashboardPopupMenu *self);

	void (*item_activated)(EsdashboardPopupMenu *self, EsdashboardPopupMenuItem *inMenuItem);

	void (*item_added)(EsdashboardPopupMenu *self, EsdashboardPopupMenuItem *inMenuItem);
	void (*item_removed)(EsdashboardPopupMenu *self, EsdashboardPopupMenuItem *inMenuItem);
};

/* Public API */
GType esdashboard_popup_menu_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_popup_menu_new(void);
ClutterActor* esdashboard_popup_menu_new_for_source(ClutterActor *inSource);

gboolean esdashboard_popup_menu_get_destroy_on_cancel(EsdashboardPopupMenu *self);
void esdashboard_popup_menu_set_destroy_on_cancel(EsdashboardPopupMenu *self, gboolean inDestroyOnCancel);

ClutterActor* esdashboard_popup_menu_get_source(EsdashboardPopupMenu *self);
void esdashboard_popup_menu_set_source(EsdashboardPopupMenu *self, ClutterActor *inSource);

gboolean esdashboard_popup_menu_get_show_title(EsdashboardPopupMenu *self);
void esdashboard_popup_menu_set_show_title(EsdashboardPopupMenu *self, gboolean inShowTitle);

const gchar* esdashboard_popup_menu_get_title(EsdashboardPopupMenu *self);
void esdashboard_popup_menu_set_title(EsdashboardPopupMenu *self, const gchar *inMarkupTitle);

gboolean esdashboard_popup_menu_get_show_title_icon(EsdashboardPopupMenu *self);
void esdashboard_popup_menu_set_show_title_icon(EsdashboardPopupMenu *self, gboolean inShowTitleIcon);

const gchar* esdashboard_popup_menu_get_title_icon_name(EsdashboardPopupMenu *self);
void esdashboard_popup_menu_set_title_icon_name(EsdashboardPopupMenu *self, const gchar *inIconName);

GIcon* esdashboard_popup_menu_get_title_gicon(EsdashboardPopupMenu *self);
void esdashboard_popup_menu_set_title_gicon(EsdashboardPopupMenu *self, GIcon *inIcon);


gint esdashboard_popup_menu_add_item(EsdashboardPopupMenu *self,
										EsdashboardPopupMenuItem *inMenuItem);
gint esdashboard_popup_menu_insert_item(EsdashboardPopupMenu *self,
										EsdashboardPopupMenuItem *inMenuItem,
										gint inIndex);
gboolean esdashboard_popup_menu_move_item(EsdashboardPopupMenu *self,
											EsdashboardPopupMenuItem *inMenuItem,
											gint inIndex);
EsdashboardPopupMenuItem* esdashboard_popup_menu_get_item(EsdashboardPopupMenu *self, gint inIndex);
gint esdashboard_popup_menu_get_item_index(EsdashboardPopupMenu *self, EsdashboardPopupMenuItem *inMenuItem);
gboolean esdashboard_popup_menu_remove_item(EsdashboardPopupMenu *self, EsdashboardPopupMenuItem *inMenuItem);


void esdashboard_popup_menu_activate(EsdashboardPopupMenu *self);
void esdashboard_popup_menu_cancel(EsdashboardPopupMenu *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_POPUP_MENU__ */
