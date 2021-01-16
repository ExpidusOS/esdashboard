/*
 * css-selector: A CSS simple selector class
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

#ifndef __LIBESDASHBOARD_CSS_SELECTOR__
#define __LIBESDASHBOARD_CSS_SELECTOR__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib-object.h>
#include <glib.h>

#include <libesdashboard/stylable.h>

G_BEGIN_DECLS

#define ESDASHBOARD_TYPE_CSS_SELECTOR				(esdashboard_css_selector_get_type())
#define ESDASHBOARD_CSS_SELECTOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_CSS_SELECTOR, EsdashboardCssSelector))
#define ESDASHBOARD_IS_CSS_SELECTOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_CSS_SELECTOR))
#define ESDASHBOARD_CSS_SELECTOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_CSS_SELECTOR, EsdashboardCssSelectorClass))
#define ESDASHBOARD_IS_CSS_SELECTOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_CSS_SELECTOR))
#define ESDASHBOARD_CSS_SELECTOR_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_CSS_SELECTOR, EsdashboardCssSelectorClass))

typedef struct _EsdashboardCssSelector				EsdashboardCssSelector; 
typedef struct _EsdashboardCssSelectorPrivate		EsdashboardCssSelectorPrivate;
typedef struct _EsdashboardCssSelectorClass			EsdashboardCssSelectorClass;

struct _EsdashboardCssSelector
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	EsdashboardCssSelectorPrivate	*priv;
};

struct _EsdashboardCssSelectorClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

typedef struct _EsdashboardCssSelectorRule			EsdashboardCssSelectorRule;

/* Public declarations */
#define ESDASHBOARD_CSS_SELECTOR_PARSE_FINISH_OK			TRUE
#define ESDASHBOARD_CSS_SELECTOR_PARSE_FINISH_BAD_STATE		FALSE

typedef gboolean (*EsdashboardCssSelectorParseFinishCallback)(EsdashboardCssSelector *inSelector,
																GScanner *inScanner,
																GTokenType inPeekNextToken,
																gpointer inUserData);

/* Public API */
GType esdashboard_css_selector_get_type(void) G_GNUC_CONST;

EsdashboardCssSelector* esdashboard_css_selector_new_from_string(const gchar *inSelector);
EsdashboardCssSelector* esdashboard_css_selector_new_from_string_with_priority(const gchar *inSelector, gint inPriority);
EsdashboardCssSelector* esdashboard_css_selector_new_from_scanner(GScanner *ioScanner,
																	EsdashboardCssSelectorParseFinishCallback inFinishCallback,
																	gpointer inUserData);
EsdashboardCssSelector* esdashboard_css_selector_new_from_scanner_with_priority(GScanner *ioScanner,
																				gint inPriority,
																				EsdashboardCssSelectorParseFinishCallback inFinishCallback,
																				gpointer inUserData);

gchar* esdashboard_css_selector_to_string(EsdashboardCssSelector *self);

gint esdashboard_css_selector_score(EsdashboardCssSelector *self, EsdashboardStylable *inStylable);

void esdashboard_css_selector_adjust_to_offset(EsdashboardCssSelector *self, gint inLine, gint inPosition);

EsdashboardCssSelectorRule* esdashboard_css_selector_get_rule(EsdashboardCssSelector *self);

const gchar* esdashboard_css_selector_rule_get_type(EsdashboardCssSelectorRule *inRule);
const gchar* esdashboard_css_selector_rule_get_id(EsdashboardCssSelectorRule *inRule);
const gchar* esdashboard_css_selector_rule_get_classes(EsdashboardCssSelectorRule *inRule);
const gchar* esdashboard_css_selector_rule_get_pseudo_classes(EsdashboardCssSelectorRule *inRule);
EsdashboardCssSelectorRule* esdashboard_css_selector_rule_get_parent(EsdashboardCssSelectorRule *inRule);
EsdashboardCssSelectorRule* esdashboard_css_selector_rule_get_ancestor(EsdashboardCssSelectorRule *inRule);
const gchar* esdashboard_css_selector_rule_get_source(EsdashboardCssSelectorRule *inRule);
gint esdashboard_css_selector_rule_get_priority(EsdashboardCssSelectorRule *inRule);
guint esdashboard_css_selector_rule_get_line(EsdashboardCssSelectorRule *inRule);
guint esdashboard_css_selector_rule_get_position(EsdashboardCssSelectorRule *inRule);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_CSS_SELECTOR__ */
