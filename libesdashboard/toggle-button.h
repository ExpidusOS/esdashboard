/*
 * toggle-button: A button which can toggle its state between on and off
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

#ifndef __LIBESDASHBOARD_TOGGLE_BUTTON__
#define __LIBESDASHBOARD_TOGGLE_BUTTON__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/button.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_TOGGLE_BUTTON				(esdashboard_toggle_button_get_type())
#define ESDASHBOARD_TOGGLE_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_TOGGLE_BUTTON, EsdashboardToggleButton))
#define ESDASHBOARD_IS_TOGGLE_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_TOGGLE_BUTTON))
#define ESDASHBOARD_TOGGLE_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_TOGGLE_BUTTON, EsdashboardToggleButtonClass))
#define ESDASHBOARD_IS_TOGGLE_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_TOGGLE_BUTTON))
#define ESDASHBOARD_TOGGLE_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_TOGGLE_BUTTON, EsdashboardToggleButtonClass))

typedef struct _EsdashboardToggleButton				EsdashboardToggleButton;
typedef struct _EsdashboardToggleButtonClass		EsdashboardToggleButtonClass;
typedef struct _EsdashboardToggleButtonPrivate		EsdashboardToggleButtonPrivate;

/**
 * EsdashboardToggleButton:
 *
 * The #EsdashboardToggleButton structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardToggleButton
{
	/*< private >*/
	/* Parent instance */
	EsdashboardButton				parent_instance;

	/* Private structure */
	EsdashboardToggleButtonPrivate	*priv;
};

/**
 * EsdashboardToggleButtonClass:
 * @toggled: Class handler for the #EsdashboardToggleButtonClass::toggled signal
 *
 * The #EsdashboardToggleButtonClass structure contains only private data
 */
struct _EsdashboardToggleButtonClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardButtonClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*toggled)(EsdashboardToggleButton *self);
};

/* Public API */
GType esdashboard_toggle_button_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_toggle_button_new(void);
ClutterActor* esdashboard_toggle_button_new_with_text(const gchar *inText);
ClutterActor* esdashboard_toggle_button_new_with_icon_name(const gchar *inIconName);
ClutterActor* esdashboard_toggle_button_new_with_gicon(GIcon *inIcon);
ClutterActor* esdashboard_toggle_button_new_full_with_icon_name(const gchar *inIconName, const gchar *inText);
ClutterActor* esdashboard_toggle_button_new_full_with_gicon(GIcon *inIcon, const gchar *inText);

gboolean esdashboard_toggle_button_get_toggle_state(EsdashboardToggleButton *self);
void esdashboard_toggle_button_set_toggle_state(EsdashboardToggleButton *self, gboolean inToggleState);

gboolean esdashboard_toggle_button_get_auto_toggle(EsdashboardToggleButton *self);
void esdashboard_toggle_button_set_auto_toggle(EsdashboardToggleButton *self, gboolean inAuto);

void esdashboard_toggle_button_toggle(EsdashboardToggleButton *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_TOGGLE_BUTTON__ */
