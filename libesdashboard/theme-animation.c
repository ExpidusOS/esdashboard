/*
 * theme-animation: A theme used for building animations by XML files
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

#include <libesdashboard/theme-animation.h>

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>
#include <errno.h>

#include <libesdashboard/transition-group.h>
#include <libesdashboard/stylable.h>
#include <libesdashboard/application.h>
#include <libesdashboard/enums.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
struct _EsdashboardThemeAnimationPrivate
{
	/* Instance related */
	GSList			*specs;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardThemeAnimation,
							esdashboard_theme_animation,
							G_TYPE_OBJECT)


/* IMPLEMENTATION: Private variables and methods */
#define ENABLE_ANIMATIONS_ESCONF_PROP		"/enable-animations"
#define DEFAULT_ENABLE_ANIMATIONS			TRUE

enum
{
	TAG_DOCUMENT,
	TAG_ANIMATIONS,
	TAG_TRIGGER,
	TAG_TIMELINE,
	TAG_APPLY,
	TAG_PROPERTY
};

enum
{
	ESDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER=0,
	ESDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_STAGE=1
};

typedef struct _EsdashboardThemeAnimationSpec				EsdashboardThemeAnimationSpec;
struct _EsdashboardThemeAnimationSpec
{
	gint					refCount;
	gchar					*id;
	EsdashboardCssSelector	*senderSelector;
	gchar					*signal;
	GSList					*targets;
};

typedef struct _EsdashboardThemeAnimationTargets			EsdashboardThemeAnimationTargets;
struct _EsdashboardThemeAnimationTargets
{
	gint					refCount;
	EsdashboardCssSelector	*targetSelector;
	guint					origin;
	ClutterTimeline			*timeline;
	GSList					*properties;
};

typedef struct _EsdashboardThemeAnimationTargetsProperty	EsdashboardThemeAnimationTargetsProperty;
struct _EsdashboardThemeAnimationTargetsProperty
{
	gint					refCount;
	gchar					*name;
	GValue					from;
	GValue					to;
};

typedef struct _EsdashboardThemeAnimationParserData			EsdashboardThemeAnimationParserData;
struct _EsdashboardThemeAnimationParserData
{
	EsdashboardThemeAnimation			*self;

	GSList								*specs;

	EsdashboardThemeAnimationSpec		*currentSpec;
	ClutterTimeline						*currentTimeline;
	EsdashboardThemeAnimationTargets	*currentTargets;

	gint								lastLine;
	gint								lastPosition;
	gint								currentLine;
	gint								currentPostition;
};


/* Create, destroy, ref and unref animation targets data */
static EsdashboardThemeAnimationTargetsProperty* _esdashboard_theme_animation_targets_property_new(const gchar *inPropertyName,
																									const gchar *inPropertyFrom,
																									const gchar *inPropertyTo)
{
	EsdashboardThemeAnimationTargetsProperty		*data;

	g_return_val_if_fail(inPropertyName && *inPropertyName, NULL);
	g_return_val_if_fail(!inPropertyFrom || *inPropertyFrom, NULL);
	g_return_val_if_fail(!inPropertyTo || *inPropertyTo, NULL);

	/* Create animation targets property data */
	data=g_new0(EsdashboardThemeAnimationTargetsProperty, 1);
	if(!data)
	{
		g_critical("Cannot allocate memory for animation targets property '%s'", inPropertyName);
		return(NULL);
	}

	data->refCount=1;
	data->name=g_strdup(inPropertyName);

	if(inPropertyFrom)
	{
		g_value_init(&data->from, G_TYPE_STRING);
		g_value_set_string(&data->from, inPropertyFrom);
	}
	
	if(inPropertyTo)
	{
		g_value_init(&data->to, G_TYPE_STRING);
		g_value_set_string(&data->to, inPropertyTo);
	}

	return(data);
}

static void _esdashboard_theme_animation_targets_property_free(EsdashboardThemeAnimationTargetsProperty *inData)
{
	g_return_if_fail(inData);

	/* Free property data */
	if(inData->name) g_free(inData->name);
	g_value_unset(&inData->from);
	g_value_unset(&inData->to);
	g_free(inData);
}

static EsdashboardThemeAnimationTargetsProperty* _esdashboard_theme_animation_targets_property_ref(EsdashboardThemeAnimationTargetsProperty *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _esdashboard_theme_animation_targets_property_unref(EsdashboardThemeAnimationTargetsProperty *inData)
{
	g_return_if_fail(inData);

	if(inData->refCount==1) _esdashboard_theme_animation_targets_property_free(inData);
		else inData->refCount--;
}

/* Create, destroy, ref and unref animation targets data */
static EsdashboardThemeAnimationTargets* _esdashboard_theme_animation_targets_new(EsdashboardCssSelector *inTargetSelector,
																					gint inOrigin,
																					ClutterTimeline *inTimelineSource)
{
	EsdashboardThemeAnimationTargets		*data;

	g_return_val_if_fail(!inTargetSelector || ESDASHBOARD_IS_CSS_SELECTOR(inTargetSelector), NULL);
	g_return_val_if_fail(inOrigin>=ESDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER && inOrigin<=ESDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_STAGE, NULL);
	g_return_val_if_fail(CLUTTER_IS_TIMELINE(inTimelineSource), NULL);

	/* Create animation targets data */
	data=g_new0(EsdashboardThemeAnimationTargets, 1);
	if(!data)
	{
		gchar							*selector;

		selector=esdashboard_css_selector_to_string(inTargetSelector);
		g_critical("Cannot allocate memory for animation targets data with selector '%s'", selector);
		g_free(selector);

		return(NULL);
	}

	data->refCount=1;
	data->targetSelector=(inTargetSelector ? g_object_ref(inTargetSelector) : NULL);
	data->origin=inOrigin;
	data->timeline=g_object_ref(inTimelineSource);
	data->properties=NULL;

	return(data);
}

static void _esdashboard_theme_animation_targets_free(EsdashboardThemeAnimationTargets *inData)
{
	g_return_if_fail(inData);

#ifdef DEBUG
	if(inData->refCount>1)
	{
		g_critical("Freeing animation targets data at %p with a reference counter of %d greater than one",
					inData,
					inData->refCount);
	}
#endif

	/* Release allocated resources */
	if(inData->targetSelector) g_object_unref(inData->targetSelector);
	if(inData->timeline) g_object_unref(inData->timeline);
	if(inData->properties) g_slist_free_full(inData->properties, (GDestroyNotify)_esdashboard_theme_animation_targets_property_unref);
	g_free(inData);
}

static EsdashboardThemeAnimationTargets* _esdashboard_theme_animation_targets_ref(EsdashboardThemeAnimationTargets *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _esdashboard_theme_animation_targets_unref(EsdashboardThemeAnimationTargets *inData)
{
	g_return_if_fail(inData);

	if(inData->refCount==1) _esdashboard_theme_animation_targets_free(inData);
		else inData->refCount--;
}

/* Add property name and from-to values to animation targets data */
static void _esdashboard_theme_animation_targets_add_property(EsdashboardThemeAnimationTargets *inData,
																EsdashboardThemeAnimationTargetsProperty *inProperty)
{
	g_return_if_fail(inData);
	g_return_if_fail(inProperty);

	/* Add property to animation targets data */
	inData->properties=g_slist_prepend(inData->properties, _esdashboard_theme_animation_targets_property_ref(inProperty));
}


/* Create, destroy, ref and unref animation specification data */
static EsdashboardThemeAnimationSpec* _esdashboard_theme_animation_spec_new(const gchar *inID,
																			EsdashboardCssSelector *inSenderSelector,
																			const gchar *inSignal)
{
	EsdashboardThemeAnimationSpec		*data;

	g_return_val_if_fail(inID && *inID, NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_CSS_SELECTOR(inSenderSelector), NULL);
	g_return_val_if_fail(inSignal && *inSignal, NULL);

	/* Create animation specification data */
	data=g_new0(EsdashboardThemeAnimationSpec, 1);
	if(!data)
	{
		gchar							*selector;

		selector=esdashboard_css_selector_to_string(inSenderSelector);
		g_critical("Cannot allocate memory for animation specification data with sender '%s' and signal '%s'",
					selector,
					inSignal);
		g_free(selector);

		return(NULL);
	}

	data->refCount=1;
	data->id=g_strdup(inID);
	data->senderSelector=g_object_ref(inSenderSelector);
	data->signal=g_strdup(inSignal);
	data->targets=NULL;

	return(data);
}

static void _esdashboard_theme_animation_spec_free(EsdashboardThemeAnimationSpec *inData)
{
	g_return_if_fail(inData);

#ifdef DEBUG
	if(inData->refCount>1)
	{
		g_critical("Freeing animation specification data at %p with a reference counter of %d greater than one",
					inData,
					inData->refCount);
	}
#endif

	/* Release allocated resources */
	if(inData->id) g_free(inData->id);
	if(inData->senderSelector) g_object_unref(inData->senderSelector);
	if(inData->signal) g_free(inData->signal);
	if(inData->targets) g_slist_free_full(inData->targets, (GDestroyNotify)_esdashboard_theme_animation_targets_unref);
	g_free(inData);
}

static EsdashboardThemeAnimationSpec* _esdashboard_theme_animation_spec_ref(EsdashboardThemeAnimationSpec *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _esdashboard_theme_animation_spec_unref(EsdashboardThemeAnimationSpec *inData)
{
	g_return_if_fail(inData);

	if(inData->refCount==1) _esdashboard_theme_animation_spec_free(inData);
		else inData->refCount--;
}

/* Add a animation target to animation specification */
static void _esdashboard_theme_animation_spec_add_targets(EsdashboardThemeAnimationSpec *inData,
															EsdashboardThemeAnimationTargets *inTargets)
{
	g_return_if_fail(inData);
	g_return_if_fail(inTargets);

	/* Add target to specification */
	inData->targets=g_slist_prepend(inData->targets, _esdashboard_theme_animation_targets_ref(inTargets));
}

/* Find best matching animation specification for sender and signal */
static EsdashboardThemeAnimationSpec* _esdashboard_theme_animation_find_matching_animation_spec(EsdashboardThemeAnimation *self,
																										EsdashboardStylable *inSender,
																										const gchar *inSignal)
{
	EsdashboardThemeAnimationPrivate			*priv;
	GSList										*iter;
	gint										score;
	EsdashboardThemeAnimationSpec				*spec;
	gint										bestScore;
	EsdashboardThemeAnimationSpec				*bestAnimation;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_STYLABLE(inSender), NULL);
	g_return_val_if_fail(inSignal && *inSignal, NULL);

	priv=self->priv;
	bestScore=0;
	bestAnimation=NULL;

	/* Iterate through all animation specification and get its score against sender.
	 * If the iterated specification gets a higher score than the previous one,
	 * remember this specification for return value.
	 */
	for(iter=priv->specs; iter; iter=g_slist_next(iter))
	{
		/* Get currently iterated specification */
		spec=(EsdashboardThemeAnimationSpec*)iter->data;
		if(!spec) continue;

		/* Skip animation specification if its signal definition does not match
		 * the emitted signal.
		 */
		if(g_strcmp0(spec->signal, inSignal)!=0) continue;

		/* Get score of currently iterated specification against sender.
		 * Skip this iterated specification if score is zero or lower.
		 */
		score=esdashboard_css_selector_score(spec->senderSelector, inSender);
		if(score<=0) continue;

		/* If score is higher than the previous best matching one, then release
		 * old specificationr remembered and ref new one.
		 */
		if(score>bestScore)
		{
			/* Release old remembered specification */
			if(bestAnimation) _esdashboard_theme_animation_spec_unref(bestAnimation);

			/* Remember new one and take a reference */
			bestScore=score;
			bestAnimation=_esdashboard_theme_animation_spec_ref(spec);
		}
	}

	/* Return best matching animation specification found */
	return(bestAnimation);
}

/* Lookup animation specification for requested ID */
static EsdashboardThemeAnimationSpec* _esdashboard_theme_animation_find_animation_spec_by_id(EsdashboardThemeAnimation *self,
																									const gchar *inID)
{
	EsdashboardThemeAnimationPrivate			*priv;
	GSList										*iter;
	EsdashboardThemeAnimationSpec				*spec;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(inID && *inID, NULL);

	priv=self->priv;

	/* Iterate through all animation specification and check if it matches
	 * the requested ID.
	 */
	for(iter=priv->specs; iter; iter=g_slist_next(iter))
	{
		/* Get currently iterated specification */
		spec=(EsdashboardThemeAnimationSpec*)iter->data;
		if(!spec) continue;

		/* Return animation specification if ID matches requested one */
		if(g_strcmp0(spec->id, inID)==0)
		{
			_esdashboard_theme_animation_spec_ref(spec);
			return(spec);
		}
	}

	/* If we get here no animation specification with requested ID was found */
	return(NULL);
}

/* Find actors matching an animation target data */
static gboolean _esdashboard_theme_animation_find_actors_for_animation_targets_traverse_callback(ClutterActor *inActor, gpointer inUserData)
{
	GSList		**actors=(GSList**)inUserData;

	*actors=g_slist_prepend(*actors, inActor);

	return(ESDASHBOARD_TRAVERSAL_CONTINUE);
}

static GSList* _esdashboard_theme_animation_find_actors_for_animation_targets(EsdashboardThemeAnimation *self,
																				EsdashboardThemeAnimationTargets *inTargetSpec,
																				ClutterActor *inSender)
{
	GSList									*actors;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(inTargetSpec, NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSender), NULL);

	/* Traverse through actors beginning at the root node and collect each actor
	 * matching the target selector but only if a target selector is set. If
	 * target selector is NULL then set up a single-item list containing only
	 * the sender actor as list of actors found.
	 */
	actors=NULL;
	if(inTargetSpec->targetSelector)
	{
		ClutterActor						*rootNode;

		/* Depending on origin at animation target data select root node to start
		 * traversal and collecting matching actors.
		 */
		rootNode=NULL;
		if(inTargetSpec->origin==ESDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER)
		{
			rootNode=inSender;
		}

		esdashboard_traverse_actor(rootNode,
									inTargetSpec->targetSelector,
									_esdashboard_theme_animation_find_actors_for_animation_targets_traverse_callback,
									&actors);
	}
		else
		{
			actors=g_slist_prepend(actors, inSender);
		}

	/* Correct order in list */
	actors=g_slist_reverse(actors);

	/* Return list of actors found */
	return(actors);
}

/* Callback to add each successfully parsed animation specifications to list of
 * known animations of this theme.
 */
static void _esdashboard_theme_animation_ref_and_add_to_theme(gpointer inData, gpointer inUserData)
{
	EsdashboardThemeAnimation				*self;
	EsdashboardThemeAnimationPrivate		*priv;
	EsdashboardThemeAnimationSpec			*spec;

	g_return_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(inUserData));
	g_return_if_fail(inData);

	self=ESDASHBOARD_THEME_ANIMATION(inUserData);
	priv=self->priv;
	spec=(EsdashboardThemeAnimationSpec*)inData;

	/* Increase reference of specified animation specification and add to list
	 * of known ones.
	 */
	priv->specs=g_slist_prepend(priv->specs, _esdashboard_theme_animation_spec_ref(spec));
}

/* Check if an animation specification with requested ID exists */
static gboolean _esdashboard_theme_animation_has_id(EsdashboardThemeAnimation *self,
													EsdashboardThemeAnimationParserData *inParserData,
													const gchar *inID)
{
	EsdashboardThemeAnimationPrivate		*priv;
	GSList									*ids;
	gboolean								hasID;
	EsdashboardThemeAnimationSpec			*spec;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), TRUE);

	priv=self->priv;
	hasID=FALSE;

	/* Check that ID to lookup is specified */
	g_assert(inID && *inID);

	/* Lookup ID first in currently parsed file if specified */
	if(inParserData)
	{
		for(ids=inParserData->specs; !hasID && ids; ids=g_slist_next(ids))
		{
			spec=(EsdashboardThemeAnimationSpec*)ids->data;
			if(spec && g_strcmp0(spec->id, inID)==0) hasID=TRUE;
		}
	}

	/* If ID was not found in currently parsed effects xml file (if specified)
	 * continue search in already parsed and known effects.
	 */
	if(!hasID)
	{
		for(ids=priv->specs; !hasID && ids; ids=g_slist_next(ids))
		{
			spec=(EsdashboardThemeAnimationSpec*)ids->data;
			if(spec && g_strcmp0(spec->id, inID)==0) hasID=TRUE;
		}
	}

	/* Return lookup result */
	return(hasID);
}

/* Convert string to integer and throw error if conversion failed */
static gboolean _esdashboard_theme_animation_string_to_gint(const gchar *inNumberString, gint *outNumber, GError **outError)
{
	gint64			convertedNumber;
	gchar			*outNumberStringEnd;

	g_return_val_if_fail(inNumberString && *inNumberString, FALSE);
	g_return_val_if_fail(outNumber, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	/* Try to convert string to number */
	convertedNumber=g_ascii_strtoll(inNumberString, &outNumberStringEnd, 10);

	/* Check if invalid base was specified */
	if(errno==EINVAL)
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_THEME_ANIMATION_ERROR,
					ESDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					"Invalid base for conversion");
		return(FALSE);
	}

	/* Check if integer is out of range */
	if(errno==ERANGE)
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_THEME_ANIMATION_ERROR,
					ESDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					"Integer out of range");
		return(FALSE);
	}

	/* If converted integer resulted in zero check if end pointer
	 * has moved and does not match start pointer and points to a
	 * NULL byte (as NULL-terminated strings must be provided).
	 */
	if(convertedNumber==0 &&
		(outNumberStringEnd==inNumberString || *outNumberStringEnd!=0))
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_THEME_ANIMATION_ERROR,
					ESDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					"Cannot convert string '%s' to integer",
					inNumberString);
		return(FALSE);
	}

	/* Set converted number - the integer */
	*outNumber=((gint)convertedNumber);

	/* Return TRUE for successful conversion */
	return(TRUE);
}

/* Helper function to set up GError object in this parser */
static void _esdashboard_theme_animation_parse_set_error(EsdashboardThemeAnimationParserData *inParserData,
															GMarkupParseContext *inContext,
															GError **outError,
															EsdashboardThemeAnimationErrorEnum inCode,
															const gchar *inFormat,
															...)
{
	GError		*tempError;
	gchar		*message;
	va_list		args;

	/* Get error message */
	va_start(args, inFormat);
	message=g_strdup_vprintf(inFormat, args);
	va_end(args);

	/* Create error object */
	tempError=g_error_new_literal(ESDASHBOARD_THEME_ANIMATION_ERROR, inCode, message);
	if(inParserData)
	{
		g_prefix_error(&tempError,
						"Error on line %d char %d: ",
						inParserData->lastLine,
						inParserData->lastPosition);
	}

	/* Set error */
	g_propagate_error(outError, tempError);

	/* Release allocated resources */
	g_free(message);
}

/* General callbacks which can be used for any tag */
static void _esdashboard_theme_animation_parse_general_no_text_nodes(GMarkupParseContext *inContext,
																		const gchar *inText,
																		gsize inTextLength,
																		gpointer inUserData,
																		GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;
	gchar										*realText;

	/* Check if text contains only whitespace. If we find any non-whitespace
	 * in text then set error.
	 */
	realText=g_strstrip(g_strdup(inText));
	if(*realText)
	{
		const GSList	*parents;

		parents=g_markup_parse_context_get_element_stack(inContext);
		if(parents) parents=g_slist_next(parents);

		_esdashboard_theme_animation_parse_set_error(data,
														inContext,
														outError,
														ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
														"Unexpected text node '%s' at tag <%s>",
														realText,
														parents ? (gchar*)parents->data : "document");
	}
	g_free(realText);
}

/* Determine tag name and ID */
static gint _esdashboard_theme_animation_get_tag_by_name(const gchar *inTag)
{
	g_return_val_if_fail(inTag && *inTag, -1);

	/* Compare string and return type ID */
	if(g_strcmp0(inTag, "animations")==0) return(TAG_ANIMATIONS);
	if(g_strcmp0(inTag, "trigger")==0) return(TAG_TRIGGER);
	if(g_strcmp0(inTag, "timeline")==0) return(TAG_TIMELINE);
	if(g_strcmp0(inTag, "apply")==0) return(TAG_APPLY);
	if(g_strcmp0(inTag, "property")==0) return(TAG_PROPERTY);

	/* If we get here we do not know tag name and return invalid ID */
	return(-1);
}

static const gchar* _esdashboard_theme_animation_get_tag_by_id(guint inTagType)
{
	/* Compare ID and return string */
	switch(inTagType)
	{
		case TAG_DOCUMENT:
			return("document");

		case TAG_ANIMATIONS:
			return("animations");

		case TAG_TRIGGER:
			return("trigger");

		case TAG_TIMELINE:
			return("timeline");

		case TAG_APPLY:
			return("apply");

		case TAG_PROPERTY:
			return("property");

		default:
			break;
	}

	/* If we get here we do not know tag name and return NULL */
	return(NULL);
}

/* Parser callbacks for <property> node */
static void _esdashboard_theme_animation_parse_property_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_PROPERTY;
	gint										nextTag;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_esdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Unknown tag <%s>",
													inElementName);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Tag <%s> cannot contain tag <%s>",
													_esdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

/* Parser callbacks for <apply> node */
static void _esdashboard_theme_animation_parse_apply_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_APPLY;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_esdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Unknown tag <%s>",
													inElementName);
		return;
	}

	/* Check if element name is <property> and follows expected parent tags:
	 * <apply>
	 */
	if(nextTag==TAG_PROPERTY)
	{
		static GMarkupParser						propertyParser=
													{
														_esdashboard_theme_animation_parse_property_start,
														NULL,
														_esdashboard_theme_animation_parse_general_no_text_nodes,
														NULL,
														NULL,
													};

		const gchar									*propertyName=NULL;
		const gchar									*propertyFrom=NULL;
		const gchar									*propertyTo=NULL;
		EsdashboardThemeAnimationTargetsProperty	*property=NULL;

		g_assert(data->currentTargets!=NULL);

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRING,
											"name",
											&propertyName,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"from",
											&propertyFrom,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"to",
											&propertyTo,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Check tag's attributes */
		if(strlen(propertyName)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Empty 'name' at tag '%s'",
															inElementName);
			return;
		}

		if(propertyFrom && strlen(propertyFrom)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Empty 'from' at tag '%s'",
															inElementName);
			return;
		}

		if(propertyTo && strlen(propertyTo)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Empty 'to' at tag '%s'",
															inElementName);
			return;
		}

		/* Create new animation property and add to current targets */
		property=_esdashboard_theme_animation_targets_property_new(propertyName, propertyFrom, propertyTo);
		_esdashboard_theme_animation_targets_add_property(data->currentTargets, property);
		_esdashboard_theme_animation_targets_property_unref(property);

		/* Set up context for tag <apply> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Tag <%s> cannot contain tag <%s>",
													_esdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _esdashboard_theme_animation_parse_apply_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for <timeline> node */
static void _esdashboard_theme_animation_parse_timeline_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_TIMELINE;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_esdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Unknown tag <%s>",
													inElementName);
		return;
	}

	/* Check if element name is <apply> and follows expected parent tags:
	 * <timeline>
	 */
	if(nextTag==TAG_APPLY)
	{
		static GMarkupParser					propertyParser=
												{
													_esdashboard_theme_animation_parse_apply_start,
													_esdashboard_theme_animation_parse_apply_end,
													_esdashboard_theme_animation_parse_general_no_text_nodes,
													NULL,
													NULL,
												};

		const gchar								*applyToText=NULL;
		EsdashboardCssSelector					*applyTo=NULL;
		const gchar								*applyOriginText=NULL;
		gint									applyOrigin;

		g_assert(data->currentTargets==NULL);

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"to",
											&applyToText,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"origin",
											&applyOriginText,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Check tag's attributes */
		if(applyToText && strlen(applyToText)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Empty 'to' at tag '%s'",
															inElementName);
			return;
		}

		if(applyOriginText && strlen(applyOriginText)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Empty 'origin' at tag '%s'",
															inElementName);
			return;
		}

		/* Convert tag's attributes' value to usable values */
		applyOrigin=ESDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER;
		if(applyOriginText)
		{
			if(g_strcmp0(applyOriginText, "sender")==0) applyOrigin=ESDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER;
				else if(g_strcmp0(applyOriginText, "stage")==0) applyOrigin=ESDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_STAGE;
				else
				{
					_esdashboard_theme_animation_parse_set_error(data,
																	inContext,
																	outError,
																	ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
																	"Invalid value '%s' for 'origin' at tag '%s'",
																	applyOriginText,
																	inElementName);
					return;
				}
		}

		/* Create new animation timeline with timeline data */
		applyTo=NULL;
		if(applyToText)
		{
			applyTo=esdashboard_css_selector_new_from_string(applyToText);
		}

		data->currentTargets=_esdashboard_theme_animation_targets_new(applyTo, applyOrigin, data->currentTimeline);

		if(applyTo) g_object_unref(applyTo);

		/* Set up context for tag <apply> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Tag <%s> cannot contain tag <%s>",
													_esdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _esdashboard_theme_animation_parse_timeline_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;

	g_assert(data->currentSpec);
	g_assert(data->currentTargets);

	/* Add targets to animation specification */
	_esdashboard_theme_animation_spec_add_targets(data->currentSpec, data->currentTargets);
	_esdashboard_theme_animation_targets_unref(data->currentTargets);
	data->currentTargets=NULL;

	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for <trigger> node */
static void _esdashboard_theme_animation_parse_trigger_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_TRIGGER;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_esdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Unknown tag <%s>",
													inElementName);
		return;
	}

	/* Check if element name is <timeline> and follows expected parent tags:
	 * <trigger>
	 */
	if(nextTag==TAG_TIMELINE)
	{
		static GMarkupParser					propertyParser=
												{
													_esdashboard_theme_animation_parse_timeline_start,
													_esdashboard_theme_animation_parse_timeline_end,
													_esdashboard_theme_animation_parse_general_no_text_nodes,
													NULL,
													NULL,
												};

		const gchar								*timelineDelayText=NULL;
		const gchar								*timelineDurationText=NULL;
		const gchar								*timelineModeText=NULL;
		const gchar								*timelineRepeatText=NULL;
		gint									timelineDelay;
		gint									timelineDuration;
		gint									timelineMode;
		gint									timelineRepeat;

		g_assert(data->currentTimeline==NULL);

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRING,
											"delay",
											&timelineDelayText,
											G_MARKUP_COLLECT_STRING,
											"duration",
											&timelineDurationText,
											G_MARKUP_COLLECT_STRING,
											"mode",
											&timelineModeText,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"repeat",
											&timelineRepeatText,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Check tag's attributes */
		if(!timelineDelayText || strlen(timelineDelayText)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Missing or empty delay at tag '%s'",
															inElementName);
			return;
		}

		if(!timelineDurationText || strlen(timelineDurationText)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Missing or empty duration at tag '%s'",
															inElementName);
			return;
		}

		if(!timelineModeText || strlen(timelineModeText)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Missing or empty mode at tag '%s'",
															inElementName);
			return;
		}

		if(timelineRepeatText && strlen(timelineRepeatText)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Empty repeat at tag '%s'",
															inElementName);
			return;
		}

		/* Convert tag's attributes' value to usable values */
		if(!_esdashboard_theme_animation_string_to_gint(timelineDelayText, &timelineDelay, &error))
		{
			g_propagate_error(outError, error);
			return;
		}

		if(!_esdashboard_theme_animation_string_to_gint(timelineDurationText, &timelineDuration, &error))
		{
			g_propagate_error(outError, error);
			return;
		}

		timelineMode=esdashboard_get_enum_value_from_nickname(CLUTTER_TYPE_ANIMATION_MODE, timelineModeText);
		if(timelineMode==G_MININT)
		{
			/* Set error */
			g_set_error(outError,
						ESDASHBOARD_THEME_ANIMATION_ERROR,
						ESDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
						"Invalid mode '%s'",
						timelineModeText);
			return;
		}

		timelineRepeat=0;
		if(timelineRepeatText &&
			!_esdashboard_theme_animation_string_to_gint(timelineRepeatText, &timelineRepeat, &error))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Create new animation timeline with timeline data */
		data->currentTimeline=clutter_timeline_new(timelineDuration);
		clutter_timeline_set_delay(data->currentTimeline, timelineDelay);
		clutter_timeline_set_progress_mode(data->currentTimeline, timelineMode);
		clutter_timeline_set_repeat_count(data->currentTimeline, timelineRepeat);

		/* Set up context for tag <timeline> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Tag <%s> cannot contain tag <%s>",
													_esdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _esdashboard_theme_animation_parse_trigger_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;

	g_assert(data->currentTimeline);

	/* Add targets to animation specification */
	g_object_unref(data->currentTimeline);
	data->currentTimeline=NULL;

	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for <animations> node */
static void _esdashboard_theme_animation_parse_animations_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_ANIMATIONS;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_esdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Unknown tag <%s>",
													inElementName);
		return;
	}

	/* Check if element name is <trigger> and follows expected parent tags:
	 * <animations>
	 */
	if(nextTag==TAG_TRIGGER)
	{
		static GMarkupParser					propertyParser=
												{
													_esdashboard_theme_animation_parse_trigger_start,
													_esdashboard_theme_animation_parse_trigger_end,
													_esdashboard_theme_animation_parse_general_no_text_nodes,
													NULL,
													NULL,
												};

		const gchar								*triggerID=NULL;
		const gchar								*triggerSender=NULL;
		const gchar								*triggerSignal=NULL;
		EsdashboardCssSelector					*selector=NULL;

		g_assert(data->currentSpec==NULL);

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRING,
											"id",
											&triggerID,
											G_MARKUP_COLLECT_STRING,
											"sender",
											&triggerSender,
											G_MARKUP_COLLECT_STRING,
											"signal",
											&triggerSignal,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Check tag's attributes */
		if(!triggerID || strlen(triggerID)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Missing or empty ID at tag '%s'",
															inElementName);
			return;
		}

		if(!triggerSender || strlen(triggerSender)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Missing or empty sender at tag '%s'",
															inElementName);
			return;
		}

		if(!triggerSignal || strlen(triggerSignal)==0)
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Missing or empty signal at tag '%s'",
															inElementName);
			return;
		}

		if(!esdashboard_is_valid_id(triggerID))
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Invalid ID '%s' at tag '%s'",
															triggerID,
															inElementName);
			return;
		}

		if(_esdashboard_theme_animation_has_id(data->self, data, triggerID))
		{
			_esdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															"Multiple definition of trigger with ID '%s'",
															triggerID);
			return;
		}

		/* Create new animation specification with trigger data */
		selector=esdashboard_css_selector_new_from_string(triggerSender);
		data->currentSpec=_esdashboard_theme_animation_spec_new(triggerID, selector, triggerSignal);
		g_object_unref(selector);

		/* Set up context for tag <trigger> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Tag <%s> cannot contain tag <%s>",
													_esdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _esdashboard_theme_animation_parse_animations_end(GMarkupParseContext *inContext,
																const gchar *inElementName,
																gpointer inUserData,
																GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;

	g_assert(data->currentSpec);

	/* Add animation specification to list of animations */
	data->specs=g_slist_prepend(data->specs, data->currentSpec);
	data->currentSpec=NULL;

	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for document root node */
static void _esdashboard_theme_animation_parse_document_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	EsdashboardThemeAnimationParserData			*data=(EsdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_DOCUMENT;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_esdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Unknown tag <%s>",
													inElementName);
		return;
	}

	/* Check if element name is <animations> and follows expected parent tags:
	 * <document>
	 */
	if(nextTag==TAG_ANIMATIONS)
	{
		static GMarkupParser					propertyParser=
												{
													_esdashboard_theme_animation_parse_animations_start,
													_esdashboard_theme_animation_parse_animations_end,
													_esdashboard_theme_animation_parse_general_no_text_nodes,
													NULL,
													NULL,
												};

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_INVALID,
											NULL))
		{
			g_propagate_error(outError, error);
		}

		/* Set up context for tag <animations> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_esdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													"Tag <%s> cannot contain tag <%s>",
													_esdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _esdashboard_theme_animation_parse_document_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parse XML from string */
static gboolean _esdashboard_theme_animation_parse_xml(EsdashboardThemeAnimation *self,
														const gchar *inPath,
														const gchar *inContents,
														GError **outError)
{
	static GMarkupParser					parser=
											{
												_esdashboard_theme_animation_parse_document_start,
												_esdashboard_theme_animation_parse_document_end,
												_esdashboard_theme_animation_parse_general_no_text_nodes,
												NULL,
												NULL,
											};

	EsdashboardThemeAnimationParserData		*data;
	GMarkupParseContext						*context;
	GError									*error;
	gboolean								success;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), FALSE);
	g_return_val_if_fail(inPath && *inPath, FALSE);
	g_return_val_if_fail(inContents && *inContents, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	error=NULL;
	success=TRUE;

	/* Create and set up parser instance */
	data=g_new0(EsdashboardThemeAnimationParserData, 1);
	if(!data)
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_THEME_ANIMATION_ERROR,
					ESDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					"Could not set up parser data for file %s",
					inPath);
		return(FALSE);
	}

	context=g_markup_parse_context_new(&parser, 0, data, NULL);
	if(!context)
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_THEME_ANIMATION_ERROR,
					ESDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					"Could not create parser for file %s",
					inPath);

		g_free(data);
		return(FALSE);
	}

	/* Now the parser and its context is set up and we can now
	 * safely initialize data.
	 */
	data->self=self;
	data->specs=NULL;
	data->currentSpec=NULL;
	data->currentTimeline=NULL;
	data->currentTargets=NULL;
	data->lastLine=1;
	data->lastPosition=1;
	data->currentLine=1;
	data->currentPostition=1;

	/* Parse XML string */
	if(success && !g_markup_parse_context_parse(context, inContents, -1, &error))
	{
		g_propagate_error(outError, error);
		success=FALSE;
	}

	if(success && !g_markup_parse_context_end_parse(context, &error))
	{
		g_propagate_error(outError, error);
		success=FALSE;
	}

	/* Handle collected data if parsing was successful */
	if(success)
	{
		g_slist_foreach(data->specs, (GFunc)_esdashboard_theme_animation_ref_and_add_to_theme, self);
	}

	/* Clean up resources */
#ifdef DEBUG
	if(!success)
	{
		// TODO: g_slist_foreach(data->specs, (GFunc)_esdashboard_theme_animation_print_parsed_objects, "Animation specs (this file):");
		// TODO: g_slist_foreach(self->priv->specs, (GFunc)_esdashboard_theme_animation_print_parsed_objects, "Animation specs (parsed before):");
		ESDASHBOARD_DEBUG(self, THEME,
							"PARSER ERROR: %s",
							(outError && *outError) ? (*outError)->message : "unknown error");
	}
#endif

	g_markup_parse_context_free(context);

	g_slist_free_full(data->specs, (GDestroyNotify)_esdashboard_theme_animation_spec_unref);
	g_free(data);

	return(success);
}

/* Find matching entry in list of provided default value and set up value if found */
static gboolean _esdashboard_theme_animation_find_default_property_value(EsdashboardThemeAnimation *self,
																			EsdashboardAnimationValue **inDefaultValues,
																			EsdashboardActor *inSender,
																			const gchar *inProperty,
																			ClutterActor *inActor,
																			GValue *ioValue)
{
	EsdashboardAnimationValue			*foundValue;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTOR(inSender), FALSE);
	g_return_val_if_fail(inProperty && *inProperty, FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), FALSE);
	g_return_val_if_fail(ioValue && G_VALUE_TYPE(ioValue)!=G_TYPE_INVALID, FALSE);

	foundValue=NULL;

	/* Skip if actor is not stylable and cannot be matched againt the selectors in values list */
	if(!ESDASHBOARD_IS_STYLABLE(inActor))
	{
		ESDASHBOARD_DEBUG(self, ANIMATION,
							"Actor %s@%p is not stylable and cannot match any selector in list of default values so fail",
							G_OBJECT_TYPE_NAME(inActor),
							inActor);

		/* Unset value to prevent using it as it could not be converted */
		g_value_unset(ioValue);

		return(FALSE);
	}

	/* Iterate through list of values and find best matching entry for given actor */
	if(inDefaultValues)
	{
		EsdashboardAnimationValue		**iter;
		gfloat							iterScore;
		gfloat							foundScore;

		for(iter=inDefaultValues; *iter; iter++)
		{
			/* Check if currently iterated entry in list of value matches the actor */
			if((*iter)->selector)
			{
				iterScore=esdashboard_css_selector_score((*iter)->selector, ESDASHBOARD_STYLABLE(inActor));
				if(iterScore<0) continue;

				/* Check match is the best one with a higher sore as the previous one (if available) */
				if(foundValue && iterScore<=foundScore) continue;
			}
				else if((gpointer)inActor!=(gpointer)inSender) continue;

			/* Check if entry matches the requested property */
			if(g_strcmp0((*iter)->property, inProperty)!=0) continue;

			/* Remember this match as best one */
			foundValue=*iter;
			foundScore=iterScore;
		}
	}

	/* If no match was found, return FALSE here */
	if(!foundValue) return(FALSE);

	/* Set up value as a match was found */
	if(!g_value_transform(foundValue->value, ioValue))
	{
		g_warning("Could not transform default value of for property '%s' of %s from type %s to %s of class %s",
					foundValue->property,
					G_OBJECT_TYPE_NAME(inActor),
					G_VALUE_TYPE_NAME(foundValue->value),
					G_VALUE_TYPE_NAME(ioValue),
					G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(inActor)));

		/* Unset value to prevent using it as it could not be converted */
		g_value_unset(ioValue);

		return(FALSE);
	}

	/* If we get here, we found a match and could convert the value */
	return(TRUE);
}

/* Create animation by specification */
static void _esdashboard_theme_animation_free_animation_actor_map_hashtable(gpointer inKey, gpointer inValue, gpointer inUserData)
{
	g_return_if_fail(inKey);

	g_slist_free(inKey);
}

static EsdashboardAnimation* _esdashboard_theme_animation_create_by_spec(EsdashboardThemeAnimation *self,
																			EsdashboardThemeAnimationSpec *inSpec,
																			EsdashboardActor *inSender,
																			EsdashboardAnimationValue **inDefaultInitialValues,
																			EsdashboardAnimationValue **inDefaultFinalValues)
{
	EsdashboardAnimation									*animation;
	EsdashboardAnimationClass								*animationClass;
	GSList													*iterTargets;
	gint													counterTargets;
	GHashTable												*animationActorMap;
#ifdef DEBUG
	gboolean												doDebug=FALSE;
#endif

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTOR(inSender), NULL);
	g_return_val_if_fail(inSpec, NULL);

	animation=NULL;
	animationActorMap=NULL;

	/* Create animation for animation specification */
	animation=g_object_new(ESDASHBOARD_TYPE_ANIMATION,
							"id", inSpec->id,
							NULL);
	if(!animation)
	{
		g_critical("Cannot allocate memory for animation '%s'",
					inSpec->id);
		return(NULL);
	}

	animationClass=ESDASHBOARD_ANIMATION_GET_CLASS(animation);
	if(!animationClass->add_animation)
	{
		g_warning("Will not be able to add animations to actors as object of type %s does not implement required virtual function EsdashboardAnimation::%s",
					G_OBJECT_TYPE_NAME(self),
					"add_animation");
	}

	/* Create actor-animation-list-mapping via a hash-table */
	animationActorMap=g_hash_table_new(g_direct_hash, g_direct_equal);

	/* Iterate through animation targets of animation specification and create
	 * property transition for each target and property found.
	 */
	for(counterTargets=0, iterTargets=inSpec->targets; iterTargets; iterTargets=g_slist_next(iterTargets), counterTargets++)
	{
		EsdashboardThemeAnimationTargets					*targets;
		GSList												*actors;
		GSList												*iterActors;
		gint												counterActors;

		/* Get currently iterate animation targets */
		targets=(EsdashboardThemeAnimationTargets*)iterTargets->data;
		if(!targets) continue;

		/* Find targets to apply property transitions to */
		actors=_esdashboard_theme_animation_find_actors_for_animation_targets(self, targets, CLUTTER_ACTOR(inSender));
		if(!actors) continue;
		ESDASHBOARD_DEBUG(self, ANIMATION,
							"Target #%d of animation specification '%s' applies to %d actors",
							counterTargets,
							inSpec->id,
							g_slist_length(actors));

		/* Iterate through actor, create a transition group to collect
		 * property transitions at, create a property transition for each
		 * property specified in animation target, determine "from" value
		 * where missing, add property transition to transition group and
		 * finally add transition group with currently iterated actor to
		 * animation object.
		 */
		for(counterActors=0, iterActors=actors; iterActors; iterActors=g_slist_next(iterActors), counterActors++)
		{
			GSList											*iterProperties;
			ClutterActor									*actor;
			int												counterProperties;

			/* Get actor */
			actor=(ClutterActor*)iterActors->data;
			if(!actor) continue;

			/* Iterate through properties and create a property transition
			 * with cloned timeline from animation target. Determine "from"
			 * value if missing in animation targets property specification
			 * and add the property transition to transition group.
			 */
			for(counterProperties=0, iterProperties=targets->properties; iterProperties; iterProperties=g_slist_next(iterProperties), counterProperties++)
			{
				EsdashboardThemeAnimationTargetsProperty	*propertyTargetSpec;
				GParamSpec									*propertySpec;
				GValue										fromValue=G_VALUE_INIT;
				GValue										toValue=G_VALUE_INIT;

				/* Get target's property data to animate */
				propertyTargetSpec=(EsdashboardThemeAnimationTargetsProperty*)iterProperties->data;
				if(!propertyTargetSpec) continue;

				/* Check if actor has property to animate */
				propertySpec=g_object_class_find_property(G_OBJECT_GET_CLASS(actor), propertyTargetSpec->name);
				if(!propertySpec && CLUTTER_IS_ANIMATABLE(actor))
				{
					propertySpec=clutter_animatable_find_property(CLUTTER_ANIMATABLE(actor), propertyTargetSpec->name);
				}

				if(!propertySpec)
				{
					g_warning("Cannot create animation '%s' for non-existing property '%s' at actor of type '%s'",
								inSpec->id,
								propertyTargetSpec->name,
								G_OBJECT_TYPE_NAME(actor));

					/* Skip this property as it does not exist at actor */
					continue;
				}

				/* If no "from" value is set at target's property data, get
				 * current value. Otherwise convert "from" value from target's
				 * property data to expected type of property.
				 */
				if(G_VALUE_TYPE(&propertyTargetSpec->from)!=G_TYPE_INVALID)
				{
					g_value_init(&fromValue, G_PARAM_SPEC_VALUE_TYPE(propertySpec));
					if(!g_value_transform(&propertyTargetSpec->from, &fromValue))
					{
						g_warning("Could not transform 'from'-value of '%s' for property '%s' to type %s of class %s",
									g_value_get_string(&propertyTargetSpec->from),
									propertyTargetSpec->name,
									g_type_name(G_PARAM_SPEC_VALUE_TYPE(propertySpec)),
									G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(actor)));

						/* Unset "from" value to skip it, means it will no transition will be
						 * create and it will not be added to transition group.
						 */
						g_value_unset(&fromValue);
					}
#ifdef DEBUG
					if(doDebug)
					{
						gchar	*valueString;

						valueString=g_strdup_value_contents(&propertyTargetSpec->from);
						ESDASHBOARD_DEBUG(self, ANIMATION,
											"%s 'from'-value %s of type %s for property '%s' to type %s of class %s for target #%d and actor #%d (%s@%p) of animation specification '%s'",
											(G_VALUE_TYPE(&fromValue)!=G_TYPE_INVALID) ? "Converted" : "Could not convert",
											valueString,
											G_VALUE_TYPE_NAME(&propertyTargetSpec->from),
											propertyTargetSpec->name,
											g_type_name(G_PARAM_SPEC_VALUE_TYPE(propertySpec)),
											G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(actor)),
											counterTargets,
											counterActors,
											G_OBJECT_TYPE_NAME(actor),
											actor,
											inSpec->id);
						g_free(valueString);
					}
#endif
				}
					else
					{
						g_value_init(&fromValue, G_PARAM_SPEC_VALUE_TYPE(propertySpec));
						if(inDefaultInitialValues && _esdashboard_theme_animation_find_default_property_value(self, inDefaultInitialValues, inSender, propertyTargetSpec->name, actor, &fromValue))
						{
#ifdef DEBUG
							if(doDebug)
							{
								gchar	*valueString;

								valueString=g_strdup_value_contents(&fromValue);
								ESDASHBOARD_DEBUG(self, ANIMATION,
													"Using provided default 'from'-value %s for property '%s' from target #%d and actor #%d (%s@%p) of animation specification '%s' as no 'from' value was specified",
													valueString,
													propertyTargetSpec->name,
													counterTargets,
													counterActors,
													G_OBJECT_TYPE_NAME(actor),
													actor,
													inSpec->id);
								g_free(valueString);
							}
#endif
						}
							else
							{
								g_object_get_property(G_OBJECT(actor),
														propertyTargetSpec->name,
														&fromValue);
#ifdef DEBUG
								if(doDebug)
								{
									gchar	*valueString;

									valueString=g_strdup_value_contents(&fromValue);
									ESDASHBOARD_DEBUG(self, ANIMATION,
														"Fetching current 'from'-value %s for property '%s' from target #%d and actor #%d (%s@%p) of animation specification '%s' as no 'from' value was specified",
														valueString,
														propertyTargetSpec->name,
														counterTargets,
														counterActors,
														G_OBJECT_TYPE_NAME(actor),
														actor,
														inSpec->id);
									g_free(valueString);
								}
#endif
							}
					}

				/* If "to" value is set at target's property data, convert it
				 * from target's property data to expected type of property.
				 */
				if(G_VALUE_TYPE(&propertyTargetSpec->to)!=G_TYPE_INVALID)
				{
					g_value_init(&toValue, G_PARAM_SPEC_VALUE_TYPE(propertySpec));
					if(!g_value_transform(&propertyTargetSpec->to, &toValue))
					{
						g_warning("Could not transform 'to'-value of '%s' for property '%s' to type %s of class %s",
									g_value_get_string(&propertyTargetSpec->to),
									propertyTargetSpec->name,
									g_type_name(G_PARAM_SPEC_VALUE_TYPE(propertySpec)),
									G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(actor)));

						/* Unset "to" value to prevent setting it at transition.
						 * The animation will set a value when starting the
						 * animation.
						 */
						g_value_unset(&toValue);
					}
#ifdef DEBUG
					if(doDebug)
					{
						gchar	*valueString;

						valueString=g_strdup_value_contents(&propertyTargetSpec->to);
						ESDASHBOARD_DEBUG(self, ANIMATION,
											"%s 'to'-value %s of type %s for property '%s' to type %s of class %s for target #%d and actor #%d (%s@%p) of animation specification '%s'",
											(G_VALUE_TYPE(&toValue)!=G_TYPE_INVALID) ? "Converted" : "Could not convert",
											valueString,
											G_VALUE_TYPE_NAME(&propertyTargetSpec->to),
											propertyTargetSpec->name,
											g_type_name(G_PARAM_SPEC_VALUE_TYPE(propertySpec)),
											G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(actor)),
											counterTargets,
											counterActors,
											G_OBJECT_TYPE_NAME(actor),
											actor,
											inSpec->id);
						g_free(valueString);
					}
#endif
				}
					else
					{
						g_value_init(&toValue, G_PARAM_SPEC_VALUE_TYPE(propertySpec));
						if(inDefaultFinalValues && _esdashboard_theme_animation_find_default_property_value(self, inDefaultFinalValues, inSender, propertyTargetSpec->name, actor, &toValue))
						{
#ifdef DEBUG
							if(doDebug)
							{
								gchar	*valueString;

								valueString=g_strdup_value_contents(&toValue);
								ESDASHBOARD_DEBUG(self, ANIMATION,
													"Using provided default 'to'-value %s for property '%s' from target #%d and actor #%d (%s@%p) of animation specification '%s' as no 'to' value was specified",
													valueString,
													propertyTargetSpec->name,
													counterTargets,
													counterActors,
													G_OBJECT_TYPE_NAME(actor),
													actor,
													inSpec->id);
								g_free(valueString);
							}
#endif
						}
					}

				/* Create property transition for property with cloned timeline
				 * and add this new transition to transition group if from value
				 * is not invalid.
				 */
				if(G_VALUE_TYPE(&fromValue)!=G_TYPE_INVALID)
				{
					ClutterTransition						*propertyTransition;
					GSList									*animationList;

					/* Create property transition */
					propertyTransition=clutter_property_transition_new(propertyTargetSpec->name);
					if(!propertyTransition)
					{
						g_critical("Cannot allocate memory for transition of property '%s' of animation specification '%s'",
									propertyTargetSpec->name,
									inSpec->id);

						/* Release allocated resources */
						g_value_unset(&fromValue);
						g_value_unset(&toValue);

						/* Skip this property transition */
						continue;
					}

					/* Clone timeline configuration from animation target */
					clutter_timeline_set_duration(CLUTTER_TIMELINE(propertyTransition), clutter_timeline_get_duration(targets->timeline));
					clutter_timeline_set_delay(CLUTTER_TIMELINE(propertyTransition), clutter_timeline_get_delay(targets->timeline));
					clutter_timeline_set_progress_mode(CLUTTER_TIMELINE(propertyTransition), clutter_timeline_get_progress_mode(targets->timeline));
					clutter_timeline_set_repeat_count(CLUTTER_TIMELINE(propertyTransition), clutter_timeline_get_repeat_count(targets->timeline));

					/* Set "from" value */
					clutter_transition_set_from_value(propertyTransition, &fromValue);

					/* Set "to" value if valid */
					if(G_VALUE_TYPE(&toValue)!=G_TYPE_INVALID)
					{
						clutter_transition_set_to_value(propertyTransition, &toValue);
					}

					/* Add animation to list of animations of target actor */
					animationList=g_hash_table_lookup(animationActorMap, actor);
					animationList=g_slist_prepend(animationList, propertyTransition);
					g_hash_table_insert(animationActorMap, actor, animationList);

					ESDASHBOARD_DEBUG(self, ANIMATION,
										"Created transition for property '%s' at target #%d and actor #%d (%s@%p) of animation specification '%s'",
										propertyTargetSpec->name,
										counterTargets,
										counterActors,
										G_OBJECT_TYPE_NAME(actor),
										actor,
										inSpec->id);
				}

				/* Release allocated resources */
				g_value_unset(&fromValue);
				g_value_unset(&toValue);
			}
		}

		/* Release allocated resources */
		g_slist_free(actors);
	}

	/* Now iterate through actor-animation-list-mapping, create a transition group for each actor
	 * and add its animation to this newly created group.
	 */
	if(animationActorMap)
	{
		GHashTableIter										hashIter;
		ClutterActor										*hashIterActor;
		GSList												*hashIterList;

		g_hash_table_iter_init(&hashIter, animationActorMap);
		while(g_hash_table_iter_next(&hashIter, (gpointer*)&hashIterActor, (gpointer*)&hashIterList))
		{
			ClutterTransition								*transitionGroup;
			GSList											*listIter;
			guint											groupDuration;
			gint											groupLoop;

			/* Skip empty but warn */
			if(!hashIterList)
			{
				g_critical("Empty animation list when creating animation for animation specification '%s'",
							inSpec->id);
				continue;
			}

			/* Create transition group to collect property transitions at */
			transitionGroup=esdashboard_transition_group_new();
			if(!transitionGroup)
			{
				g_critical("Cannot allocate memory for transition group of animation specification '%s'",
							inSpec->id);
				
				/* Release allocated resources */
				g_hash_table_foreach(animationActorMap, _esdashboard_theme_animation_free_animation_actor_map_hashtable, NULL);
				g_hash_table_destroy(animationActorMap);
				g_object_unref(animation);

				return(NULL);
			}

			ESDASHBOARD_DEBUG(self, ANIMATION,
								"Created transition group at %p for %d properties for actor %s@%p of animation specification '%s'",
								transitionGroup,
								g_slist_length(hashIterList),
								G_OBJECT_TYPE_NAME(hashIterActor),
								hashIterActor,
								inSpec->id);

			/* Add animations to transition group */
			groupDuration=0;
			groupLoop=0;
			for(listIter=hashIterList; listIter; listIter=g_slist_next(listIter))
			{
				ClutterTransition							*transition;
				gint										transitionLoop;
				guint										transitionDuration;

				/* Get transition to add */
				transition=(ClutterTransition*)listIter->data;
				if(!transition) continue;

				/* Adjust timeline values for transition group to play animation fully */
				transitionDuration=clutter_timeline_get_delay(CLUTTER_TIMELINE(transition))+clutter_timeline_get_duration(CLUTTER_TIMELINE(transition));
				if(transitionDuration>groupDuration) groupDuration=transitionDuration;

				transitionLoop=0;
				if(groupLoop>=0)
				{
					transitionLoop=clutter_timeline_get_repeat_count(CLUTTER_TIMELINE(transition));
					if(transitionLoop>groupLoop) groupLoop=transitionLoop;
				}

				/* Add property transition to transition group */
				esdashboard_transition_group_add_transition(ESDASHBOARD_TRANSITION_GROUP(transitionGroup), transition);

				ESDASHBOARD_DEBUG(self, ANIMATION,
									"Added transition %s@%p (duration %u ms in %d loops) for actor %s@%p of animation specification '%s' to group %s@%p",
									G_OBJECT_TYPE_NAME(transition),
									transition,
									transitionDuration,
									transitionLoop,
									G_OBJECT_TYPE_NAME(hashIterActor),
									hashIterActor,
									inSpec->id,
									G_OBJECT_TYPE_NAME(transitionGroup),
									transitionGroup);
			}

			/* Set up timeline configuration for transition group of target actor */
			clutter_timeline_set_duration(CLUTTER_TIMELINE(transitionGroup), groupDuration);
			clutter_timeline_set_delay(CLUTTER_TIMELINE(transitionGroup), 0);
			clutter_timeline_set_progress_mode(CLUTTER_TIMELINE(transitionGroup), CLUTTER_LINEAR);
			clutter_timeline_set_repeat_count(CLUTTER_TIMELINE(transitionGroup), groupLoop);

			ESDASHBOARD_DEBUG(self, ANIMATION,
								"Set up timeline of group %s@%p with duration of %u ms and %d loops for actor %s@%p",
								G_OBJECT_TYPE_NAME(transitionGroup),
								transitionGroup,
								groupDuration,
								groupLoop,
								G_OBJECT_TYPE_NAME(hashIterActor),
								hashIterActor);

			/* Add transition group with collected property transitions
			 * to actor.
			 */
			if(animationClass->add_animation)
			{
				animationClass->add_animation(animation, hashIterActor, transitionGroup);
				ESDASHBOARD_DEBUG(self, ANIMATION,
									"Added transition group %s@%p to actor %s@%p of animation specification '%s'",
									G_OBJECT_TYPE_NAME(transitionGroup),
									transitionGroup,
									G_OBJECT_TYPE_NAME(hashIterActor),
									hashIterActor,
									inSpec->id);
			}

			/* Release allocated resources */
			g_object_unref(transitionGroup);
			g_slist_free_full(hashIterList, g_object_unref);
		}

		/* Destroy hash table */
		g_hash_table_destroy(animationActorMap);
	}

	return(animation);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_theme_animation_dispose(GObject *inObject)
{
	EsdashboardThemeAnimation				*self=ESDASHBOARD_THEME_ANIMATION(inObject);
	EsdashboardThemeAnimationPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->specs)
	{
		g_slist_free_full(priv->specs, (GDestroyNotify)_esdashboard_theme_animation_spec_unref);
		priv->specs=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_theme_animation_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_theme_animation_class_init(EsdashboardThemeAnimationClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_theme_animation_dispose;
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_theme_animation_init(EsdashboardThemeAnimation *self)
{
	EsdashboardThemeAnimationPrivate		*priv;

	priv=self->priv=esdashboard_theme_animation_get_instance_private(self);

	/* Set default values */
	priv->specs=NULL;
}

/* IMPLEMENTATION: Errors */

GQuark esdashboard_theme_animation_error_quark(void)
{
	return(g_quark_from_static_string("esdashboard-theme-animation-error-quark"));
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
EsdashboardThemeAnimation* esdashboard_theme_animation_new(void)
{
	return(ESDASHBOARD_THEME_ANIMATION(g_object_new(ESDASHBOARD_TYPE_THEME_ANIMATION, NULL)));
}

/* Load a XML file into theme */
gboolean esdashboard_theme_animation_add_file(EsdashboardThemeAnimation *self,
												const gchar *inPath,
												GError **outError)
{
	gchar								*contents;
	gsize								contentsLength;
	GError								*error;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), FALSE);
	g_return_val_if_fail(inPath!=NULL && *inPath!=0, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	/* Load XML file, parse it and build objects from file */
	error=NULL;
	if(!g_file_get_contents(inPath, &contents, &contentsLength, &error))
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	_esdashboard_theme_animation_parse_xml(self, inPath, contents, &error);
	if(error)
	{
		g_propagate_error(outError, error);
		g_free(contents);
		return(FALSE);
	}
	ESDASHBOARD_DEBUG(self, THEME, "Loaded animation file '%s'", inPath);

	/* Release allocated resources */
	g_free(contents);

	/* If we get here loading and parsing XML file was successful
	 * so return TRUE here
	 */
	return(TRUE);
}

/* Build requested animation for sender and its signal */
EsdashboardAnimation* esdashboard_theme_animation_create(EsdashboardThemeAnimation *self,
															EsdashboardActor *inSender,
															const gchar *inSignal,
															EsdashboardAnimationValue **inDefaultInitialValues,
															EsdashboardAnimationValue **inDefaultFinalValues)
{
	EsdashboardThemeAnimationSpec							*spec;
	EsdashboardAnimation									*animation;
	gboolean												animationEnabled;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTOR(inSender), NULL);
	g_return_val_if_fail(inSignal && *inSignal, NULL);

	animation=NULL;

	/* Check if user wants animation at all. If user does not want any animation,
	 * return NULL.
	 */
	animationEnabled=esconf_channel_get_bool(esdashboard_application_get_esconf_channel(NULL),
												ENABLE_ANIMATIONS_ESCONF_PROP,
												DEFAULT_ENABLE_ANIMATIONS);
	if(!animationEnabled)
	{
		ESDASHBOARD_DEBUG(self, ANIMATION,
							"User disabled animations so do not lookup animation for sender '%s' and signal '%s'",
							G_OBJECT_TYPE_NAME(inSender),
							inSignal);

		/* Return NULL as user does not want any animation */
		return(NULL);
	}

	/* Get best matching animation specification for sender and signal.
	 * If no matching animation specification is found, return the empty one.
	 */
	spec=_esdashboard_theme_animation_find_matching_animation_spec(self, ESDASHBOARD_STYLABLE(inSender), inSignal);
	if(!spec)
	{
		ESDASHBOARD_DEBUG(self, ANIMATION,
							"Could not find an animation specification for sender '%s' and signal '%s'",
							G_OBJECT_TYPE_NAME(inSender),
							inSignal);

		/* Return NULL as no matching animation specification was found */
		return(NULL);
	}

	ESDASHBOARD_DEBUG(self, ANIMATION,
						"Found animation specification '%s' for sender '%s' and signal '%s' with %d targets",
						spec->id,
						G_OBJECT_TYPE_NAME(inSender),
						inSignal,
						g_slist_length(spec->targets));

	/* Create animation for found animation specification */
	animation=_esdashboard_theme_animation_create_by_spec(self, spec, inSender, inDefaultInitialValues, inDefaultFinalValues);

	/* Release allocated resources */
	if(spec) _esdashboard_theme_animation_spec_unref(spec);

	return(animation);
}

/* Build requested animation by animation ID */
EsdashboardAnimation* esdashboard_theme_animation_create_by_id(EsdashboardThemeAnimation *self,
																EsdashboardActor *inSender,
																const gchar *inID,
																EsdashboardAnimationValue **inDefaultInitialValues,
																EsdashboardAnimationValue **inDefaultFinalValues)
{
	EsdashboardThemeAnimationSpec							*spec;
	EsdashboardAnimation									*animation;
	gboolean												animationEnabled;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTOR(inSender), NULL);
	g_return_val_if_fail(inID && *inID, NULL);

	animation=NULL;

	/* Check if user wants animation at all. If user does not want any animation,
	 * return NULL.
	 */
	animationEnabled=esconf_channel_get_bool(esdashboard_application_get_esconf_channel(NULL),
												ENABLE_ANIMATIONS_ESCONF_PROP,
												DEFAULT_ENABLE_ANIMATIONS);
	if(!animationEnabled)
	{
		ESDASHBOARD_DEBUG(self, ANIMATION,
							"User disabled animations so do not lookup animation with ID '%s'",
							inID);

		/* Return NULL as user does not want any animation */
		return(NULL);
	}

	/* Find animation specification by looking up requested ID.
	 * If no matching animation specification is found, return the empty one.
	 */
	spec=_esdashboard_theme_animation_find_animation_spec_by_id(self, inID);
	if(!spec)
	{
		ESDASHBOARD_DEBUG(self, ANIMATION,
							"Could not find an animation specification with ID '%s'",
							inID);

		/* Return NULL as no matching animation specification was found */
		return(NULL);
	}

	ESDASHBOARD_DEBUG(self, ANIMATION,
						"Found animation specification '%s'  with %d targets",
						spec->id,
						g_slist_length(spec->targets));

	/* Create animation for found animation specification */
	animation=_esdashboard_theme_animation_create_by_spec(self, spec, inSender, inDefaultInitialValues, inDefaultFinalValues);

	/* Release allocated resources */
	if(spec) _esdashboard_theme_animation_spec_unref(spec);

	return(animation);
}

/* Look up ID of animation specification for sender and its signal*/
gchar* esdashboard_theme_animation_lookup_id(EsdashboardThemeAnimation *self,
												EsdashboardActor *inSender,
												const gchar *inSignal)
{
	EsdashboardThemeAnimationSpec							*spec;
	gchar													*id;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_ACTOR(inSender), NULL);
	g_return_val_if_fail(inSignal && *inSignal, NULL);

	id=NULL;

	/* Get best matching animation specification for sender and signal.
	 * If no matching animation specification is found, return the empty one.
	 */
	spec=_esdashboard_theme_animation_find_matching_animation_spec(self, ESDASHBOARD_STYLABLE(inSender), inSignal);
	if(spec)
	{
		/* Get ID of animation specification to return */
		id=g_strdup(spec->id);

		/* Release allocated resources */
		_esdashboard_theme_animation_spec_unref(spec);
	}

	/* Return found ID or NULL */
	return(id);
}