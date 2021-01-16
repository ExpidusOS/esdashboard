/*
 * focusable: An interface which can be inherited by actors to get
 *            managed by focus manager for keyboard navigation and
 *            selection handling
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

#ifndef __LIBESDASHBOARD_FOCUSABLE__
#define __LIBESDASHBOARD_FOCUSABLE__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/types.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_FOCUSABLE				(esdashboard_focusable_get_type())
#define ESDASHBOARD_FOCUSABLE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_FOCUSABLE, EsdashboardFocusable))
#define ESDASHBOARD_IS_FOCUSABLE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_FOCUSABLE))
#define ESDASHBOARD_FOCUSABLE_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj), ESDASHBOARD_TYPE_FOCUSABLE, EsdashboardFocusableInterface))

typedef struct _EsdashboardFocusable			EsdashboardFocusable;
typedef struct _EsdashboardFocusableInterface	EsdashboardFocusableInterface;

struct _EsdashboardFocusableInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface				parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*can_focus)(EsdashboardFocusable *self);
	void (*set_focus)(EsdashboardFocusable *self);
	void (*unset_focus)(EsdashboardFocusable *self);

	gboolean (*supports_selection)(EsdashboardFocusable *self);
	ClutterActor* (*get_selection)(EsdashboardFocusable *self);
	gboolean (*set_selection)(EsdashboardFocusable *self, ClutterActor *inSelection);
	ClutterActor* (*find_selection)(EsdashboardFocusable *self, ClutterActor *inSelection, EsdashboardSelectionTarget inDirection);
	gboolean (*activate_selection)(EsdashboardFocusable *self, ClutterActor *inSelection);

	/* Binding actions */
	gboolean (*selection_move_left)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_right)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_up)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_down)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_first)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_last)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_next)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_previous)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_page_left)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_page_right)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_page_up)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_page_down)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_activate)(EsdashboardFocusable *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);

	gboolean (*focus_move_to)(EsdashboardFocusable *self,
								EsdashboardFocusable *inSource,
								const gchar *inAction,
								ClutterEvent *inEvent);
};

/* Public API */
GType esdashboard_focusable_get_type(void) G_GNUC_CONST;

gboolean esdashboard_focusable_can_focus(EsdashboardFocusable *self);
void esdashboard_focusable_set_focus(EsdashboardFocusable *self);
void esdashboard_focusable_unset_focus(EsdashboardFocusable *self);

gboolean esdashboard_focusable_supports_selection(EsdashboardFocusable *self);
ClutterActor* esdashboard_focusable_get_selection(EsdashboardFocusable *self);
gboolean esdashboard_focusable_set_selection(EsdashboardFocusable *self, ClutterActor *inSelection);
ClutterActor* esdashboard_focusable_find_selection(EsdashboardFocusable *self, ClutterActor *inSelection, EsdashboardSelectionTarget inDirection);
gboolean esdashboard_focusable_activate_selection(EsdashboardFocusable *self, ClutterActor *inSelection);

gboolean esdashboard_focusable_move_focus_to(EsdashboardFocusable *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_FOCUSABLE__ */
