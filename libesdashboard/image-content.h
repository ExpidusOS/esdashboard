/*
 * image-content: An asynchronous loaded and cached image content
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

#ifndef __LIBESDASHBOARD_IMAGE_CONTENT__
#define __LIBESDASHBOARD_IMAGE_CONTENT__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_IMAGE_CONTENT				(esdashboard_image_content_get_type())
#define ESDASHBOARD_IMAGE_CONTENT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_IMAGE_CONTENT, EsdashboardImageContent))
#define ESDASHBOARD_IS_IMAGE_CONTENT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_IMAGE_CONTENT))
#define ESDASHBOARD_IMAGE_CONTENT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_IMAGE_CONTENT, EsdashboardImageContentClass))
#define ESDASHBOARD_IS_IMAGE_CONTENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_IMAGE_CONTENT))
#define ESDASHBOARD_IMAGE_CONTENT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_IMAGE_CONTENT, EsdashboardImageContentClass))

typedef struct _EsdashboardImageContent				EsdashboardImageContent;
typedef struct _EsdashboardImageContentClass		EsdashboardImageContentClass;
typedef struct _EsdashboardImageContentPrivate		EsdashboardImageContentPrivate;

struct _EsdashboardImageContent
{
	/*< private >*/
	/* Parent instance */
	ClutterImage						parent_instance;

	/* Private structure */
	EsdashboardImageContentPrivate		*priv;
};

struct _EsdashboardImageContentClass
{
	/*< private >*/
	/* Parent class */
	ClutterImageClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*loaded)(EsdashboardImageContent *self);
	void (*loading_failed)(EsdashboardImageContent *self);
};

/* Public API */
typedef enum /*< prefix=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE >*/
{
	ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_NONE=0,
	ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADING,
	ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_SUCCESSFULLY,
	ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_FAILED
} EsdashboardImageContentLoadingState;

GType esdashboard_image_content_get_type(void) G_GNUC_CONST;

ClutterContent* esdashboard_image_content_new_for_icon_name(const gchar *inIconName, gint inSize);
ClutterContent* esdashboard_image_content_new_for_gicon(GIcon *inIcon, gint inSize);
ClutterContent* esdashboard_image_content_new_for_pixbuf(GdkPixbuf *inPixbuf);

const gchar* esdashboard_image_content_get_missing_icon_name(EsdashboardImageContent *self);
void esdashboard_image_content_set_missing_icon_name(EsdashboardImageContent *self, const gchar *inMissingIconName);

EsdashboardImageContentLoadingState esdashboard_image_content_get_state(EsdashboardImageContent *self);

void esdashboard_image_content_force_load(EsdashboardImageContent *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_IMAGE_CONTENT__ */
