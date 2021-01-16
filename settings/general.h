/*
 * general: General settings of application
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

#ifndef __ESDASHBOARD_SETTINGS_GENERAL__
#define __ESDASHBOARD_SETTINGS_GENERAL__

#include <gtk/gtkx.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_SETTINGS_GENERAL				(esdashboard_settings_general_get_type())
#define ESDASHBOARD_SETTINGS_GENERAL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_SETTINGS_GENERAL, EsdashboardSettingsGeneral))
#define ESDASHBOARD_IS_SETTINGS_GENERAL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_SETTINGS_GENERAL))
#define ESDASHBOARD_SETTINGS_GENERAL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_SETTINGS_GENERAL, EsdashboardSettingsGeneralClass))
#define ESDASHBOARD_IS_SETTINGS_GENERAL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_SETTINGS_GENERAL))
#define ESDASHBOARD_SETTINGS_GENERAL_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_SETTINGS_GENERAL, EsdashboardSettingsGeneralClass))

typedef struct _EsdashboardSettingsGeneral				EsdashboardSettingsGeneral;
typedef struct _EsdashboardSettingsGeneralClass			EsdashboardSettingsGeneralClass;
typedef struct _EsdashboardSettingsGeneralPrivate		EsdashboardSettingsGeneralPrivate;

struct _EsdashboardSettingsGeneral
{
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	EsdashboardSettingsGeneralPrivate	*priv;
};

struct _EsdashboardSettingsGeneralClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_settings_general_get_type(void) G_GNUC_CONST;

EsdashboardSettingsGeneral* esdashboard_settings_general_new(GtkBuilder *inBuilder);

G_END_DECLS

#endif	/* __ESDASHBOARD_SETTINGS_GENERAL__ */
