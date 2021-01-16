/*
 * tooltip-action: An action to display a tooltip after a short timeout
 *                 without movement at the referred actor
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

#ifndef __LIBESDASHBOARD_TOOLTIP_ACTION__
#define __LIBESDASHBOARD_TOOLTIP_ACTION__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_TOOLTIP_ACTION				(esdashboard_tooltip_action_get_type ())
#define ESDASHBOARD_TOOLTIP_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), ESDASHBOARD_TYPE_TOOLTIP_ACTION, EsdashboardTooltipAction))
#define ESDASHBOARD_IS_TOOLTIP_ACTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), ESDASHBOARD_TYPE_TOOLTIP_ACTION))
#define ESDASHBOARD_TOOLTIP_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), ESDASHBOARD_TYPE_TOOLTIP_ACTION, EsdashboardTooltipActionClass))
#define ESDASHBOARD_IS_TOOLTIP_ACTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), ESDASHBOARD_TYPE_TOOLTIP_ACTION))
#define ESDASHBOARD_TOOLTIP_ACTION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), ESDASHBOARD_TYPE_TOOLTIP_ACTION, EsdashboardTooltipActionClass))

typedef struct _EsdashboardTooltipAction			EsdashboardTooltipAction;
typedef struct _EsdashboardTooltipActionPrivate		EsdashboardTooltipActionPrivate;
typedef struct _EsdashboardTooltipActionClass		EsdashboardTooltipActionClass;

struct _EsdashboardTooltipAction
{
	/*< private >*/
	/* Parent instance */
	ClutterAction						parent_instance;

	/* Private structure */
	EsdashboardTooltipActionPrivate		*priv;
};

struct _EsdashboardTooltipActionClass
{
	/*< private >*/
	/* Parent class */
	ClutterActionClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*activating)(EsdashboardTooltipAction *self);
};

/* Public API */
GType esdashboard_tooltip_action_get_type(void) G_GNUC_CONST;

ClutterAction* esdashboard_tooltip_action_new(void);

const gchar* esdashboard_tooltip_action_get_text(EsdashboardTooltipAction *self);
void esdashboard_tooltip_action_set_text(EsdashboardTooltipAction *self, const gchar *inTooltipText);

void esdashboard_tooltip_action_get_position(EsdashboardTooltipAction *self, gfloat *outX, gfloat *outY);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_TOOLTIP_ACTION__ */
