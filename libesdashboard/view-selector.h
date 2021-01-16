/*
 * view-selector: A selector for registered views
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

#ifndef __LIBESDASHBOARD_VIEW_SELECTOR__
#define __LIBESDASHBOARD_VIEW_SELECTOR__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/actor.h>
#include <libesdashboard/viewpad.h>
#include <libesdashboard/toggle-button.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_VIEW_SELECTOR				(esdashboard_view_selector_get_type())
#define ESDASHBOARD_VIEW_SELECTOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_VIEW_SELECTOR, EsdashboardViewSelector))
#define ESDASHBOARD_IS_VIEW_SELECTOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_VIEW_SELECTOR))
#define ESDASHBOARD_VIEW_SELECTOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_VIEW_SELECTOR, EsdashboardViewSelectorClass))
#define ESDASHBOARD_IS_VIEW_SELECTOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_VIEW_SELECTOR))
#define ESDASHBOARD_VIEW_SELECTOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_VIEW_SELECTOR, EsdashboardViewSelectorClass))

typedef struct _EsdashboardViewSelector				EsdashboardViewSelector; 
typedef struct _EsdashboardViewSelectorPrivate		EsdashboardViewSelectorPrivate;
typedef struct _EsdashboardViewSelectorClass		EsdashboardViewSelectorClass;

/**
 * EsdashboardViewSelector:
 *
 * The #EsdashboardViewSelector structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardViewSelector
{
	/*< private >*/
	/* Parent instance */
	EsdashboardActor				parent_instance;

	/* Private structure */
	EsdashboardViewSelectorPrivate	*priv;
};

/**
 * EsdashboardViewSelectorClass:
 * @state_changed: Class handler for the #EsdashboardViewSelectorClass::state_changed signal
 *
 * The #EsdashboardViewSelectorClass structure contains only private data
 */
struct _EsdashboardViewSelectorClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardActorClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*state_changed)(EsdashboardViewSelector *self, EsdashboardToggleButton *inButton);
};

/* Public API */
GType esdashboard_view_selector_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_view_selector_new(void);
ClutterActor* esdashboard_view_selector_new_for_viewpad(EsdashboardViewpad *inViewpad);

EsdashboardViewpad* esdashboard_view_selector_get_viewpad(EsdashboardViewSelector *self);
void esdashboard_view_selector_set_viewpad(EsdashboardViewSelector *self, EsdashboardViewpad *inViewpad);

gfloat esdashboard_view_selector_get_spacing(EsdashboardViewSelector *self);
void esdashboard_view_selector_set_spacing(EsdashboardViewSelector *self, gfloat inSpacing);

ClutterOrientation esdashboard_view_selector_get_orientation(EsdashboardViewSelector *self);
void esdashboard_view_selector_set_orientation(EsdashboardViewSelector *self, ClutterOrientation inOrientation);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_VIEW_SELECTOR__ */
