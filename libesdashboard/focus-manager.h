/*
 * focus-manager: Single-instance managing focusable actors
 *                for keyboard navigation
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

#ifndef __LIBESDASHBOARD_FOCUS_MANAGER__
#define __LIBESDASHBOARD_FOCUS_MANAGER__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libesdashboard/focusable.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_FOCUS_MANAGER				(esdashboard_focus_manager_get_type())
#define ESDASHBOARD_FOCUS_MANAGER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_FOCUS_MANAGER, EsdashboardFocusManager))
#define ESDASHBOARD_IS_FOCUS_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_FOCUS_MANAGER))
#define ESDASHBOARD_FOCUS_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_FOCUS_MANAGER, EsdashboardFocusManagerClass))
#define ESDASHBOARD_IS_FOCUS_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_FOCUS_MANAGER))
#define ESDASHBOARD_FOCUS_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_FOCUS_MANAGER, EsdashboardFocusManagerClass))

typedef struct _EsdashboardFocusManager				EsdashboardFocusManager;
typedef struct _EsdashboardFocusManagerClass		EsdashboardFocusManagerClass;
typedef struct _EsdashboardFocusManagerPrivate		EsdashboardFocusManagerPrivate;

struct _EsdashboardFocusManager
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	EsdashboardFocusManagerPrivate	*priv;
};

struct _EsdashboardFocusManagerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*registered)(EsdashboardFocusManager *self, EsdashboardFocusable *inActor);
	void (*unregistered)(EsdashboardFocusManager *self, EsdashboardFocusable *inActor);

	void (*changed)(EsdashboardFocusManager *self,
						EsdashboardFocusable *oldActor,
						EsdashboardFocusable *newActor);

	/* Binding actions */
	gboolean (*focus_move_first)(EsdashboardFocusManager *self,
									EsdashboardFocusable *inSource,
									const gchar *inAction,
									ClutterEvent *inEvent);
	gboolean (*focus_move_last)(EsdashboardFocusManager *self,
								EsdashboardFocusable *inSource,
								const gchar *inAction,
								ClutterEvent *inEvent);
	gboolean (*focus_move_next)(EsdashboardFocusManager *self,
									EsdashboardFocusable *inSource,
									const gchar *inAction,
									ClutterEvent *inEvent);
	gboolean (*focus_move_previous)(EsdashboardFocusManager *self,
									EsdashboardFocusable *inSource,
									const gchar *inAction,
									ClutterEvent *inEvent);
};

/* Public API */
GType esdashboard_focus_manager_get_type(void) G_GNUC_CONST;

EsdashboardFocusManager* esdashboard_focus_manager_get_default(void);

void esdashboard_focus_manager_register(EsdashboardFocusManager *self, EsdashboardFocusable *inFocusable);
void esdashboard_focus_manager_register_after(EsdashboardFocusManager *self, EsdashboardFocusable *inFocusable, EsdashboardFocusable *inAfterFocusable);
void esdashboard_focus_manager_unregister(EsdashboardFocusManager *self, EsdashboardFocusable *inFocusable);
GList* esdashboard_focus_manager_get_registered(EsdashboardFocusManager *self);
gboolean esdashboard_focus_manager_is_registered(EsdashboardFocusManager *self, EsdashboardFocusable *inFocusable);

GSList* esdashboard_focus_manager_get_targets(EsdashboardFocusManager *self, const gchar *inTarget);

gboolean esdashboard_focus_manager_has_focus(EsdashboardFocusManager *self, EsdashboardFocusable *inFocusable);
EsdashboardFocusable* esdashboard_focus_manager_get_focus(EsdashboardFocusManager *self);
void esdashboard_focus_manager_set_focus(EsdashboardFocusManager *self, EsdashboardFocusable *inFocusable);

EsdashboardFocusable* esdashboard_focus_manager_get_next_focusable(EsdashboardFocusManager *self,
																	EsdashboardFocusable *inBeginFocusable);
EsdashboardFocusable* esdashboard_focus_manager_get_previous_focusable(EsdashboardFocusManager *self,
																		EsdashboardFocusable *inBeginFocusable);

gboolean esdashboard_focus_manager_get_event_targets_and_action(EsdashboardFocusManager *self,
																const ClutterEvent *inEvent,
																EsdashboardFocusable *inFocusable,
																GSList **outTargets,
																const gchar **outAction);
gboolean esdashboard_focus_manager_handle_key_event(EsdashboardFocusManager *self,
													const ClutterEvent *inEvent,
													EsdashboardFocusable *inFocusable);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_FOCUS_MANAGER__ */
