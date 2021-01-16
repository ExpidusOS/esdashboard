/*
 * viewpad: A viewpad managing views
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

#ifndef __LIBESDASHBOARD_VIEWPAD__
#define __LIBESDASHBOARD_VIEWPAD__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/background.h>
#include <libesdashboard/view.h>
#include <libesdashboard/types.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_VIEWPAD				(esdashboard_viewpad_get_type())
#define ESDASHBOARD_VIEWPAD(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_VIEWPAD, EsdashboardViewpad))
#define ESDASHBOARD_IS_VIEWPAD(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_VIEWPAD))
#define ESDASHBOARD_VIEWPAD_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_VIEWPAD, EsdashboardViewpadClass))
#define ESDASHBOARD_IS_VIEWPAD_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_VIEWPAD))
#define ESDASHBOARD_VIEWPAD_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_VIEWPAD, EsdashboardViewpadClass))

typedef struct _EsdashboardViewpad				EsdashboardViewpad; 
typedef struct _EsdashboardViewpadPrivate		EsdashboardViewpadPrivate;
typedef struct _EsdashboardViewpadClass			EsdashboardViewpadClass;

struct _EsdashboardViewpad
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground		parent_instance;

	/* Private structure */
	EsdashboardViewpadPrivate	*priv;
};

struct _EsdashboardViewpadClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass	parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*view_added)(EsdashboardViewpad *self, EsdashboardView *inView);
	void (*view_removed)(EsdashboardViewpad *self, EsdashboardView *inView);

	void (*view_activating)(EsdashboardViewpad *self, EsdashboardView *inView);
	void (*view_activated)(EsdashboardViewpad *self, EsdashboardView *inView);
	void (*view_deactivating)(EsdashboardViewpad *self, EsdashboardView *inView);
	void (*view_deactivated)(EsdashboardViewpad *self, EsdashboardView *inView);
};

/* Public API */
GType esdashboard_viewpad_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_viewpad_new(void);

gfloat esdashboard_viewpad_get_spacing(EsdashboardViewpad *self);
void esdashboard_viewpad_set_spacing(EsdashboardViewpad *self, gfloat inSpacing);

GList* esdashboard_viewpad_get_views(EsdashboardViewpad *self);
gboolean esdashboard_viewpad_has_view(EsdashboardViewpad *self, EsdashboardView *inView);
EsdashboardView* esdashboard_viewpad_find_view_by_type(EsdashboardViewpad *self, GType inType);
EsdashboardView* esdashboard_viewpad_find_view_by_id(EsdashboardViewpad *self, const gchar *inID);

EsdashboardView* esdashboard_viewpad_get_active_view(EsdashboardViewpad *self);
void esdashboard_viewpad_set_active_view(EsdashboardViewpad *self, EsdashboardView *inView);

gboolean esdashboard_viewpad_get_horizontal_scrollbar_visible(EsdashboardViewpad *self);
gboolean esdashboard_viewpad_get_vertical_scrollbar_visible(EsdashboardViewpad *self);

EsdashboardVisibilityPolicy esdashboard_viewpad_get_horizontal_scrollbar_policy(EsdashboardViewpad *self);
void esdashboard_viewpad_set_horizontal_scrollbar_policy(EsdashboardViewpad *self, EsdashboardVisibilityPolicy inPolicy);

EsdashboardVisibilityPolicy esdashboard_viewpad_get_vertical_scrollbar_policy(EsdashboardViewpad *self);
void esdashboard_viewpad_set_vertical_scrollbar_policy(EsdashboardViewpad *self, EsdashboardVisibilityPolicy inPolicy);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_VIEWPAD__ */
