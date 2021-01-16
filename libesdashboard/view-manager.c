/*
 * view-manager: Single-instance managing views
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/view-manager.h>

#include <glib/gi18n-lib.h>

#include <libesdashboard/view.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
struct _EsdashboardViewManagerPrivate
{
	/* Instance related */
	GList		*registeredViews;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardViewManager,
							esdashboard_view_manager,
							G_TYPE_OBJECT)

/* Signals */
enum
{
	SIGNAL_REGISTERED,
	SIGNAL_UNREGISTERED,

	SIGNAL_LAST
};

static guint EsdashboardViewManagerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
typedef struct _EsdashboardViewManagerData		EsdashboardViewManagerData;
struct _EsdashboardViewManagerData
{
	gchar		*ID;
	GType		gtype;
};

/* Single instance of view manager */
static EsdashboardViewManager*		_esdashboard_view_manager=NULL;

/* Free an registered view entry */
static void _esdashboard_view_manager_entry_free(EsdashboardViewManagerData *inData)
{
	g_return_if_fail(inData);

	/* Release allocated resources */
	if(inData->ID) g_free(inData->ID);
	g_free(inData);
}

/* Create an entry for a registered view */
static EsdashboardViewManagerData* _esdashboard_view_manager_entry_new(const gchar *inID, GType inType)
{
	EsdashboardViewManagerData		*data;

	g_return_val_if_fail(inID && *inID, NULL);

	/* Create new entry */
	data=g_new0(EsdashboardViewManagerData, 1);
	if(!data) return(NULL);

	data->ID=g_strdup(inID);
	data->gtype=inType;

	/* Return newly created entry */
	return(data);
}

/* Find entry for a registered view by ID */
static GList* _esdashboard_view_manager_entry_find_list_entry_by_id(EsdashboardViewManager *self,
																	const gchar *inID)
{
	EsdashboardViewManagerPrivate	*priv;
	GList							*iter;
	EsdashboardViewManagerData		*data;

	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_MANAGER(self), NULL);
	g_return_val_if_fail(inID && *inID, NULL);

	priv=self->priv;

	/* Iterate through list and lookup list entry whose data has requested ID */
	for(iter=priv->registeredViews; iter; iter=g_list_next(iter))
	{
		/* Get data of currently iterated list entry */
		data=(EsdashboardViewManagerData*)(iter->data);
		if(!data) continue;

		/* Check if ID of data matches requested one and
		 * return list entry if it does.
		 */
		if(g_strcmp0(data->ID, inID)==0) return(iter);
	}

	/* If we get here we did not find a matching list entry */
	return(NULL);
}

static EsdashboardViewManagerData* _esdashboard_view_manager_entry_find_data_by_id(EsdashboardViewManager *self,
																					const gchar *inID)
{
	GList							*iter;
	EsdashboardViewManagerData		*data;

	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_MANAGER(self), NULL);
	g_return_val_if_fail(inID && *inID, NULL);

	/* Find list entry matching requested ID */
	iter=_esdashboard_view_manager_entry_find_list_entry_by_id(self, inID);
	if(!iter) return(NULL);

	/* We found a matching list entry so return its data */
	data=(EsdashboardViewManagerData*)(iter->data);

	/* Return data of matching list entry */
	return(data);
}

/* IMPLEMENTATION: GObject */

/* Construct this object */
static GObject* _esdashboard_view_manager_constructor(GType inType,
														guint inNumberConstructParams,
														GObjectConstructParam *inConstructParams)
{
	GObject									*object;

	if(!_esdashboard_view_manager)
	{
		object=G_OBJECT_CLASS(esdashboard_view_manager_parent_class)->constructor(inType, inNumberConstructParams, inConstructParams);
		_esdashboard_view_manager=ESDASHBOARD_VIEW_MANAGER(object);
	}
		else
		{
			object=g_object_ref(G_OBJECT(_esdashboard_view_manager));
		}

	return(object);
}

/* Dispose this object */
static void _esdashboard_view_manager_dispose_unregister_view(gpointer inData, gpointer inUserData)
{
	EsdashboardViewManagerData		*data;

	g_return_if_fail(ESDASHBOARD_IS_VIEW_MANAGER(inUserData));

	data=(EsdashboardViewManagerData*)inData;
	esdashboard_view_manager_unregister(ESDASHBOARD_VIEW_MANAGER(inUserData), data->ID);
}

static void _esdashboard_view_manager_dispose(GObject *inObject)
{
	EsdashboardViewManager			*self=ESDASHBOARD_VIEW_MANAGER(inObject);
	EsdashboardViewManagerPrivate	*priv=self->priv;

	/* Release allocated resouces */
	if(priv->registeredViews)
	{
		g_list_foreach(priv->registeredViews, _esdashboard_view_manager_dispose_unregister_view, self);
		g_list_free(priv->registeredViews);
		priv->registeredViews=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_view_manager_parent_class)->dispose(inObject);
}

/* Finalize this object */
static void _esdashboard_view_manager_finalize(GObject *inObject)
{
	/* Release allocated resources finally, e.g. unset singleton */
	if(G_LIKELY(G_OBJECT(_esdashboard_view_manager)==inObject))
	{
		_esdashboard_view_manager=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_view_manager_parent_class)->finalize(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void esdashboard_view_manager_class_init(EsdashboardViewManagerClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->constructor=_esdashboard_view_manager_constructor;
	gobjectClass->dispose=_esdashboard_view_manager_dispose;
	gobjectClass->finalize=_esdashboard_view_manager_finalize;

	/* Define signals */
	EsdashboardViewManagerSignals[SIGNAL_REGISTERED]=
		g_signal_new("registered",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardViewManagerClass, registered),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_GTYPE);

	EsdashboardViewManagerSignals[SIGNAL_UNREGISTERED]=
		g_signal_new("unregistered",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardViewManagerClass, unregistered),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_GTYPE);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_view_manager_init(EsdashboardViewManager *self)
{
	EsdashboardViewManagerPrivate	*priv;

	priv=self->priv=esdashboard_view_manager_get_instance_private(self);

	/* Set default values */
	priv->registeredViews=NULL;
}

/* IMPLEMENTATION: Public API */

/* Get single instance of manager */
EsdashboardViewManager* esdashboard_view_manager_get_default(void)
{
	GObject									*singleton;

	singleton=g_object_new(ESDASHBOARD_TYPE_VIEW_MANAGER, NULL);
	return(ESDASHBOARD_VIEW_MANAGER(singleton));
}

/* Register a view */
gboolean esdashboard_view_manager_register(EsdashboardViewManager *self, const gchar *inID, GType inViewType)
{
	EsdashboardViewManagerPrivate		*priv;
	EsdashboardViewManagerData			*data;

	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_MANAGER(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	priv=self->priv;

	/* Check if given type is not a EsdashboardView but a derived type from it */
	if(inViewType==ESDASHBOARD_TYPE_VIEW ||
		g_type_is_a(inViewType, ESDASHBOARD_TYPE_VIEW)!=TRUE)
	{
		g_warning("View %s of type %s is not a %s and cannot be registered",
					inID,
					g_type_name(inViewType),
					g_type_name(ESDASHBOARD_TYPE_VIEW));
		return(FALSE);
	}

	/* Check if view is registered already */
	if(_esdashboard_view_manager_entry_find_list_entry_by_id(self, inID))
	{
		g_warning("View %s of type %s is registered already",
					inID,
					g_type_name(inViewType));
		return(FALSE);
	}

	/* Register view */
	ESDASHBOARD_DEBUG(self, MISC,
						"Registering view %s of type %s",
						inID,
						g_type_name(inViewType));

	data=_esdashboard_view_manager_entry_new(inID, inViewType);
	if(!data)
	{
		g_warning("Failed to register view %s of type %s",
					inID,
					g_type_name(inViewType));
		return(FALSE);
	}

	priv->registeredViews=g_list_append(priv->registeredViews, data);
	g_signal_emit(self, EsdashboardViewManagerSignals[SIGNAL_REGISTERED], 0, data->ID);

	/* View was registered successfully so return TRUE here */
	return(TRUE);
}

/* Unregister a view */
gboolean esdashboard_view_manager_unregister(EsdashboardViewManager *self, const gchar *inID)
{
	EsdashboardViewManagerPrivate		*priv;
	GList								*iter;
	EsdashboardViewManagerData			*data;

	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_MANAGER(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	priv=self->priv;

	/* Check if view is registered  */
	iter=_esdashboard_view_manager_entry_find_list_entry_by_id(self, inID);
	if(!iter)
	{
		g_warning("View %s is not registered and cannot be unregistered", inID);
		return(FALSE);
	}

	/* Get data from found list entry */
	data=(EsdashboardViewManagerData*)(iter->data);

	/* Remove from list of registered views */
	ESDASHBOARD_DEBUG(self, MISC,
						"Unregistering view %s of type %s",
						data->ID,
						g_type_name(data->gtype));

	priv->registeredViews=g_list_remove_link(priv->registeredViews, iter);
	g_signal_emit(self, EsdashboardViewManagerSignals[SIGNAL_UNREGISTERED], 0, data->ID);

	/* Free data entry and list element at iterator */
	_esdashboard_view_manager_entry_free(data);
	g_list_free(iter);

	/* View was unregistered successfully so return TRUE here */
	return(TRUE);
}

/* Get list of registered views types.
 * Returned GList must be freed with g_list_free_full(result, g_free) by caller.
 */
GList* esdashboard_view_manager_get_registered(EsdashboardViewManager *self)
{
	GList						*copy;
	GList						*iter;
	EsdashboardViewManagerData	*data;

	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_MANAGER(self), NULL);

	/* Return a copy of all IDs stored in list of registered view types */
	copy=NULL;
	for(iter=self->priv->registeredViews; iter; iter=g_list_next(iter))
	{
		data=(EsdashboardViewManagerData*)(iter->data);

		copy=g_list_prepend(copy, g_strdup(data->ID));
	}

	/* Restore order in copied list to match origin */
	copy=g_list_reverse(copy);

	/* Return copied list of IDs of registered views */
	return(copy);
}

/* Check if a view for requested ID is registered */
gboolean esdashboard_view_manager_has_registered_id(EsdashboardViewManager *self, const gchar *inID)
{
	GList							*iter;

	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_MANAGER(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	/* Check if view is registered by getting pointer to list element
	 * in list of registered views.
	 */
	iter=_esdashboard_view_manager_entry_find_list_entry_by_id(self, inID);
	if(iter) return(TRUE);

	/* If we get here we did not find a view for requested ID */
	return(FALSE);
}

/* Create view for requested ID */
GObject* esdashboard_view_manager_create_view(EsdashboardViewManager *self, const gchar *inID)
{
	EsdashboardViewManagerData			*data;
	GObject								*view;

	g_return_val_if_fail(ESDASHBOARD_IS_VIEW_MANAGER(self), NULL);
	g_return_val_if_fail(inID && *inID, NULL);

	/* Check if view is registered and get its data */
	data=_esdashboard_view_manager_entry_find_data_by_id(self, inID);
	if(!data)
	{
		g_warning("Cannot create view %s because it is not registered", inID);
		return(NULL);
	}

	/* Create view */
	view=g_object_new(data->gtype, "view-id", data->ID, NULL);

	/* Return newly created view */
	return(view);
}
