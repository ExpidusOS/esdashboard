/*
 * bindings: Customizable keyboard and pointer bindings for focusable actors
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

#ifndef __LIBESDASHBOARD_BINDINGS_POOL__
#define __LIBESDASHBOARD_BINDINGS_POOL__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/binding.h>
#include <libesdashboard/types.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_BINDINGS_POOL				(esdashboard_bindings_pool_get_type())
#define ESDASHBOARD_BINDINGS_POOL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_BINDINGS_POOL, EsdashboardBindingsPool))
#define ESDASHBOARD_IS_BINDINGS_POOL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_BINDINGS_POOL))
#define ESDASHBOARD_BINDINGS_POOL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_BINDINGS_POOL, EsdashboardBindingsPoolClass))
#define ESDASHBOARD_IS_BINDINGS_POOL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_BINDINGS_POOL))
#define ESDASHBOARD_BINDINGS_POOL_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_BINDINGS_POOL, EsdashboardBindingsPoolClass))

typedef struct _EsdashboardBindingsPool				EsdashboardBindingsPool;
typedef struct _EsdashboardBindingsPoolClass		EsdashboardBindingsPoolClass;
typedef struct _EsdashboardBindingsPoolPrivate		EsdashboardBindingsPoolPrivate;

struct _EsdashboardBindingsPool
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	EsdashboardBindingsPoolPrivate		*priv;
};

struct _EsdashboardBindingsPoolClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;
};

/* Errors */
#define ESDASHBOARD_BINDINGS_POOL_ERROR					(esdashboard_bindings_pool_error_quark())

GQuark esdashboard_bindings_pool_error_quark(void);

typedef enum /*< prefix=ESDASHBOARD_BINDINGS_POOL_ERROR >*/
{
	ESDASHBOARD_BINDINGS_POOL_ERROR_FILE_NOT_FOUND,
	ESDASHBOARD_BINDINGS_POOL_ERROR_PARSER_INTERNAL_ERROR,
	ESDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
	ESDASHBOARD_BINDINGS_POOL_ERROR_INTERNAL_ERROR
} EsdashboardBindingsPoolErrorEnum;

/* Public API */
GType esdashboard_bindings_pool_get_type(void) G_GNUC_CONST;

EsdashboardBindingsPool* esdashboard_bindings_pool_get_default(void);

gboolean esdashboard_bindings_pool_load(EsdashboardBindingsPool *self, GError **outError);

const EsdashboardBinding* esdashboard_bindings_pool_find_for_event(EsdashboardBindingsPool *self, ClutterActor *inActor, const ClutterEvent *inEvent);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_BINDINGS_POOL__ */
