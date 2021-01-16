/*
 * applications-menu-model: A list model containing menu items
 *                          of applications
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

#ifndef __LIBESDASHBOARD_APPLICATIONS_MENU_MODEL__
#define __LIBESDASHBOARD_APPLICATIONS_MENU_MODEL__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <garcon/garcon.h>

#include <libesdashboard/model.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL			(esdashboard_applications_menu_model_get_type())
#define ESDASHBOARD_APPLICATIONS_MENU_MODEL(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL, EsdashboardApplicationsMenuModel))
#define ESDASHBOARD_IS_APPLICATIONS_MENU_MODEL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL))
#define ESDASHBOARD_APPLICATIONS_MENU_MODEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL, EsdashboardApplicationsMenuModelClass))
#define ESDASHBOARD_IS_APPLICATIONS_MENU_MODEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL))
#define ESDASHBOARD_APPLICATIONS_MENU_MODEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL, EsdashboardApplicationsMenuModelClass))

typedef struct _EsdashboardApplicationsMenuModel			EsdashboardApplicationsMenuModel; 
typedef struct _EsdashboardApplicationsMenuModelPrivate		EsdashboardApplicationsMenuModelPrivate;
typedef struct _EsdashboardApplicationsMenuModelClass		EsdashboardApplicationsMenuModelClass;

struct _EsdashboardApplicationsMenuModel
{
	/*< private >*/
	/* Parent instance */
	EsdashboardModel							parent_instance;

	/* Private structure */
	EsdashboardApplicationsMenuModelPrivate		*priv;
};

struct _EsdashboardApplicationsMenuModelClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardModelClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*loaded)(EsdashboardApplicationsMenuModel *self);
};

/* Public API */

/* Columns of model */
enum
{
	ESDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID,

	ESDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT,
	ESDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU,
	ESDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SECTION,

	ESDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE,
	ESDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION,

	ESDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_LAST
};

GType esdashboard_applications_menu_model_get_type(void) G_GNUC_CONST;

EsdashboardModel* esdashboard_applications_menu_model_new(void);

void esdashboard_applications_menu_model_get(EsdashboardApplicationsMenuModel *self,
												EsdashboardModelIter *inIter,
												...);

void esdashboard_applications_menu_model_filter_by_menu(EsdashboardApplicationsMenuModel *self,
														GarconMenu *inMenu);
void esdashboard_applications_menu_model_filter_by_section(EsdashboardApplicationsMenuModel *self,
															GarconMenu *inSection);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_APPLICATIONS_MENU_MODEL__ */

