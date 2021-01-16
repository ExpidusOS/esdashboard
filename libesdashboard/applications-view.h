/*
 * applications-view: A view showing all installed applications as menu
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

#ifndef __LIBESDASHBOARD_APPLICATIONS_VIEW__
#define __LIBESDASHBOARD_APPLICATIONS_VIEW__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/view.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_APPLICATIONS_VIEW				(esdashboard_applications_view_get_type())
#define ESDASHBOARD_APPLICATIONS_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_APPLICATIONS_VIEW, EsdashboardApplicationsView))
#define ESDASHBOARD_IS_APPLICATIONS_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_APPLICATIONS_VIEW))
#define ESDASHBOARD_APPLICATIONS_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_APPLICATIONS_VIEW, EsdashboardApplicationsViewClass))
#define ESDASHBOARD_IS_APPLICATIONS_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_APPLICATIONS_VIEW))
#define ESDASHBOARD_APPLICATIONS_VIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_APPLICATIONS_VIEW, EsdashboardApplicationsViewClass))

typedef struct _EsdashboardApplicationsView				EsdashboardApplicationsView; 
typedef struct _EsdashboardApplicationsViewPrivate		EsdashboardApplicationsViewPrivate;
typedef struct _EsdashboardApplicationsViewClass		EsdashboardApplicationsViewClass;

struct _EsdashboardApplicationsView
{
	/*< private >*/
	/* Parent instance */
	EsdashboardView						parent_instance;

	/* Private structure */
	EsdashboardApplicationsViewPrivate	*priv;
};

struct _EsdashboardApplicationsViewClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardViewClass				parent_class;
};

/* Public API */
GType esdashboard_applications_view_get_type(void) G_GNUC_CONST;

EsdashboardViewMode esdashboard_applications_view_get_view_mode(EsdashboardApplicationsView *self);
void esdashboard_applications_view_set_view_mode(EsdashboardApplicationsView *self, const EsdashboardViewMode inMode);

gfloat esdashboard_applications_view_get_spacing(EsdashboardApplicationsView *self);
void esdashboard_applications_view_set_spacing(EsdashboardApplicationsView *self, const gfloat inSpacing);

const gchar* esdashboard_applications_view_get_parent_menu_icon(EsdashboardApplicationsView *self);
void esdashboard_applications_view_set_parent_menu_icon(EsdashboardApplicationsView *self, const gchar *inIconName);

const gchar* esdashboard_applications_view_get_format_title_only(EsdashboardApplicationsView *self);
void esdashboard_applications_view_set_format_title_only(EsdashboardApplicationsView *self, const gchar *inFormat);

const gchar* esdashboard_applications_view_get_format_title_description(EsdashboardApplicationsView *self);
void esdashboard_applications_view_set_format_title_description(EsdashboardApplicationsView *self, const gchar *inFormat);

gboolean esdashboard_applications_view_get_show_all_apps(EsdashboardApplicationsView *self);
void esdashboard_applications_view_set_show_all_apps(EsdashboardApplicationsView *self, gboolean inShowAllApps);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_APPLICATIONS_VIEW__ */
