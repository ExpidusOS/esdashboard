/*
 * label: An actor representing a label and an icon (both optional)
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

#ifndef __LIBESDASHBOARD_LABEL__
#define __LIBESDASHBOARD_LABEL__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libesdashboard/background.h>
#include <libesdashboard/types.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * EsdashboardLabelStyle:
 * @ESDASHBOARD_LABEL_STYLE_TEXT: The actor will show only text labels.
 * @ESDASHBOARD_LABEL_STYLE_ICON: The actor will show only icons.
 * @ESDASHBOARD_LABEL_STYLE_BOTH: The actor will show both, text labels and icons.
 *
 * Determines the style of an actor, e.g. text labels and icons at labels.
 */
typedef enum /*< prefix=ESDASHBOARD_LABEL_STYLE >*/
{
	ESDASHBOARD_LABEL_STYLE_TEXT=0,
	ESDASHBOARD_LABEL_STYLE_ICON,
	ESDASHBOARD_LABEL_STYLE_BOTH
} EsdashboardLabelStyle;


/* Object declaration */
#define ESDASHBOARD_TYPE_LABEL					(esdashboard_label_get_type())
#define ESDASHBOARD_LABEL(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_LABEL, EsdashboardLabel))
#define ESDASHBOARD_IS_LABEL(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_LABEL))
#define ESDASHBOARD_LABEL_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_LABEL, EsdashboardLabelClass))
#define ESDASHBOARD_IS_LABEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_LABEL))
#define ESDASHBOARD_LABEL_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_LABEL, EsdashboardLabelClass))

typedef struct _EsdashboardLabel				EsdashboardLabel;
typedef struct _EsdashboardLabelClass			EsdashboardLabelClass;
typedef struct _EsdashboardLabelPrivate			EsdashboardLabelPrivate;

struct _EsdashboardLabel
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground		parent_instance;

	/* Private structure */
	EsdashboardLabelPrivate	*priv;
};

struct _EsdashboardLabelClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass	parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(EsdashboardLabel *self);
};

/* Public API */
GType esdashboard_label_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_label_new(void);
ClutterActor* esdashboard_label_new_with_text(const gchar *inText);
ClutterActor* esdashboard_label_new_with_icon_name(const gchar *inIconName);
ClutterActor* esdashboard_label_new_with_gicon(GIcon *inIcon);
ClutterActor* esdashboard_label_new_full_with_icon_name(const gchar *inIconName, const gchar *inText);
ClutterActor* esdashboard_label_new_full_with_gicon(GIcon *inIcon, const gchar *inText);

/* General functions */
gfloat esdashboard_label_get_padding(EsdashboardLabel *self);
void esdashboard_label_set_padding(EsdashboardLabel *self, const gfloat inPadding);

gfloat esdashboard_label_get_spacing(EsdashboardLabel *self);
void esdashboard_label_set_spacing(EsdashboardLabel *self, const gfloat inSpacing);

EsdashboardLabelStyle esdashboard_label_get_style(EsdashboardLabel *self);
void esdashboard_label_set_style(EsdashboardLabel *self, const EsdashboardLabelStyle inStyle);

/* Icon functions */
const gchar* esdashboard_label_get_icon_name(EsdashboardLabel *self);
void esdashboard_label_set_icon_name(EsdashboardLabel *self, const gchar *inIconName);

GIcon* esdashboard_label_get_gicon(EsdashboardLabel *self);
void esdashboard_label_set_gicon(EsdashboardLabel *self, GIcon *inIcon);

ClutterImage* esdashboard_label_get_icon_image(EsdashboardLabel *self);
void esdashboard_label_set_icon_image(EsdashboardLabel *self, ClutterImage *inIconImage);

gint esdashboard_label_get_icon_size(EsdashboardLabel *self);
void esdashboard_label_set_icon_size(EsdashboardLabel *self, gint inSize);

gboolean esdashboard_label_get_sync_icon_size(EsdashboardLabel *self);
void esdashboard_label_set_sync_icon_size(EsdashboardLabel *self, gboolean inSync);

EsdashboardOrientation esdashboard_label_get_icon_orientation(EsdashboardLabel *self);
void esdashboard_label_set_icon_orientation(EsdashboardLabel *self, const EsdashboardOrientation inOrientation);

/* Label functions */
const gchar* esdashboard_label_get_text(EsdashboardLabel *self);
void esdashboard_label_set_text(EsdashboardLabel *self, const gchar *inMarkupText);

const gchar* esdashboard_label_get_font(EsdashboardLabel *self);
void esdashboard_label_set_font(EsdashboardLabel *self, const gchar *inFont);

const ClutterColor* esdashboard_label_get_color(EsdashboardLabel *self);
void esdashboard_label_set_color(EsdashboardLabel *self, const ClutterColor *inColor);

PangoEllipsizeMode esdashboard_label_get_ellipsize_mode(EsdashboardLabel *self);
void esdashboard_label_set_ellipsize_mode(EsdashboardLabel *self, const PangoEllipsizeMode inMode);

gboolean esdashboard_label_get_single_line_mode(EsdashboardLabel *self);
void esdashboard_label_set_single_line_mode(EsdashboardLabel *self, const gboolean inSingleLine);

PangoAlignment esdashboard_label_get_text_justification(EsdashboardLabel *self);
void esdashboard_label_set_text_justification(EsdashboardLabel *self, const PangoAlignment inJustification);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_LABEL__ */
