/*
 * action-button: A button representing an action to execute when clicked
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

#ifndef __LIBESDASHBOARD_ACTION_BUTTON__
#define __LIBESDASHBOARD_ACTION_BUTTON__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/button.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_ACTION_BUTTON				(esdashboard_action_button_get_type())
#define ESDASHBOARD_ACTION_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_ACTION_BUTTON, EsdashboardActionButton))
#define ESDASHBOARD_IS_ACTION_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_ACTION_BUTTON))
#define ESDASHBOARD_ACTION_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_ACTION_BUTTON, EsdashboardActionButtonClass))
#define ESDASHBOARD_IS_ACTION_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_ACTION_BUTTON))
#define ESDASHBOARD_ACTION_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_ACTION_BUTTON, EsdashboardActionButtonClass))

typedef struct _EsdashboardActionButton				EsdashboardActionButton;
typedef struct _EsdashboardActionButtonClass		EsdashboardActionButtonClass;
typedef struct _EsdashboardActionButtonPrivate		EsdashboardActionButtonPrivate;

/**
 * EsdashboardActionButton:
 *
 * The #EsdashboardActionButton structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardActionButton
{
	/*< private >*/
	/* Parent instance */
	EsdashboardButton						parent_instance;

	/* Private structure */
	EsdashboardActionButtonPrivate			*priv;
};

/**
 * EsdashboardActionButtonClass:
 *
 * The #EsdashboardActionButtonClass structure contains only private data
 */
struct _EsdashboardActionButtonClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardButtonClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_action_button_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_action_button_new(void);

const gchar* esdashboard_action_button_get_target(EsdashboardActionButton *self);
void esdashboard_action_button_set_target(EsdashboardActionButton *self, const gchar *inTarget);

const gchar* esdashboard_action_button_get_action(EsdashboardActionButton *self);
void esdashboard_action_button_set_action(EsdashboardActionButton *self, const gchar *inAction);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_ACTION_BUTTON__ */
