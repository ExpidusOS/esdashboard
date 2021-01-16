/*
 * windows-view: A view showing visible windows
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

#ifndef __LIBESDASHBOARD_WINDOWS_VIEW__
#define __LIBESDASHBOARD_WINDOWS_VIEW__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/view.h>
#include <libesdashboard/focusable.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_WINDOWS_VIEW				(esdashboard_windows_view_get_type())
#define ESDASHBOARD_WINDOWS_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_WINDOWS_VIEW, EsdashboardWindowsView))
#define ESDASHBOARD_IS_WINDOWS_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_WINDOWS_VIEW))
#define ESDASHBOARD_WINDOWS_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_WINDOWS_VIEW, EsdashboardWindowsViewClass))
#define ESDASHBOARD_IS_WINDOWS_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_WINDOWS_VIEW))
#define ESDASHBOARD_WINDOWS_VIEW_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_WINDOWS_VIEW, EsdashboardWindowsViewClass))

typedef struct _EsdashboardWindowsView				EsdashboardWindowsView; 
typedef struct _EsdashboardWindowsViewPrivate		EsdashboardWindowsViewPrivate;
typedef struct _EsdashboardWindowsViewClass			EsdashboardWindowsViewClass;

struct _EsdashboardWindowsView
{
	/*< private >*/
	/* Parent instance */
	EsdashboardView					parent_instance;

	/* Private structure */
	EsdashboardWindowsViewPrivate	*priv;
};

struct _EsdashboardWindowsViewClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardViewClass			parent_class;

	/*< public >*/
	/* Virtual functions */

	/* Binding actions */
	gboolean (*window_close)(EsdashboardWindowsView *self,
								EsdashboardFocusable *inSource,
								const gchar *inAction,
								ClutterEvent *inEvent);
	gboolean (*windows_show_numbers)(EsdashboardWindowsView *self,
										EsdashboardFocusable *inSource,
										const gchar *inAction,
										ClutterEvent *inEvent);
	gboolean (*windows_hide_numbers)(EsdashboardWindowsView *self,
										EsdashboardFocusable *inSource,
										const gchar *inAction,
										ClutterEvent *inEvent);
	gboolean (*windows_activate_window_one)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_two)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_three)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_four)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_five)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_six)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_seven)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_eight)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_nine)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_ten)(EsdashboardWindowsView *self,
												EsdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);

};

/* Public API */
GType esdashboard_windows_view_get_type(void) G_GNUC_CONST;

gfloat esdashboard_windows_view_get_spacing(EsdashboardWindowsView *self);
void esdashboard_windows_view_set_spacing(EsdashboardWindowsView *self, const gfloat inSpacing);

gboolean esdashboard_windows_view_get_prevent_upscaling(EsdashboardWindowsView *self);
void esdashboard_windows_view_set_prevent_upscaling(EsdashboardWindowsView *self, gboolean inPreventUpscaling);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_WINDOWS_VIEW__ */
