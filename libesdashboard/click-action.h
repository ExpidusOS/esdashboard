/*
 * click-action: Bad workaround for click action which prevent drag actions
 *               to work properly since clutter version 1.12 at least.
 *               This object/file is a complete copy of the original
 *               clutter-click-action.{c,h} files of clutter 1.12 except
 *               for one line, the renamed function names and the applied
 *               coding style. The clutter-click-action.{c,h} files of
 *               later clutter versions do not differ much from this one
 *               so this object should work also for this versions.
 *
 *               See bug: https://bugzilla.gnome.org/show_bug.cgi?id=714993
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
 *         original by Emmanuele Bassi <ebassi@linux.intel.com>
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

#ifndef __LIBESDASHBOARD_CLICK_ACTION__
#define __LIBESDASHBOARD_CLICK_ACTION__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * ESDASHBOARD_CLICK_ACTION_LEFT_BUTTON:
 *
 * A helper macro to determine left button clicks using
 * the function esdashboard_click_action_get_button().
 * The value for this macro is an unsinged integer.
 **/
#define ESDASHBOARD_CLICK_ACTION_LEFT_BUTTON 		((guint)1)

/**
 * ESDASHBOARD_CLICK_ACTION_MIDDLE_BUTTON:
 *
 * A helper macro to determine middle button clicks using
 * the function esdashboard_click_action_get_button().
 * The value for this macro is an unsinged integer.
 **/
#define ESDASHBOARD_CLICK_ACTION_MIDDLE_BUTTON 		((guint)2)

/**
 * ESDASHBOARD_CLICK_ACTION_RIGHT_BUTTON:
 *
 * A helper macro to determine right button clicks using
 * the function esdashboard_click_action_get_button().
 * The value for this macro is an unsinged integer.
 **/
#define ESDASHBOARD_CLICK_ACTION_RIGHT_BUTTON 		((guint)3)


/* Object declaration */
#define ESDASHBOARD_TYPE_CLICK_ACTION				(esdashboard_click_action_get_type ())
#define ESDASHBOARD_CLICK_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ESDASHBOARD_TYPE_CLICK_ACTION, EsdashboardClickAction))
#define ESDASHBOARD_IS_CLICK_ACTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ESDASHBOARD_TYPE_CLICK_ACTION))
#define ESDASHBOARD_CLICK_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), ESDASHBOARD_TYPE_CLICK_ACTION, EsdashboardClickActionClass))
#define ESDASHBOARD_IS_CLICK_ACTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), ESDASHBOARD_TYPE_CLICK_ACTION))
#define ESDASHBOARD_CLICK_ACTION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), ESDASHBOARD_TYPE_CLICK_ACTION, EsdashboardClickActionClass))

typedef struct _EsdashboardClickAction				EsdashboardClickAction;
typedef struct _EsdashboardClickActionPrivate		EsdashboardClickActionPrivate;
typedef struct _EsdashboardClickActionClass			EsdashboardClickActionClass;

/**
 * EsdashboardClickAction:
 *
 * The #EsdashboardClickAction structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardClickAction
{
	/*< private >*/
	/* Parent instance */
	ClutterAction					parent_instance;

	EsdashboardClickActionPrivate	*priv;
};

/**
 * EsdashboardClickActionClass:
 * @clicked: Class handler for the #EsdashboardClickActionClass::clicked signal
 * @long_press: Class handler for the #EsdashboardClickActionClass::long-press signal
 *
 * The #EsdashboardClickActionClass structure contains only private data
 */
struct _EsdashboardClickActionClass
{
	/*< private >*/
	/* Parent class */
	ClutterActionClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(EsdashboardClickAction *self, ClutterActor *inActor);
	gboolean (*long_press)(EsdashboardClickAction *self, ClutterActor *inActor, ClutterLongPressState inState);
};

/* Public API */
GType esdashboard_click_action_get_type(void) G_GNUC_CONST;

ClutterAction* esdashboard_click_action_new(void);

guint esdashboard_click_action_get_button(EsdashboardClickAction *self);
ClutterModifierType esdashboard_click_action_get_state(EsdashboardClickAction *self);
void esdashboard_click_action_get_coords(EsdashboardClickAction *self, gfloat *outPressX, gfloat *outPressY);

void esdashboard_click_action_release(EsdashboardClickAction *self);

gboolean esdashboard_click_action_is_left_button_or_tap(EsdashboardClickAction *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_CLICK_ACTION__ */
