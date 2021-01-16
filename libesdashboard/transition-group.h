/*
 * transition-group: A grouping transition
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

#ifndef __LIBESDASHBOARD_TRANSITION_GROUP__
#define __LIBESDASHBOARD_TRANSITION_GROUP__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/actor.h>

G_BEGIN_DECLS

/* Object declaration */
#define ESDASHBOARD_TYPE_TRANSITION_GROUP				(esdashboard_transition_group_get_type())
#define ESDASHBOARD_TRANSITION_GROUP(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_TRANSITION_GROUP, EsdashboardTransitionGroup))
#define ESDASHBOARD_IS_TRANSITION_GROUP(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_TRANSITION_GROUP))
#define ESDASHBOARD_TRANSITION_GROUP_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_TRANSITION_GROUP, EsdashboardTransitionGroupClass))
#define ESDASHBOARD_IS_TRANSITION_GROUP_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_TRANSITION_GROUP))
#define ESDASHBOARD_TRANSITION_GROUP_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_TRANSITION_GROUP, EsdashboardTransitionGroupClass))

typedef struct _EsdashboardTransitionGroup			EsdashboardTransitionGroup;
typedef struct _EsdashboardTransitionGroupClass		EsdashboardTransitionGroupClass;
typedef struct _EsdashboardTransitionGroupPrivate		EsdashboardTransitionGroupPrivate;

/**
 * EsdashboardTransitionGroup:
 *
 * The #EsdashboardTransitionGroup structure contains only private data and
 * should be accessed using the provided API
 */
 struct _EsdashboardTransitionGroup
{
	/*< private >*/
	/* Parent instance */
	ClutterTransition					parent_instance;

	/* Private structure */
	EsdashboardTransitionGroupPrivate	*priv;
};

/**
 * EsdashboardTransitionGroupClass:
 *
 * The #EsdashboardTransitionGroupClass structure contains only private data
 */
struct _EsdashboardTransitionGroupClass
{
	/*< private >*/
	/* Parent class */
	ClutterTransitionClass				parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType esdashboard_transition_group_get_type(void) G_GNUC_CONST;

ClutterTransition* esdashboard_transition_group_new(void);

void esdashboard_transition_group_add_transition(EsdashboardTransitionGroup *self,
													ClutterTransition *inTransition);
void esdashboard_transition_group_remove_transition(EsdashboardTransitionGroup *self,
													ClutterTransition *inTransition);
void esdashboard_transition_group_remove_all(EsdashboardTransitionGroup *self);

GSList* esdashboard_transition_group_get_transitions(EsdashboardTransitionGroup *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_TRANSITION_GROUP__ */
