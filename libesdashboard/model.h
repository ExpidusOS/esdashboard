/*
 * model: A simple and generic data model holding one value per row
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

#ifndef __LIBESDASHBOARD_MODEL__
#define __LIBESDASHBOARD_MODEL__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

/* Object declaration: EsdashboardModelIter */
#define ESDASHBOARD_TYPE_MODEL_ITER				(esdashboard_model_iter_get_type())
#define ESDASHBOARD_MODEL_ITER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_MODEL_ITER, EsdashboardModelIter))
#define ESDASHBOARD_IS_MODEL_ITER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_MODEL_ITER))
#define ESDASHBOARD_MODEL_ITER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_MODEL_ITER, EsdashboardModelIterClass))
#define ESDASHBOARD_IS_MODEL_ITER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_MODEL_ITER))
#define ESDASHBOARD_MODEL_ITER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_MODEL_ITER, EsdashboardModelIterClass))

typedef struct _EsdashboardModelIter			EsdashboardModelIter;
typedef struct _EsdashboardModelIterClass		EsdashboardModelIterClass;
typedef struct _EsdashboardModelIterPrivate		EsdashboardModelIterPrivate;

struct _EsdashboardModelIter
{
	/*< private >*/
	/* Parent instance */
	GObjectClass					parent_instance;

	/* Private structure */
	EsdashboardModelIterPrivate		*priv;
};

struct _EsdashboardModelIterClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};


/* Object declaration: EsdashboardModel */
#define ESDASHBOARD_TYPE_MODEL				(esdashboard_model_get_type())
#define ESDASHBOARD_MODEL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_MODEL, EsdashboardModel))
#define ESDASHBOARD_IS_MODEL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_MODEL))
#define ESDASHBOARD_MODEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_MODEL, EsdashboardModelClass))
#define ESDASHBOARD_IS_MODEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_MODEL))
#define ESDASHBOARD_MODEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_MODEL, EsdashboardModelClass))

typedef struct _EsdashboardModel			EsdashboardModel;
typedef struct _EsdashboardModelClass		EsdashboardModelClass;
typedef struct _EsdashboardModelPrivate		EsdashboardModelPrivate;

struct _EsdashboardModel
{
	/* Parent instance */
	GObjectClass					parent_instance;

	/* Private structure */
	EsdashboardModelPrivate			*priv;
};

struct _EsdashboardModelClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*row_added)(EsdashboardModel *self,
						EsdashboardModelIter *inIter);
	void (*row_removed)(EsdashboardModel *self,
						EsdashboardModelIter *inIter);
	void (*row_changed)(EsdashboardModel *self,
						EsdashboardModelIter *inIter);

	void (*sort_changed)(EsdashboardModel *self);

	void (*filter_changed)(EsdashboardModel *self);
};


/* Public API */
typedef void (*EsdashboardModelForeachFunc)(EsdashboardModelIter *inIter,
											gpointer inData,
											gpointer inUserData);
typedef gint (*EsdashboardModelSortFunc)(EsdashboardModelIter *inLeftIter,
											EsdashboardModelIter *inRightIter,
											gpointer inUserData);
typedef gboolean (*EsdashboardModelFilterFunc)(EsdashboardModelIter *inIter,
												gpointer inUserData);

GType esdashboard_model_get_type(void) G_GNUC_CONST;
GType esdashboard_model_iter_get_type(void) G_GNUC_CONST;

/* Model creation functions */
EsdashboardModel* esdashboard_model_new(void);
EsdashboardModel* esdashboard_model_new_with_free_data(GDestroyNotify inFreeDataFunc);

/* Model information functions */
gint esdashboard_model_get_rows_count(EsdashboardModel *self);

/* Model access functions */
gpointer esdashboard_model_get(EsdashboardModel *self, gint inRow);

/* Model manipulation functions */
gboolean esdashboard_model_append(EsdashboardModel *self,
									gpointer inData,
									EsdashboardModelIter **outIter);
gboolean esdashboard_model_prepend(EsdashboardModel *self,
									gpointer inData,
									EsdashboardModelIter **outIter);
gboolean esdashboard_model_insert(EsdashboardModel *self,
									gint inRow,
									gpointer inData,
									EsdashboardModelIter **outIter);
gboolean esdashboard_model_set(EsdashboardModel *self,
								gint inRow,
								gpointer inData,
								EsdashboardModelIter **outIter);
gboolean esdashboard_model_remove(EsdashboardModel *self, gint inRow);
void esdashboard_model_remove_all(EsdashboardModel *self);

/* Model foreach functions */
void esdashboard_model_foreach(EsdashboardModel *self,
								EsdashboardModelForeachFunc inForeachCallback,
								gpointer inUserData);

/* Model sort functions */
gboolean esdashboard_model_is_sorted(EsdashboardModel *self);
void esdashboard_model_set_sort(EsdashboardModel *self,
								EsdashboardModelSortFunc inSortCallback,
								gpointer inUserData,
								GDestroyNotify inUserDataDestroyCallback);
void esdashboard_model_resort(EsdashboardModel *self);

/* Model filter functions */
gboolean esdashboard_model_is_filtered(EsdashboardModel *self);
void esdashboard_model_set_filter(EsdashboardModel *self,
									EsdashboardModelFilterFunc inFilterCallback,
									gpointer inUserData,
									GDestroyNotify inUserDataDestroyCallback);
gboolean esdashboard_model_filter_row(EsdashboardModel *self, gint inRow);


/* Model iterator functions */
EsdashboardModelIter* esdashboard_model_iter_new(EsdashboardModel *inModel);
EsdashboardModelIter* esdashboard_model_iter_new_for_row(EsdashboardModel *inModel, gint inRow);
EsdashboardModelIter* esdashboard_model_iter_copy(EsdashboardModelIter *self);
gboolean esdashboard_model_iter_next(EsdashboardModelIter *self);
gboolean esdashboard_model_iter_prev(EsdashboardModelIter *self);
gboolean esdashboard_model_iter_move_to_row(EsdashboardModelIter *self, gint inRow);

/* Model iterator information functions */
EsdashboardModel* esdashboard_model_iter_get_model(EsdashboardModelIter *self);
guint esdashboard_model_iter_get_row(EsdashboardModelIter *self);

/* Model iterator access functions */
gpointer esdashboard_model_iter_get(EsdashboardModelIter *self);

/* Model iterator manipulation functions */
gboolean esdashboard_model_iter_set(EsdashboardModelIter *self, gpointer inData);
gboolean esdashboard_model_iter_remove(EsdashboardModelIter *self);

/* Model filter functions */
gboolean esdashboard_model_iter_filter(EsdashboardModelIter *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_MODEL__ */
