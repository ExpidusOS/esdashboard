/*
 * quicklaunch: Quicklaunch box
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

#ifndef __LIBESDASHBOARD_QUICKLAUNCH__
#define __LIBESDASHBOARD_QUICKLAUNCH__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/background.h>
#include <libesdashboard/toggle-button.h>
#include <libesdashboard/focusable.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_QUICKLAUNCH				(esdashboard_quicklaunch_get_type())
#define ESDASHBOARD_QUICKLAUNCH(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_QUICKLAUNCH, EsdashboardQuicklaunch))
#define ESDASHBOARD_IS_QUICKLAUNCH(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_QUICKLAUNCH))
#define ESDASHBOARD_QUICKLAUNCH_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_QUICKLAUNCH, EsdashboardQuicklaunchClass))
#define ESDASHBOARD_IS_QUICKLAUNCH_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_QUICKLAUNCH))
#define ESDASHBOARD_QUICKLAUNCH_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_QUICKLAUNCH, EsdashboardQuicklaunchClass))

typedef struct _EsdashboardQuicklaunch				EsdashboardQuicklaunch;
typedef struct _EsdashboardQuicklaunchClass			EsdashboardQuicklaunchClass;
typedef struct _EsdashboardQuicklaunchPrivate		EsdashboardQuicklaunchPrivate;

struct _EsdashboardQuicklaunch
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground			parent_instance;

	/* Private structure */
	EsdashboardQuicklaunchPrivate	*priv;
};

struct _EsdashboardQuicklaunchClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*favourite_added)(EsdashboardQuicklaunch *self, GAppInfo *inAppInfo);
	void (*favourite_removed)(EsdashboardQuicklaunch *self, GAppInfo *inAppInfo);

	/* Binding actions */
	gboolean (*selection_add_favourite)(EsdashboardQuicklaunch *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_remove_favourite)(EsdashboardQuicklaunch *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);

	gboolean (*favourite_reorder_left)(EsdashboardQuicklaunch *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*favourite_reorder_right)(EsdashboardQuicklaunch *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*favourite_reorder_up)(EsdashboardQuicklaunch *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*favourite_reorder_down)(EsdashboardQuicklaunch *self,
											EsdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
};

/* Public API */
GType esdashboard_quicklaunch_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_quicklaunch_new(void);
ClutterActor* esdashboard_quicklaunch_new_with_orientation(ClutterOrientation inOrientation);

gfloat esdashboard_quicklaunch_get_normal_icon_size(EsdashboardQuicklaunch *self);
void esdashboard_quicklaunch_set_normal_icon_size(EsdashboardQuicklaunch *self, const gfloat inIconSize);

gfloat esdashboard_quicklaunch_get_spacing(EsdashboardQuicklaunch *self);
void esdashboard_quicklaunch_set_spacing(EsdashboardQuicklaunch *self, const gfloat inSpacing);

ClutterOrientation esdashboard_quicklaunch_get_orientation(EsdashboardQuicklaunch *self);
void esdashboard_quicklaunch_set_orientation(EsdashboardQuicklaunch *self, ClutterOrientation inOrientation);

EsdashboardToggleButton* esdashboard_quicklaunch_get_apps_button(EsdashboardQuicklaunch *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_QUICKLAUNCH__ */
