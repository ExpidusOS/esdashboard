/*
 * button: A label actor which can react on click actions
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

#ifndef __LIBESDASHBOARD_BUTTON__
#define __LIBESDASHBOARD_BUTTON__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/label.h>

G_BEGIN_DECLS

/* Object declaration */
#define ESDASHBOARD_TYPE_BUTTON					(esdashboard_button_get_type())
#define ESDASHBOARD_BUTTON(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_BUTTON, EsdashboardButton))
#define ESDASHBOARD_IS_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_BUTTON))
#define ESDASHBOARD_BUTTON_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_BUTTON, EsdashboardButtonClass))
#define ESDASHBOARD_IS_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_BUTTON))
#define ESDASHBOARD_BUTTON_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_BUTTON, EsdashboardButtonClass))

typedef struct _EsdashboardButton				EsdashboardButton;
typedef struct _EsdashboardButtonClass			EsdashboardButtonClass;
typedef struct _EsdashboardButtonPrivate		EsdashboardButtonPrivate;

struct _EsdashboardButton
{
	/*< private >*/
	/* Parent instance */
	EsdashboardLabel			parent_instance;

	/* Private structure */
	EsdashboardButtonPrivate	*priv;
};

struct _EsdashboardButtonClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardLabelClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(EsdashboardButton *self);
};

/* Public API */
GType esdashboard_button_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_button_new(void);
ClutterActor* esdashboard_button_new_with_text(const gchar *inText);
ClutterActor* esdashboard_button_new_with_icon_name(const gchar *inIconName);
ClutterActor* esdashboard_button_new_with_gicon(GIcon *inIcon);
ClutterActor* esdashboard_button_new_full_with_icon_name(const gchar *inIconName, const gchar *inText);
ClutterActor* esdashboard_button_new_full_with_gicon(GIcon *inIcon, const gchar *inText);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_BUTTON__ */
