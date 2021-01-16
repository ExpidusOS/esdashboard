/*
 * textbox: An actor representing an editable or read-only text-box
 *          with optinal icons
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

#ifndef __LIBESDASHBOARD_TEXT_BOX__
#define __LIBESDASHBOARD_TEXT_BOX__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <libesdashboard/background.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_TEXT_BOX				(esdashboard_text_box_get_type())
#define ESDASHBOARD_TEXT_BOX(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_TEXT_BOX, EsdashboardTextBox))
#define ESDASHBOARD_IS_TEXT_BOX(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_TEXT_BOX))
#define ESDASHBOARD_TEXT_BOX_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_TEXT_BOX, EsdashboardTextBoxClass))
#define ESDASHBOARD_IS_TEXT_BOX_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_TEXT_BOX))
#define ESDASHBOARD_TEXT_BOX_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_TEXT_BOX, EsdashboardTextBoxClass))

typedef struct _EsdashboardTextBox				EsdashboardTextBox;
typedef struct _EsdashboardTextBoxClass			EsdashboardTextBoxClass;
typedef struct _EsdashboardTextBoxPrivate		EsdashboardTextBoxPrivate;

struct _EsdashboardTextBox
{
	/*< private >*/
	/* Parent instance */
	EsdashboardBackground			parent_instance;

	/* Private structure */
	EsdashboardTextBoxPrivate		*priv;
};

struct _EsdashboardTextBoxClass
{
	/*< private >*/
	/* Parent class */
	EsdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*text_changed)(EsdashboardTextBox *self, gchar *inText);
	
	void (*primary_icon_clicked)(EsdashboardTextBox *self);
	void (*secondary_icon_clicked)(EsdashboardTextBox *self);
};

/* Public API */
GType esdashboard_text_box_get_type(void) G_GNUC_CONST;

ClutterActor* esdashboard_text_box_new(void);

gfloat esdashboard_text_box_get_padding(EsdashboardTextBox *self);
void esdashboard_text_box_set_padding(EsdashboardTextBox *self, gfloat inPadding);

gfloat esdashboard_text_box_get_spacing(EsdashboardTextBox *self);
void esdashboard_text_box_set_spacing(EsdashboardTextBox *self, gfloat inSpacing);

gboolean esdashboard_text_box_get_editable(EsdashboardTextBox *self);
void esdashboard_text_box_set_editable(EsdashboardTextBox *self, gboolean isEditable);

gboolean esdashboard_text_box_is_empty(EsdashboardTextBox *self);
gint esdashboard_text_box_get_length(EsdashboardTextBox *self);
const gchar* esdashboard_text_box_get_text(EsdashboardTextBox *self);
void esdashboard_text_box_set_text(EsdashboardTextBox *self, const gchar *inMarkupText);
void esdashboard_text_box_set_text_va(EsdashboardTextBox *self, const gchar *inFormat, ...) G_GNUC_PRINTF(2, 3);

const gchar* esdashboard_text_box_get_text_font(EsdashboardTextBox *self);
void esdashboard_text_box_set_text_font(EsdashboardTextBox *self, const gchar *inFont);

const ClutterColor* esdashboard_text_box_get_text_color(EsdashboardTextBox *self);
void esdashboard_text_box_set_text_color(EsdashboardTextBox *self, const ClutterColor *inColor);

const ClutterColor* esdashboard_text_box_get_selection_text_color(EsdashboardTextBox *self);
void esdashboard_text_box_set_selection_text_color(EsdashboardTextBox *self, const ClutterColor *inColor);

const ClutterColor* esdashboard_text_box_get_selection_background_color(EsdashboardTextBox *self);
void esdashboard_text_box_set_selection_background_color(EsdashboardTextBox *self, const ClutterColor *inColor);

gboolean esdashboard_text_box_is_hint_text_set(EsdashboardTextBox *self);
const gchar* esdashboard_text_box_get_hint_text(EsdashboardTextBox *self);
void esdashboard_text_box_set_hint_text(EsdashboardTextBox *self, const gchar *inMarkupText);
void esdashboard_text_box_set_hint_text_va(EsdashboardTextBox *self, const gchar *inFormat, ...) G_GNUC_PRINTF(2, 3);

const gchar* esdashboard_text_box_get_hint_text_font(EsdashboardTextBox *self);
void esdashboard_text_box_set_hint_text_font(EsdashboardTextBox *self, const gchar *inFont);

const ClutterColor* esdashboard_text_box_get_hint_text_color(EsdashboardTextBox *self);
void esdashboard_text_box_set_hint_text_color(EsdashboardTextBox *self, const ClutterColor *inColor);

const gchar* esdashboard_text_box_get_primary_icon(EsdashboardTextBox *self);
void esdashboard_text_box_set_primary_icon(EsdashboardTextBox *self, const gchar *inIconName);

const gchar* esdashboard_text_box_get_secondary_icon(EsdashboardTextBox *self);
void esdashboard_text_box_set_secondary_icon(EsdashboardTextBox *self, const gchar *inIconName);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_TEXT_BOX__ */
