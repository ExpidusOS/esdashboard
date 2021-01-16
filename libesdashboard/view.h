/*
 * view: Abstract class for views, optional with scrollbars
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

#ifndef __LIBESDASHBOARD_VIEW__
#define __LIBESDASHBOARD_VIEW__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/actor.h>
#include <libesdashboard/types.h>
#include <libesdashboard/focusable.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * EsdashboardViewFitMode:
 * @ESDASHBOARD_VIEW_FIT_MODE_NONE: Do not try to fit view into viewpad.
 * @ESDASHBOARD_VIEW_FIT_MODE_HORIZONTAL: Try to fit view into viewpad horizontally.
 * @ESDASHBOARD_VIEW_FIT_MODE_VERTICAL: Try to fit view into viewpad vertically.
 * @ESDASHBOARD_VIEW_FIT_MODE_BOTH: Try to fit view into viewpad horizontally and vertically.
 *
 * Determines how a view should fit into a viewpad.
 */
typedef enum /*< prefix=ESDASHBOARD_VIEW_FIT_MODE >*/
{
	ESDASHBOARD_VIEW_FIT_MODE_NONE=0,
	ESDASHBOARD_VIEW_FIT_MODE_HORIZONTAL,
	ESDASHBOARD_VIEW_FIT_MODE_VERTICAL,
	ESDASHBOARD_VIEW_FIT_MODE_BOTH
} EsdashboardViewFitMode;


/* Object declaration */
#define ESDASHBOARD_TYPE_VIEW				(esdashboard_view_get_type())
#define ESDASHBOARD_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_VIEW, EsdashboardView))
#define ESDASHBOARD_IS_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_VIEW))
#define ESDASHBOARD_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_VIEW, EsdashboardViewClass))
#define ESDASHBOARD_IS_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_VIEW))
#define ESDASHBOARD_VIEW_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_VIEW, EsdashboardViewClass))

typedef struct _EsdashboardView				EsdashboardView; 
typedef struct _EsdashboardViewPrivate		EsdashboardViewPrivate;
typedef struct _EsdashboardViewClass		EsdashboardViewClass;

struct _EsdashboardView
{
	/*< private >*/
	/* Parent instance */
	EsdashboardActor			parent_instance;

	/* Private structure */
	EsdashboardViewPrivate		*priv;
};

struct _EsdashboardViewClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardActorClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*set_view_fit_mode)(EsdashboardView *self, EsdashboardViewFitMode inFitMode);

	void (*activating)(EsdashboardView *self);
	void (*activated)(EsdashboardView *self);
	void (*deactivating)(EsdashboardView *self);
	void (*deactivated)(EsdashboardView *self);

	void (*enabling)(EsdashboardView *self);
	void (*enabled)(EsdashboardView *self);
	void (*disabling)(EsdashboardView *self);
	void (*disabled)(EsdashboardView *self);

	void (*name_changed)(EsdashboardView *self, gchar *inName);
	void (*icon_changed)(EsdashboardView *self, ClutterImage *inIcon);

	void (*scroll_to)(EsdashboardView *self, gfloat inX, gfloat inY);
	gboolean (*child_needs_scroll)(EsdashboardView *self, ClutterActor *inActor);
	void (*child_ensure_visible)(EsdashboardView *self, ClutterActor *inActor);

	/* Binding actions */
	gboolean (*view_activate)(EsdashboardView *self,
								EsdashboardFocusable *inSource,
								const gchar *inAction,
								ClutterEvent *inEvent);
};


/* Public API */
GType esdashboard_view_get_type(void) G_GNUC_CONST;

const gchar* esdashboard_view_get_id(EsdashboardView *self);
gboolean esdashboard_view_has_id(EsdashboardView *self, const gchar *inID);

const gchar* esdashboard_view_get_name(EsdashboardView *self);
void esdashboard_view_set_name(EsdashboardView *self, const gchar *inName);

const gchar* esdashboard_view_get_icon(EsdashboardView *self);
void esdashboard_view_set_icon(EsdashboardView *self, const gchar *inIcon);

EsdashboardViewFitMode esdashboard_view_get_view_fit_mode(EsdashboardView *self);
void esdashboard_view_set_view_fit_mode(EsdashboardView *self, EsdashboardViewFitMode inFitMode);

gboolean esdashboard_view_get_enabled(EsdashboardView *self);
void esdashboard_view_set_enabled(EsdashboardView *self, gboolean inIsEnabled);

void esdashboard_view_scroll_to(EsdashboardView *self, gfloat inX, gfloat inY);
gboolean esdashboard_view_child_needs_scroll(EsdashboardView *self, ClutterActor *inActor);
void esdashboard_view_child_ensure_visible(EsdashboardView *self, ClutterActor *inActor);

gboolean esdashboard_view_has_focus(EsdashboardView *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_VIEW__ */
