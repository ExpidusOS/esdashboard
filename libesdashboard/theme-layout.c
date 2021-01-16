/*
 * theme-layout: A theme used for build and layout objects by XML files
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

#include <libesdashboard/theme-layout.h>

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>

#include <libesdashboard/enums.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Forward declaration */
typedef struct _EsdashboardThemeLayoutTagData				EsdashboardThemeLayoutTagData;
typedef struct _EsdashboardThemeLayoutParsedObject			EsdashboardThemeLayoutParsedObject;
typedef struct _EsdashboardThemeLayoutParserData			EsdashboardThemeLayoutParserData;
typedef struct _EsdashboardThemeLayoutUnresolvedBuildID		EsdashboardThemeLayoutUnresolvedBuildID;
typedef struct _EsdashboardThemeLayoutCheckRefID			EsdashboardThemeLayoutCheckRefID;

/* Define this class in GObject system */
struct _EsdashboardThemeLayoutPrivate
{
	/* Instance related */
	GSList								*interfaces;

	EsdashboardThemeLayoutTagData		*focusSelected;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardThemeLayout,
							esdashboard_theme_layout,
							G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
enum
{
	TAG_DOCUMENT,
	TAG_INTERFACE,
	TAG_OBJECT,
	TAG_CHILD,
	TAG_PROPERTY,
	TAG_CONSTRAINT,
	TAG_LAYOUT,
	TAG_FOCUSABLES,
	TAG_FOCUS
};

struct _EsdashboardThemeLayoutTagData
{
	gint								refCount;

	gint								tagType;

	union
	{
		struct
		{
			gchar						*id;
			gchar						*class;
		} object;

		struct
		{
			gchar						*name;
			gchar						*value;
			gboolean					translatable;
			gchar						*refID;
		} property;

		struct
		{
			gchar						*refID;
			gboolean					selected;
		} focus;
	} tag;
};

struct _EsdashboardThemeLayoutParsedObject
{
	gint								refCount;

	gchar								*id;
	GType								classType;
	GSList								*properties;	/* 0, 1 or more entries of EsdashboardThemeLayoutTagData */
	GSList								*constraints;	/* 0, 1 or more entries of EsdashboardThemeLayoutParsedObject */
	EsdashboardThemeLayoutParsedObject	*layout;		/* 0 or 1 entry of EsdashboardThemeLayoutParsedObject */
	GSList								*children;		/* 0, 1 or more entries of EsdashboardThemeLayoutParsedObject */
	GPtrArray							*focusables;	/* 0, 1 or more entries of EsdashboardThemeLayoutTagData (only used at <interface>) */
};

struct _EsdashboardThemeLayoutParserData
{
	EsdashboardThemeLayout				*self;

	EsdashboardThemeLayoutParsedObject	*interface;
	GQueue								*stackObjects;
	GQueue								*stackTags;
	GPtrArray							*focusables;	/* 0, 1 or more entries of EsdashboardThemeLayoutTagData */

	gint								lastLine;
	gint								lastPosition;
	gint								currentLine;
	gint								currentPostition;
	const gchar							*currentPath;
};

struct _EsdashboardThemeLayoutUnresolvedBuildID
{
	GObject								*targetObject;
	EsdashboardThemeLayoutTagData		*property;
};

struct _EsdashboardThemeLayoutCheckRefID
{
	EsdashboardThemeLayout				*self;
	GHashTable							*ids;
};

/* Forward declarations */
static void _esdashboard_theme_layout_parse_set_error(EsdashboardThemeLayoutParserData *inParserData,
														GMarkupParseContext *inContext,
														GError **outError,
														EsdashboardThemeLayoutErrorEnum inCode,
														const gchar *inFormat,
														...) G_GNUC_PRINTF (5, 6);

static void _esdashboard_theme_layout_object_data_unref(EsdashboardThemeLayoutParsedObject *inData);

#ifdef DEBUG
static void _esdashboard_theme_layout_print_parsed_objects_internal(EsdashboardThemeLayoutParsedObject *inData, gint inDepth, const gchar *inPrefix)
{
#define INDENT(n, s) { gint i; for(i=0; i<n; i++) g_print("%s", s); }
#define LOOP(slist, var, vartype, func) { GSList *entry=slist; while(entry) { var=(vartype*)entry->data; func ; entry=g_slist_next(entry); } }

	EsdashboardThemeLayoutTagData		*tagData;
	EsdashboardThemeLayoutParsedObject	*objectData;
	gint								j;
	gchar								*prefix;
	const gchar							*indention="    ";

	g_return_if_fail(inData);
	g_return_if_fail(inDepth>=0);
	g_return_if_fail(inPrefix);

	INDENT(inDepth, indention);
	g_print("# %s %p[%s] with id '%s' at depth %d (ref-count=%d, properties=%d, constraints=%d, layouts=%d, children=%d)\n",
				inPrefix,
				inData, g_type_name(inData->classType),
				inData->id ? inData->id : "<none>",
				inDepth,
				inData->refCount,
				g_slist_length(inData->properties),
				g_slist_length(inData->constraints),
				inData->layout ? 1 : 0,
				g_slist_length(inData->children));

	j=1;
	LOOP(inData->properties,
			tagData, EsdashboardThemeLayoutTagData,
			{
				INDENT(inDepth+1, indention);
				g_print("# Property %d: '%s'='%s' (ref-count=%d, translatable=%s, refID=%s)\n",
							j++,
							tagData->tag.property.name,
							tagData->tag.property.value,
							tagData->refCount,
							tagData->tag.property.translatable ? "yes" : "no",
							tagData->tag.property.refID);
			});

	j=1;
	LOOP(inData->constraints,
			objectData, EsdashboardThemeLayoutParsedObject,
			{
				prefix=g_strdup_printf("Constraint %d:", j++);
				_esdashboard_theme_layout_print_parsed_objects_internal(objectData, inDepth+1, prefix);
				g_free(prefix);
			});

	if(inData->layout)
	{
		objectData=inData->layout;

		_esdashboard_theme_layout_print_parsed_objects_internal(objectData, inDepth+1, "Layout:");
	}

	j=1;
	LOOP(inData->children,
			objectData, EsdashboardThemeLayoutParsedObject,
			{
				prefix=g_strdup_printf("Child %d:", j++);
				_esdashboard_theme_layout_print_parsed_objects_internal(objectData, inDepth+1, prefix);
				g_free(prefix);
			});

#undef LOOP
#undef INDENT
}

static void _esdashboard_theme_layout_print_parsed_objects(EsdashboardThemeLayoutParsedObject *inData, gpointer inUserData)
{
	const gchar			*prefix;

	g_return_if_fail(inData);
	g_return_if_fail(inUserData);

	prefix=(const gchar*)inUserData;

	g_print("----\n");
	_esdashboard_theme_layout_print_parsed_objects_internal(inData, 0, prefix);
	g_print("----\n");
}
#endif

/* Helper function to set up GError object in this parser */
static void _esdashboard_theme_layout_parse_set_error(EsdashboardThemeLayoutParserData *inParserData,
														GMarkupParseContext *inContext,
														GError **outError,
														EsdashboardThemeLayoutErrorEnum inCode,
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
	tempError=g_error_new_literal(ESDASHBOARD_THEME_LAYOUT_ERROR, inCode, message);
	if(inParserData)
	{
		g_prefix_error(&tempError,
						"File %s - Error on line %d char %d: ",
						inParserData->currentPath,
						inParserData->lastLine,
						inParserData->lastPosition);
	}

	/* Set error */
	g_propagate_error(outError, tempError);

	/* Release allocated resources */
	g_free(message);
}

/* Helper function to create instance of requested type.
 * Returns G_TYPE_INVALID if not found or unavailable.
 */
static GType _esdashboard_theme_layout_resolve_type_lazy(const gchar *inTypeName)
{
	static GModule		*appModule=NULL;
	GType				(*objectGetTypeFunc)(void);
	GString				*symbolName=g_string_new("");
	char				c, *symbol;
	int					i;
	GType				gtype=G_TYPE_INVALID;

	/* If it is the first call of this function get application as module */
	if(!appModule) appModule=g_module_open(NULL, 0);

	/* Get *_get_type() function for type (in camel-case) */
	for(i=0; inTypeName[i]!='\0'; i++)
	{
		c=inTypeName[i];

		/* Convert type name to lower case and insert an underscore '_'
		 * in front of any upper-case character (before it is converted
		 * to lower-case) but only if it is not the first character of
		 * type name or if the character before this one was also upper-case.
		 */
		if(c==g_ascii_toupper(c) &&
			(i>0 && inTypeName[i-1]!=g_ascii_toupper(inTypeName[i-1])))
		{
			g_string_append_c(symbolName, '_');
		}
		g_string_append_c(symbolName, g_ascii_tolower(c));
	}
	g_string_append(symbolName, "_get_type");

	/* Get pointer to *_get_name() and call it */
	symbol=g_string_free(symbolName, FALSE);

	if(g_module_symbol(appModule, symbol, (gpointer)&objectGetTypeFunc))
	{
		gtype=objectGetTypeFunc();
	}

	g_free (symbol);

	/* Return retrieved GType. Will be G_TYPE_INVALID in case of any error
	 * or if *_get_type() function was unavailable.
	 */
	return(gtype);
}

/* Determine tag name and ID */
static gint _esdashboard_theme_layout_get_tag_by_name(const gchar *inTag)
{
	g_return_val_if_fail(inTag && *inTag, -1);

	/* Compare string and return type ID */
	if(g_strcmp0(inTag, "interface")==0) return(TAG_INTERFACE);
	if(g_strcmp0(inTag, "object")==0) return(TAG_OBJECT);
	if(g_strcmp0(inTag, "child")==0) return(TAG_CHILD);
	if(g_strcmp0(inTag, "property")==0) return(TAG_PROPERTY);
	if(g_strcmp0(inTag, "constraint")==0) return(TAG_CONSTRAINT);
	if(g_strcmp0(inTag, "layout")==0) return(TAG_LAYOUT);
	if(g_strcmp0(inTag, "focusables")==0) return(TAG_FOCUSABLES);
	if(g_strcmp0(inTag, "focus")==0) return(TAG_FOCUS);

	/* If we get here we do not know tag name and return invalid ID */
	return(-1);
}

static const gchar* _esdashboard_theme_layout_get_tag_by_id(guint inTagType)
{
	/* Compare ID and return string */
	switch(inTagType)
	{
		case TAG_DOCUMENT:
			return("document");

		case TAG_INTERFACE:
			return("interface");

		case TAG_OBJECT:
			return("object");

		case TAG_CHILD:
			return("child");

		case TAG_PROPERTY:
			return("property");

		case TAG_CONSTRAINT:
			return("constraint");

		case TAG_LAYOUT:
			return("layout");

		case TAG_FOCUSABLES:
			return("focusables");

		case TAG_FOCUS:
			return("focus");

		default:
			break;
	}

	/* If we get here we do not know tag name and return NULL */
	return(NULL);
}

/* Create, destroy, ref and unref tag data for a tag */
static EsdashboardThemeLayoutTagData* _esdashboard_theme_layout_tag_data_new(GMarkupParseContext *inContext,
																				gint inTagType,
																				GError **outError)
{
	const gchar						*tagName;
	EsdashboardThemeLayoutTagData	*tagData;

	g_return_val_if_fail(outError && *outError==NULL, NULL);

	/* Check tag type */
	tagName=_esdashboard_theme_layout_get_tag_by_id(inTagType);
	if(!tagName)
	{
		_esdashboard_theme_layout_parse_set_error(NULL,
													inContext,
													outError,
													ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													"Cannot allocate memory for unknown tag");
		return(NULL);
	}

	/* Create tag data for given tag type */
	tagData=g_new0(EsdashboardThemeLayoutTagData, 1);
	if(!tagData)
	{
		_esdashboard_theme_layout_parse_set_error(NULL,
													inContext,
													outError,
													ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													"Cannot allocate memory for tag '%s'",
													tagName);
		return(NULL);
	}
	tagData->refCount=1;
	tagData->tagType=inTagType;

	return(tagData);
}

static void _esdashboard_theme_layout_tag_data_free(EsdashboardThemeLayoutTagData *inData)
{
	g_return_if_fail(inData);

	/* Release allocated resources for specified tag */
	switch(inData->tagType)
	{
		case TAG_OBJECT:
			if(inData->tag.object.id) g_free(inData->tag.object.id);
			if(inData->tag.object.class) g_free(inData->tag.object.class);
			break;

		case TAG_PROPERTY:
			if(inData->tag.property.name) g_free(inData->tag.property.name);
			if(inData->tag.property.value) g_free(inData->tag.property.value);
			if(inData->tag.property.refID) g_free(inData->tag.property.refID);
			break;

		case TAG_FOCUS:
			if(inData->tag.focus.refID) g_free(inData->tag.focus.refID);
			break;
	}

	/* Release common allocated resources */
	g_free(inData);
}

static EsdashboardThemeLayoutTagData* _esdashboard_theme_layout_tag_data_ref(EsdashboardThemeLayoutTagData *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _esdashboard_theme_layout_tag_data_unref(EsdashboardThemeLayoutTagData *inData)
{
	g_return_if_fail(inData);

	inData->refCount--;
	if(inData->refCount==0) _esdashboard_theme_layout_tag_data_free(inData);
}

static void _esdashboard_theme_layout_tag_data_free_foreach_callback(gpointer inData, gpointer inUserData)
{
	EsdashboardThemeLayoutTagData	*data;

	g_return_if_fail(inData);

	data=(EsdashboardThemeLayoutTagData*)inData;

	/* Free tag data */
	_esdashboard_theme_layout_tag_data_free(data);
}

/* Create, destroy, ref and unref object data */
static EsdashboardThemeLayoutParsedObject* _esdashboard_theme_layout_object_data_new(GMarkupParseContext *inContext,
																						gint inParentTagType,
																						GError **outError)
{
	EsdashboardThemeLayoutParsedObject	*objectData;

	g_return_val_if_fail(outError && *outError==NULL, NULL);

	/* Create object data */
	objectData=g_new0(EsdashboardThemeLayoutParsedObject, 1);
	if(!objectData)
	{
		_esdashboard_theme_layout_parse_set_error(NULL,
													inContext,
													outError,
													ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													"Cannot allocate memory for object data of tag <%s>",
													_esdashboard_theme_layout_get_tag_by_id(inParentTagType));
		return(NULL);
	}
	objectData->refCount=1;
	objectData->classType=G_TYPE_INVALID;

	return(objectData);
}

static void _esdashboard_theme_layout_object_data_free(EsdashboardThemeLayoutParsedObject *inData)
{
	g_return_if_fail(inData);

	/* Release allocated resources */
	if(inData->id) g_free(inData->id);
	if(inData->properties) g_slist_free_full(inData->properties, (GDestroyNotify)_esdashboard_theme_layout_tag_data_unref);
	if(inData->constraints) g_slist_free_full(inData->constraints, (GDestroyNotify)_esdashboard_theme_layout_object_data_unref);
	if(inData->layout) _esdashboard_theme_layout_object_data_unref(inData->layout);
	if(inData->children) g_slist_free_full(inData->children, (GDestroyNotify)_esdashboard_theme_layout_object_data_unref);
	if(inData->focusables) g_ptr_array_unref(inData->focusables);
	g_free(inData);
}

static EsdashboardThemeLayoutParsedObject* _esdashboard_theme_layout_object_data_ref(EsdashboardThemeLayoutParsedObject *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _esdashboard_theme_layout_object_data_unref(EsdashboardThemeLayoutParsedObject *inData)
{
	g_return_if_fail(inData);

	inData->refCount--;
	if(inData->refCount==0) _esdashboard_theme_layout_object_data_free(inData);
}

static void _esdashboard_theme_layout_object_data_free_foreach_callback(gpointer inData, gpointer inUserData)
{
	EsdashboardThemeLayoutParsedObject	*data;

	g_return_if_fail(inData);

	data=(EsdashboardThemeLayoutParsedObject*)inData;

	/* Free object data */
	_esdashboard_theme_layout_object_data_free(data);
}

/* Free data about an unresolved property which refers an other object */
static void _esdashboard_theme_layout_create_object_free_unresolved(gpointer inData)
{
	EsdashboardThemeLayoutUnresolvedBuildID		*unresolved;

	g_return_if_fail(inData);

	/* Release allocated resources */
	unresolved=(EsdashboardThemeLayoutUnresolvedBuildID*)inData;
	if(unresolved->targetObject) g_object_unref(unresolved->targetObject);
	if(unresolved->property) _esdashboard_theme_layout_tag_data_unref(unresolved->property);
	g_free(unresolved);
}

/* Resolve all "unresolved" properties (properties which refer to other objects) and
 * set the pointer to resolved object at the associated property.
 */
static void _esdashboard_theme_layout_create_object_resolve_unresolved(EsdashboardThemeLayout *self,
																		GHashTable *inIDs,
																		GSList *inUnresolvedIDs,
																		va_list inArgs)
{
	GSList										*iter;
	EsdashboardThemeLayoutUnresolvedBuildID		*unresolvedID;
	GObject										*refObject;
	GPtrArray									*focusTable;
	ClutterActor								*focusSelected;
	gint										extraDataID;
	gpointer									*extraDataStorage;

	g_return_if_fail(ESDASHBOARD_IS_THEME_LAYOUT(self));
	g_return_if_fail(inIDs);

	focusTable=NULL;
	focusSelected=NULL;

	/* Return if no list of unresolved IDs is provided */
	if(!inUnresolvedIDs) return;

	/* Iterate through list of unresolved IDs and their mapping to properties
	 * and get referenced object and set the pointer to this object at the
	 * mapped property of target object.
	 */
	for(iter=inUnresolvedIDs; iter; iter=g_slist_next(iter))
	{
		unresolvedID=(EsdashboardThemeLayoutUnresolvedBuildID*)iter->data;
		g_assert(unresolvedID);

		/* Get ID of object to resolve but only from supported tag types */
		switch(unresolvedID->property->tagType)
		{
			case TAG_PROPERTY:
				/* Get referenced object */
				refObject=g_hash_table_lookup(inIDs, unresolvedID->property->tag.property.refID);

				/* Set pointer to referenced object in property of target object */
				g_object_set(unresolvedID->targetObject,
								unresolvedID->property->tag.property.name,
								refObject,
								NULL);
				ESDASHBOARD_DEBUG(self, THEME,
									"Set previously unresolved object %s with ID '%s' at target object %s at property '%s'",
									refObject ? G_OBJECT_TYPE_NAME(refObject) : "<unknown object>",
									unresolvedID->property->tag.property.refID,
									unresolvedID->targetObject ? G_OBJECT_TYPE_NAME(unresolvedID->targetObject) : "<unknown object>",
									unresolvedID->property->tag.property.name);
				break;

			case TAG_FOCUS:
				/* Get referenced object */
				refObject=g_hash_table_lookup(inIDs, unresolvedID->property->tag.focus.refID);

				/* Store reference object in list of focusable actors */
				if(!focusTable) focusTable=g_ptr_array_new();
				g_ptr_array_add(focusTable, refObject);

				ESDASHBOARD_DEBUG(self, THEME,
									"Added resolved focusable actor %s with reference ID '%s' to focusable list at target object %s ",
									refObject ? G_OBJECT_TYPE_NAME(refObject) : "<unknown object>",
									unresolvedID->property->tag.focus.refID,
									unresolvedID->targetObject ? G_OBJECT_TYPE_NAME(unresolvedID->targetObject) : "<unknown object>");

				/* If focusable actor is pre-selected and no other pre-selected
				 * one was seen so far, remember it now.
				 */
				if(!focusSelected && unresolvedID->property->tag.focus.selected)
				{
					focusSelected=CLUTTER_ACTOR(g_object_ref(refObject));

					ESDASHBOARD_DEBUG(self, THEME,
										"Remember resolved focusable actor %s with reference ID '%s' as pre-selected actor at target object %s ",
										refObject ? G_OBJECT_TYPE_NAME(refObject) : "<unknown object>",
										unresolvedID->property->tag.focus.refID,
										unresolvedID->targetObject ? G_OBJECT_TYPE_NAME(unresolvedID->targetObject) : "<unknown object>");
				}
				break;

			default:
				g_critical("Unsupported tag type '%s' to resolve ID",
							_esdashboard_theme_layout_get_tag_by_id(unresolvedID->property->tagType));
				break;
		}
	}

	/* Iterate through extra data ID and pointer where to store the value (the
	 * resolved object(s)) until end of list (marked with -1) is reached.
	 */
	extraDataID=va_arg(inArgs, gint);
	while(extraDataID!=-1)
	{
		/* Get generic pointer to storage as it will be casted as necessary
		 * when determining which extra data is requested.
		 */
		extraDataStorage=va_arg(inArgs, gpointer*);
		if(!extraDataStorage)
		{
			gchar *valueName;

			valueName=esdashboard_get_enum_value_name(ESDASHBOARD_TYPE_THEME_LAYOUT_BUILD_GET, extraDataID);
			g_warning("No storage pointer provided to store value of %s",
						valueName);
			g_free(valueName);
			break;
		}

		/* Check which extra data is requested and store value at pointer */
		switch(extraDataID)
		{
			case ESDASHBOARD_THEME_LAYOUT_BUILD_GET_FOCUSABLES:
				if(focusTable) *((GPtrArray**)extraDataStorage)=g_ptr_array_ref(focusTable);
					else *extraDataStorage=NULL;
				break;

			case ESDASHBOARD_THEME_LAYOUT_BUILD_GET_SELECTED_FOCUS:
				if(focusSelected) *extraDataStorage=g_object_ref(focusSelected);
					else *extraDataStorage=NULL;
				break;

			default:
				g_assert_not_reached();
				break;
		}

		/* Continue with next extra data ID and storage pointer */
		extraDataID=va_arg(inArgs, gint);
	}

	/* Release allocated resources */
	if(focusTable) g_ptr_array_unref(focusTable);
	if(focusSelected) g_object_unref(focusSelected);
}

/* Create object with all its constraints, layout and children recursively.
 * Set up all properties which do not reference any other object at creation of object
 * but remember all "unresolved" properties (which do reference other objects).
 */
static GObject* _esdashboard_theme_layout_create_object(EsdashboardThemeLayout *self,
														EsdashboardThemeLayoutParsedObject *inObjectData,
														GHashTable *ioIDs,
														GSList **ioUnresolvedIDs)
{
	GObject									*object;
	GSList									*iter;
	gchar									**names;
	GValue									*values;
	gint									maxProperties, usedProperties, i;
	guint									j;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_LAYOUT(self), NULL);
	g_return_val_if_fail(inObjectData, NULL);
	g_return_val_if_fail(ioIDs, NULL);
	g_return_val_if_fail(ioUnresolvedIDs, NULL);

	/* Collect all properties as array which do not refer to other objects */
	usedProperties=0;
	names=NULL;
	values=NULL;

	maxProperties=g_slist_length(inObjectData->properties);
	if(maxProperties>0)
	{
		names=g_new0(gchar*, maxProperties);
		values=g_new0(GValue, maxProperties);
	}

	for(iter=inObjectData->properties; iter; iter=g_slist_next(iter))
	{
		EsdashboardThemeLayoutTagData		*property;

		property=(EsdashboardThemeLayoutTagData*)iter->data;

		/* Check if property refers to an other object, if not add it */
		if(!property->tag.property.refID)
		{
			names[usedProperties]=g_strdup(property->tag.property.name);
			g_value_init(&values[usedProperties], G_TYPE_STRING);

			if(!property->tag.property.translatable) g_value_set_string(&values[usedProperties], property->tag.property.value);
				else g_value_set_string(&values[usedProperties], _(property->tag.property.value));

			usedProperties++;
		}
	}

	/* Create instance of object type and before handling error or success
	 * of creation release allocated resources for properties as they are
	 * not needed anymore.
	 */
	object=g_object_new_with_properties(inObjectData->classType,
										usedProperties,
										(const gchar **)names,
										(const GValue *)values);

	for(i=0; i<usedProperties; i++)
	{
		g_free(names[i]);
		g_value_unset(&values[i]);
	}
	g_free(names);
	g_free(values);

	if(!object)
	{
		ESDASHBOARD_DEBUG(self, THEME,
							"Failed to create object of type %s with %d properties to set",
							g_type_name(inObjectData->classType),
							usedProperties);

		/* Return NULL indicating error */
		return(NULL);
	}

	ESDASHBOARD_DEBUG(self, THEME,
						"Created object %p of type %s",
						object,
						G_OBJECT_TYPE_NAME(object));

	/* If object created has an ID and is derived from ClutterActor
	 * then set ID as name of actor if name was not set by any property.
	 * Otherwise only remember created ID.
	 */
	if(inObjectData->id)
	{
		if(CLUTTER_IS_ACTOR(object))
		{
			gchar							*name;

			g_object_get(object, "name", &name, NULL);
			if(!name || strlen(name)==0)
			{
				g_object_set(object, "name", inObjectData->id, NULL);
				ESDASHBOARD_DEBUG(self, THEME,
									"Object %s has ID but no name, setting ID '%s' as name",
									G_OBJECT_TYPE_NAME(object),
									inObjectData->id);
			}
			if(name) g_free(name);
		}

		g_hash_table_insert(ioIDs, g_strdup(inObjectData->id), object);
	}

	/* Create children */
	for(iter=inObjectData->children; iter; iter=g_slist_next(iter))
	{
		EsdashboardThemeLayoutParsedObject	*childObjectData;
		GObject								*child;

		childObjectData=(EsdashboardThemeLayoutParsedObject*)iter->data;

		/* Create child actor */
		child=_esdashboard_theme_layout_create_object(self, childObjectData, ioIDs, ioUnresolvedIDs);
		if(!child || !CLUTTER_IS_ACTOR(child))
		{
			if(child)
			{
				ESDASHBOARD_DEBUG(self, THEME,
									"Child %s is not an actor and cannot be added to actor %s",
									G_OBJECT_TYPE_NAME(child),
									G_OBJECT_TYPE_NAME(object));
			}
				else
				{
					ESDASHBOARD_DEBUG(self, THEME,
										"Failed to create child for actor %s",
										G_OBJECT_TYPE_NAME(object));
				}

			/* Release allocated resources */
			if(child) g_object_unref(child);
			g_object_unref(object);

			/* Return NULL indicating error */
			return(NULL);
		}

		/* Add successfully created child actor to this actor */
		if(childObjectData->id) g_hash_table_insert(ioIDs, g_strdup(childObjectData->id), child);
		clutter_actor_add_child(CLUTTER_ACTOR(object), CLUTTER_ACTOR(child));
		ESDASHBOARD_DEBUG(self, THEME,
							"Created child %s and added to object %s",
							G_OBJECT_TYPE_NAME(child),
							G_OBJECT_TYPE_NAME(object));
	}

	/* Create layout */
	if(inObjectData->layout)
	{
		EsdashboardThemeLayoutParsedObject	*layoutObjectData;
		GObject								*layout;

		layoutObjectData=(EsdashboardThemeLayoutParsedObject*)inObjectData->layout;

		/* Create layout */
		layout=_esdashboard_theme_layout_create_object(self, layoutObjectData, ioIDs, ioUnresolvedIDs);
		if(!layout || !CLUTTER_IS_LAYOUT_MANAGER(layout))
		{
			if(layout)
			{
				ESDASHBOARD_DEBUG(self, THEME,
									"Layout %s is not a layout manager and cannot be set at actor %s",
									G_OBJECT_TYPE_NAME(layout),
									G_OBJECT_TYPE_NAME(object));
			}
				else
				{
					ESDASHBOARD_DEBUG(self, THEME,
										"Failed to create layout manager for actor %s",
										G_OBJECT_TYPE_NAME(object));
				}

			/* Release allocated resources */
			if(layout) g_object_unref(layout);
			g_object_unref(object);

			/* Return NULL indicating error */
			return(NULL);
		}

		/* Add successfully created child actor to this actor */
		if(layoutObjectData->id) g_hash_table_insert(ioIDs, g_strdup(layoutObjectData->id), layout);
		clutter_actor_set_layout_manager(CLUTTER_ACTOR(object), CLUTTER_LAYOUT_MANAGER(layout));
		ESDASHBOARD_DEBUG(self, THEME,
							"Created layout manager %s and set at object %s",
							G_OBJECT_TYPE_NAME(layout),
							G_OBJECT_TYPE_NAME(object));
	}

	/* Create constraints */
	for(iter=inObjectData->constraints; iter; iter=g_slist_next(iter))
	{
		EsdashboardThemeLayoutParsedObject	*constraintObjectData;
		GObject								*constraint;

		constraintObjectData=(EsdashboardThemeLayoutParsedObject*)iter->data;

		/* Create constraint */
		constraint=_esdashboard_theme_layout_create_object(self, constraintObjectData, ioIDs, ioUnresolvedIDs);
		if(!constraint || !CLUTTER_IS_CONSTRAINT(constraint))
		{
			if(constraint)
			{
				ESDASHBOARD_DEBUG(self, THEME,
									"Constraint %s is not a constraint and cannot be added to actor %s",
									G_OBJECT_TYPE_NAME(constraint),
									G_OBJECT_TYPE_NAME(object));
			}
				else
				{
					ESDASHBOARD_DEBUG(self, THEME,
										"Failed to create constraint for actor %s",
										G_OBJECT_TYPE_NAME(object));
				}

			/* Release allocated resources */
			if(constraint) g_object_unref(constraint);
			g_object_unref(object);

			/* Return NULL indicating error */
			return(NULL);
		}

		/* Add successfully created constraint to this actor */
		if(constraintObjectData->id) g_hash_table_insert(ioIDs, g_strdup(constraintObjectData->id), constraint);
		clutter_actor_add_constraint(CLUTTER_ACTOR(object), CLUTTER_CONSTRAINT(constraint));
		ESDASHBOARD_DEBUG(self, THEME,
							"Created constraint %s and added to object %s",
							G_OBJECT_TYPE_NAME(constraint),
							G_OBJECT_TYPE_NAME(object));
	}

	/* Set up properties which do reference other objects */
	for(iter=inObjectData->properties; iter; iter=g_slist_next(iter))
	{
		EsdashboardThemeLayoutTagData					*property;

		/* Get property data */
		property=(EsdashboardThemeLayoutTagData*)iter->data;

		/* Check if property refers to an other object, if it does add it */
		if(property->tag.property.refID)
		{
			EsdashboardThemeLayoutUnresolvedBuildID		*unresolved;

			/* Create unresolved entry */
			unresolved=g_new0(EsdashboardThemeLayoutUnresolvedBuildID, 1);
			unresolved->targetObject=g_object_ref(object);
			unresolved->property=_esdashboard_theme_layout_tag_data_ref(property);

			/* Add to list of unresolved IDs */
			*ioUnresolvedIDs=g_slist_prepend(*ioUnresolvedIDs, unresolved);
		}
	}

	/* Set up focusables which do reference other objects */
	if(inObjectData->focusables)
	{
		for(j=0; j<inObjectData->focusables->len; j++)
		{
			EsdashboardThemeLayoutTagData					*focus;
			EsdashboardThemeLayoutUnresolvedBuildID			*unresolved;

			/* Get focus data */
			focus=(EsdashboardThemeLayoutTagData*)g_ptr_array_index(inObjectData->focusables, j);

			/* Create unresolved entry */
			unresolved=g_new0(EsdashboardThemeLayoutUnresolvedBuildID, 1);
			unresolved->targetObject=g_object_ref(object);
			unresolved->property=_esdashboard_theme_layout_tag_data_ref(focus);

			/* Add to list of unresolved IDs.
			 * It is important to add it at the end of list to keep order
			 * of focusable actors.
			 */
			*ioUnresolvedIDs=g_slist_append(*ioUnresolvedIDs, unresolved);
		}
	}

	/* Return created actor */
	return(object);
}

/* Callbacks used for <property> tag */
static void _esdashboard_theme_layout_parse_property_text_node(GMarkupParseContext *inContext,
																const gchar *inText,
																gsize inTextLength,
																gpointer inUserData,
																GError **outError)
{
	EsdashboardThemeLayoutParserData		*data=(EsdashboardThemeLayoutParserData*)inUserData;
	EsdashboardThemeLayoutTagData			*tagData;

	/* Get tag data for property */
	if(g_queue_is_empty(data->stackTags))
	{
		_esdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													"Unexpected empty tag stack when parsing property text node");
		return;
	}

	tagData=(EsdashboardThemeLayoutTagData*)g_queue_peek_tail(data->stackTags);
	if(tagData->tag.property.value)
	{
		_esdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													"Value for property '%s' is already set",
													tagData->tag.property.name);
		return;
	}

	/* Store value for property */
	tagData->tag.property.value=g_strdup(inText);
}

static void _esdashboard_theme_layout_parse_property_start(GMarkupParseContext *inContext,
															const gchar *inElementName,
															const gchar **inAttributeNames,
															const gchar **inAttributeValues,
															gpointer inUserData,
															GError **outError)
{
	EsdashboardThemeLayoutParserData		*data=(EsdashboardThemeLayoutParserData*)inUserData;
	gint									currentTag=TAG_DOCUMENT;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Set error message */
	if(!g_queue_is_empty(data->stackTags))
	{
		EsdashboardThemeLayoutTagData		*tagData;

		tagData=(EsdashboardThemeLayoutTagData*)g_queue_peek_tail(data->stackTags);
		currentTag=tagData->tagType;
	}

	_esdashboard_theme_layout_parse_set_error(data,
												inContext,
												outError,
												ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
												"Tag <%s> cannot contain tag <%s>",
												_esdashboard_theme_layout_get_tag_by_id(currentTag),
												inElementName);
}

/* General callbacks which can be used for any tag */
static void _esdashboard_theme_layout_parse_general_no_text_nodes(GMarkupParseContext *inContext,
																	const gchar *inText,
																	gsize inTextLength,
																	gpointer inUserData,
																	GError **outError)
{
	EsdashboardThemeLayoutParserData		*data=(EsdashboardThemeLayoutParserData*)inUserData;
	gchar									*realText;

	/* Check if text contains only whitespace. If we find any non-whitespace
	 * in text then set error.
	 */
	realText=g_strstrip(g_strdup(inText));
	if(*realText)
	{
		const GSList	*parents;

		parents=g_markup_parse_context_get_element_stack(inContext);
		if(parents) parents=g_slist_next(parents);

		_esdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
													"Unexpected text node '%s' at tag <%s>",
													realText,
													parents ? (gchar*)parents->data : "document");
	}
	g_free(realText);
}

static void _esdashboard_theme_layout_parse_general_start(GMarkupParseContext *inContext,
															const gchar *inElementName,
															const gchar **inAttributeNames,
															const gchar **inAttributeValues,
															gpointer inUserData,
															GError **outError)
{
	EsdashboardThemeLayoutParserData		*data=(EsdashboardThemeLayoutParserData*)inUserData;
	gint									currentTag=TAG_DOCUMENT;
	gint									nextTag;
	GError									*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* If we have not seen any tag yet then it is the document root */
	if(!g_queue_is_empty(data->stackTags))
	{
		EsdashboardThemeLayoutTagData		*tagData;

		tagData=(EsdashboardThemeLayoutTagData*)g_queue_peek_tail(data->stackTags);
		currentTag=tagData->tagType;
	}

	/* Get tag of next element */
	nextTag=_esdashboard_theme_layout_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_esdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
													"Unknown tag <%s>",
													inElementName);
		return;
	}

	/* Check if element name is <interface> and follows expected parent tags:
	 * <document>
	 */
	if(nextTag==TAG_INTERFACE &&
		currentTag==TAG_DOCUMENT)
	{
		EsdashboardThemeLayoutTagData		*tagData;

		/* Create tag data */
		tagData=_esdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_INVALID,
											NULL))
		{
			g_propagate_error(outError, error);
			_esdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);
		return;
	}

	/* Check if element name is <object> and follows expected parent tags:
	 * <interface>, <child>, <constraint>, <layout>
	 */
	if(nextTag==TAG_OBJECT &&
		(currentTag==TAG_INTERFACE ||
			currentTag==TAG_CHILD ||
			currentTag==TAG_CONSTRAINT ||
			currentTag==TAG_LAYOUT))
	{
		EsdashboardThemeLayoutTagData		*tagData;
		EsdashboardThemeLayoutParsedObject	*objectData;
		GType								expectedClassType=G_TYPE_INVALID;

		/* Create tag and object data */
		tagData=_esdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
		{
			g_propagate_error(outError, error);
			return;
		}

		objectData=_esdashboard_theme_layout_object_data_new(inContext, currentTag, &error);
		if(!objectData)
		{
			g_propagate_error(outError, error);
			_esdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
											"id",
											&tagData->tag.object.id,
											G_MARKUP_COLLECT_STRDUP,
											"class",
											&tagData->tag.object.class,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			_esdashboard_theme_layout_tag_data_unref(tagData);
			_esdashboard_theme_layout_object_data_unref(objectData);
			return;
		}

		/* Check tag's attributes */
		if(tagData->tag.object.id)
		{
			objectData->id=g_strdup(tagData->tag.object.id);
			if(strlen(objectData->id)==0)
			{
				_esdashboard_theme_layout_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
															"Empty ID at tag '%s'",
															inElementName);
				_esdashboard_theme_layout_tag_data_unref(tagData);
				_esdashboard_theme_layout_object_data_unref(objectData);
				return;
			}

			if(!esdashboard_is_valid_id(tagData->tag.object.id))
			{
				_esdashboard_theme_layout_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
															"Invalid ID '%s' at tag '%s'",
															tagData->tag.object.id,
															inElementName);
				_esdashboard_theme_layout_tag_data_unref(tagData);
				_esdashboard_theme_layout_object_data_unref(objectData);
				return;
			}
		}

		objectData->classType=_esdashboard_theme_layout_resolve_type_lazy(tagData->tag.object.class);
		if(objectData->classType==G_TYPE_INVALID)
		{
			_esdashboard_theme_layout_parse_set_error(data,
														inContext,
														outError,
														ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
														"Unknown object class %s for tag '%s'",
														tagData->tag.object.class,
														inElementName);
			_esdashboard_theme_layout_tag_data_unref(tagData);
			_esdashboard_theme_layout_object_data_unref(objectData);
			return;
		}

		if(currentTag==TAG_INTERFACE) expectedClassType=CLUTTER_TYPE_ACTOR;
			else if(currentTag==TAG_CHILD) expectedClassType=CLUTTER_TYPE_ACTOR;
			else if(currentTag==TAG_CONSTRAINT) expectedClassType=CLUTTER_TYPE_CONSTRAINT;
			else if(currentTag==TAG_LAYOUT) expectedClassType=CLUTTER_TYPE_LAYOUT_MANAGER;

		g_assert(expectedClassType!=G_TYPE_INVALID);
		if(!g_type_is_a(objectData->classType, expectedClassType))
		{
			_esdashboard_theme_layout_parse_set_error(data,
														inContext,
														outError,
														ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
														"Invalid class %s in object for parent tag <%s> - expecting class derived from %s",
														tagData->tag.object.class,
														_esdashboard_theme_layout_get_tag_by_id(currentTag),
														g_type_name(expectedClassType));
			_esdashboard_theme_layout_tag_data_unref(tagData);
			_esdashboard_theme_layout_object_data_unref(objectData);
			return;
		}

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);

		/* Push object onto stack */
		g_queue_push_tail(data->stackObjects, objectData);
		return;
	}

	/* Check if element name is <child>, <layout> or <constraint> and follows expected parent tags:
	 * <object>
	 */
	if((nextTag==TAG_CHILD || nextTag==TAG_LAYOUT || nextTag==TAG_CONSTRAINT) &&
		currentTag==TAG_OBJECT)
	{
		EsdashboardThemeLayoutTagData		*tagData;
		EsdashboardThemeLayoutParsedObject	*parentObject;

		/* <layout> and <constraint> can only be set for parent objects derived of type ClutterActor */
		parentObject=(EsdashboardThemeLayoutParsedObject*)g_queue_peek_tail(data->stackObjects);
		if(!parentObject ||
			!g_type_is_a(parentObject->classType, CLUTTER_TYPE_ACTOR))
		{
			_esdashboard_theme_layout_parse_set_error(data,
														inContext,
														outError,
														ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
														"Tag <%s> can only be set at <%s> creating objects derived from class %s",
														inElementName,
														_esdashboard_theme_layout_get_tag_by_id(currentTag),
														g_type_name(CLUTTER_TYPE_ACTOR));
			return;
		}

		/* Create tag data */
		tagData=_esdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_INVALID,
											NULL))
		{
			g_propagate_error(outError, error);
			_esdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);
		return;
	}

	/* Check if element name is <property> and follows expected parent tags:
	 * <object>
	 */
	if(nextTag==TAG_PROPERTY &&
		currentTag==TAG_OBJECT)
	{
		EsdashboardThemeLayoutTagData		*tagData;
		static GMarkupParser				propertyParser=
											{
												_esdashboard_theme_layout_parse_property_start,
												NULL,
												_esdashboard_theme_layout_parse_property_text_node,
												NULL,
												NULL,
											};

		/* Create tag data */
		tagData=_esdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRDUP,
											"name",
											&tagData->tag.property.name,
											G_MARKUP_COLLECT_BOOLEAN | G_MARKUP_COLLECT_OPTIONAL,
											"translatable",
											&tagData->tag.property.translatable,
											G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
											"ref",
											&tagData->tag.property.refID,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			_esdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Check tag's attributes */
		if(tagData->tag.property.refID &&
			strlen(tagData->tag.property.refID)==0)
		{
			_esdashboard_theme_layout_parse_set_error(data,
														inContext,
														outError,
														ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
														"Attribute 'ref' cannot be empty at tag <%s>",
														inElementName);
			_esdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);

		/* Properties can have text nodes but no children so set up context */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* Check if element name is <focusables> and follows expected parent tags:
	 * <interface>
	 */
	if(nextTag==TAG_FOCUSABLES &&
		currentTag==TAG_INTERFACE)
	{
		EsdashboardThemeLayoutTagData		*tagData;

		/* <interface> can only have one <focusables> element */
		if(data->focusables)
		{
			_esdashboard_theme_layout_parse_set_error(data,
														inContext,
														outError,
														ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
														"Tag <%s> can have only one <%s>",
														_esdashboard_theme_layout_get_tag_by_id(currentTag),
														inElementName);
			return;
		}

		/* Create tag data */
		tagData=_esdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_INVALID,
											NULL))
		{
			g_propagate_error(outError, error);
			_esdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Create array to store focusables at. An empty array will at least
		 * indicate that theme wanted to define focusables, in this case
		 * no focusable actors at all.
		 */
		data->focusables=g_ptr_array_new_with_free_func((GDestroyNotify)_esdashboard_theme_layout_tag_data_unref);

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);
		return;
	}

	/* Check if element name is <focus> and follows expected parent tags:
	 * <focusables>
	 */
	if(nextTag==TAG_FOCUS &&
		currentTag==TAG_FOCUSABLES)
	{
		EsdashboardThemeLayoutTagData		*tagData;

		/* Create tag data */
		tagData=_esdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRDUP,
											"ref",
											&tagData->tag.focus.refID,
											G_MARKUP_COLLECT_BOOLEAN | G_MARKUP_COLLECT_OPTIONAL,
											"selected",
											&tagData->tag.focus.selected,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			_esdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_esdashboard_theme_layout_parse_set_error(data,
												inContext,
												outError,
												ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
												"Tag <%s> cannot contain tag <%s>",
												_esdashboard_theme_layout_get_tag_by_id(currentTag),
												inElementName);
}

static void _esdashboard_theme_layout_parse_general_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	EsdashboardThemeLayoutParserData			*data=(EsdashboardThemeLayoutParserData*)inUserData;
	EsdashboardThemeLayoutTagData				*tagData;
	EsdashboardThemeLayoutTagData				*subTagData;

	/* Get last tag from stack */
	subTagData=(EsdashboardThemeLayoutTagData*)g_queue_pop_tail(data->stackTags);
	if(!subTagData)
	{
		_esdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													"Internal error when handling end of tag <%s>",
													inElementName);
		return;
	}

	/* Get tag data for this element */
	tagData=(EsdashboardThemeLayoutTagData*)g_queue_peek_tail(data->stackTags);

	/* Handle end of element <object> */
	if(subTagData->tagType==TAG_OBJECT)
	{
		EsdashboardThemeLayoutParsedObject		*objectData;
		EsdashboardThemeLayoutParsedObject		*parentObjectData;

		objectData=(EsdashboardThemeLayoutParsedObject*)g_queue_pop_tail(data->stackObjects);
		parentObjectData=(EsdashboardThemeLayoutParsedObject*)g_queue_peek_tail(data->stackObjects);

		/* Object is a child of <interface> */
		if(tagData->tagType==TAG_INTERFACE)
		{
			g_assert(parentObjectData==NULL);

			/* There can be only one <interface> per file */
			if(data->interface)
			{
				_esdashboard_theme_layout_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
															"Document can have only one <%s>",
															_esdashboard_theme_layout_get_tag_by_id(subTagData->tagType));
				_esdashboard_theme_layout_tag_data_unref(subTagData);
				_esdashboard_theme_layout_object_data_unref(objectData);
				return;
			}

			/* Append interface to list of known interface */
			data->interface=_esdashboard_theme_layout_object_data_ref(objectData);
		}

		/* Object is a child of <child> */
		if(tagData->tagType==TAG_CHILD)
		{
			g_assert(parentObjectData!=NULL);

			parentObjectData->children=g_slist_append(parentObjectData->children, _esdashboard_theme_layout_object_data_ref(objectData));
		}

		/* Object is a child of <constraint> */
		if(tagData->tagType==TAG_CONSTRAINT)
		{
			g_assert(parentObjectData!=NULL);

			parentObjectData->constraints=g_slist_append(parentObjectData->constraints, _esdashboard_theme_layout_object_data_ref(objectData));
		}

		/* Object is a child of <layout> */
		if(tagData->tagType==TAG_LAYOUT)
		{
			g_assert(parentObjectData!=NULL);

			if(parentObjectData->layout)
			{
				_esdashboard_theme_layout_parse_set_error(data,
															inContext,
															outError,
															ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
															"Object can have only one <%s>",
															_esdashboard_theme_layout_get_tag_by_id(subTagData->tagType));
				_esdashboard_theme_layout_tag_data_unref(subTagData);
				_esdashboard_theme_layout_object_data_unref(objectData);
				return;
			}

			parentObjectData->layout=_esdashboard_theme_layout_object_data_ref(objectData);
		}

		/* Unreference last object's data */
		_esdashboard_theme_layout_object_data_unref(objectData);
	}

	/* Handle end of element <property> */
	if(subTagData->tagType==TAG_PROPERTY)
	{
		EsdashboardThemeLayoutParsedObject		*objectData;

		/* Add property to object */
		objectData=(EsdashboardThemeLayoutParsedObject*)g_queue_peek_tail(data->stackObjects);
		objectData->properties=g_slist_append(objectData->properties, _esdashboard_theme_layout_tag_data_ref(subTagData));
		ESDASHBOARD_DEBUG(data->self, THEME,
							"Adding property '%s' with %s '%s' to object %s",
							subTagData->tag.property.name,
							subTagData->tag.property.refID ? "referenced object of ID" : "value",
							subTagData->tag.property.refID ? subTagData->tag.property.refID : subTagData->tag.property.value,
							g_type_name(objectData->classType));

		/* Restore previous parser context */
		g_markup_parse_context_pop(inContext);
	}

	/* Handle end of element <focus> */
	if(subTagData->tagType==TAG_FOCUS)
	{

		g_assert(data->focusables);

		/* Check if an actor was already marked as selected and print a warning
		 * if now multiple actors are marked as selected.
		 */
		if(subTagData->tag.focus.selected)
		{
			EsdashboardThemeLayoutPrivate		*priv;

			priv=data->self->priv;

			/* Check if any focus was already set and print a warning */
			if(priv->focusSelected)
			{
				g_warning("File %s - Warning on line %d char %d: At interface '%s' the ID '%s' should get focus but the ID '%s' was selected already",
							data->currentPath,
							data->lastLine,
							data->lastPosition,
							data->interface->id,
							subTagData->tag.focus.refID,
							priv->focusSelected->tag.focus.refID);
				ESDASHBOARD_DEBUG(data->self, THEME,
									"In file '%s' at interface '%s' the ID '%s' should get focus but the ID '%s' was selected already",
									data->currentPath,
									data->interface->id,
									subTagData->tag.focus.refID,
									priv->focusSelected->tag.focus.refID);
			}
				/* Remember actor as pre-selected focus */
				else
				{
					priv->focusSelected=_esdashboard_theme_layout_tag_data_ref(subTagData);
				}
		}

		/* Add focusable actor to parser data */
		g_ptr_array_add(data->focusables, _esdashboard_theme_layout_tag_data_ref(subTagData));
		ESDASHBOARD_DEBUG(data->self, THEME,
							"Adding focusable actor referenced by ID '%s' to parser data",
							subTagData->tag.focus.refID);
	}

	/* Handle end of element <interface> */
	if(subTagData->tagType==TAG_INTERFACE)
	{
		/* Take reference on focusable actors list if available */
		if(data->focusables)
		{
			g_assert(data->interface);
			g_assert(!data->interface->focusables);

			data->interface->focusables=g_ptr_array_ref(data->focusables);
			ESDASHBOARD_DEBUG(data->self, THEME,
								"Will resolve %d focusable actor IDs to interface '%s'",
								data->interface->focusables->len,
								data->interface->id);
		}
	}

	/* Unreference last tag's data */
	_esdashboard_theme_layout_tag_data_unref(subTagData);
}

/* Check for duplicate IDs and unresolvable refIDs */
static void _esdashboard_theme_layout_check_refids(gpointer inData, gpointer inUserData)
{
	EsdashboardThemeLayoutParsedObject	*object;
	EsdashboardThemeLayoutCheckRefID	*checkRefID;
	gpointer							value;
	GSList								*entry;
	EsdashboardThemeLayoutTagData		*property;

	g_return_if_fail(inData);
	g_return_if_fail(inUserData);

	object=(EsdashboardThemeLayoutParsedObject*)inData;
	checkRefID=(EsdashboardThemeLayoutCheckRefID*)inUserData;

	/* Iterate through properties and check referenced IDs */
	for(entry=object->properties; entry; entry=g_slist_next(entry))
	{
		property=(EsdashboardThemeLayoutTagData*)entry->data;

		/* If property references an ID then lookup key in hashtable.
		 * If not found create key with ID and value of 1.
		 * If found do nothing (keep value at zero).
		 */
		if(property->tag.property.refID &&
			!g_hash_table_lookup_extended(checkRefID->ids, property->tag.property.refID, NULL, &value))
		{
			g_hash_table_insert(checkRefID->ids, property->tag.property.refID, GINT_TO_POINTER(1));
			ESDASHBOARD_DEBUG(checkRefID->self, THEME,
								"Could not resolve referenced ID '%s', set counter to 1",
								property->tag.property.refID);
		}
			else if(property->tag.property.refID)
			{
				ESDASHBOARD_DEBUG(checkRefID->self, THEME,
									"Referenced ID '%s' resolved successfully",
									property->tag.property.refID);
			}
	}

	/* Call ourselve for each constraint object */
	g_slist_foreach(object->constraints, _esdashboard_theme_layout_check_refids, inUserData);

	/* Call ourselve for layout object */
	if(object->layout) _esdashboard_theme_layout_check_refids(object->layout, inUserData);

	/* Call ourselve for each child object */
	g_slist_foreach(object->children, _esdashboard_theme_layout_check_refids, inUserData);
}

static void _esdashboard_theme_layout_check_ids(gpointer inData, gpointer inUserData)
{
	EsdashboardThemeLayoutParsedObject	*object;
	EsdashboardThemeLayoutCheckRefID	*checkRefID;
	gpointer							value;

	g_return_if_fail(inData);
	g_return_if_fail(inUserData);

	object=(EsdashboardThemeLayoutParsedObject*)inData;
	checkRefID=(EsdashboardThemeLayoutCheckRefID*)inUserData;

	/* If ID at object is set then lookup key in hashtable.
	 * If not found create key with ID and value of 1.
	 * If found increase value.
	 */
	if(object->id)
	{
		/* If key does not exist create it with value of 1 ... */
		if(!g_hash_table_lookup_extended(checkRefID->ids, object->id, NULL, &value))
		{
			g_hash_table_insert(checkRefID->ids, object->id, GINT_TO_POINTER(1));
			ESDASHBOARD_DEBUG(checkRefID->self, THEME,
								"First occurence of ID '%s', set counter to 1",
								object->id);
		}
			/* ... otherwise increase value of key */
			else
			{
				gint					count;

				count=GPOINTER_TO_INT(value);
				count++;
				g_hash_table_replace(checkRefID->ids, object->id, GINT_TO_POINTER(count));
				ESDASHBOARD_DEBUG(checkRefID->self, THEME,
									"Found ID '%s' and increased counter to %d",
									object->id,
									count);
			}
	}

	/* Call ourselve for each constraint object */
	g_slist_foreach(object->constraints, _esdashboard_theme_layout_check_ids, inUserData);

	/* Call ourselve for layout object */
	if(object->layout) _esdashboard_theme_layout_check_ids(object->layout, inUserData);

	/* Call ourselve for each child object */
	g_slist_foreach(object->children, _esdashboard_theme_layout_check_ids, inUserData);
}

static gboolean _esdashboard_theme_layout_check_ids_and_refids(EsdashboardThemeLayout *self,
																EsdashboardThemeLayoutParsedObject *inInterfaceObject,
																GError **outError)
{
	gboolean							success;
	GHashTableIter						iter;
	gpointer							key, value;
	EsdashboardThemeLayoutCheckRefID	checkRefID;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_LAYOUT(self), FALSE);
	g_return_val_if_fail(inInterfaceObject, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	success=TRUE;

	/* The idea to check for duplicate IDs and if all referenced IDs are
	 * resolvable is a two-step full iteration over all objects with help
	 * of a hashtable which uses the ID as key and the value indicates
	 * check results.
	 */
	checkRefID.self=g_object_ref(self);
	checkRefID.ids=g_hash_table_new(g_str_hash, g_str_equal);

	/* Step one: Iterate through all objects and their contraints, layouts
	 * and children recursively beginning at interface object. For each ID
	 * specified lookup key (containing ID) in hashtable. If key is found
	 * increase counter (which is an error because ID is used more than once)
	 * or create key with integer value 1.
	 */
	_esdashboard_theme_layout_check_ids(inInterfaceObject, &checkRefID);

	/* Check if any key in hashtable has a value greater than one which means
	 * that this ID is used more than once and is an error. Reset each key
	 * check successfully to zero.
	 */
	g_hash_table_iter_init(&iter, checkRefID.ids);
	while(success && g_hash_table_iter_next(&iter, &key, &value)) 
	{
		if(GPOINTER_TO_INT(value)>1)
		{
			g_set_error(outError,
						ESDASHBOARD_THEME_LAYOUT_ERROR,
						ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
						"ID '%s' was specified more than once (%d times)",
						(const gchar*)key,
						GPOINTER_TO_INT(value));
			success=FALSE;
		}
			else
			{
				g_hash_table_iter_replace(&iter, GINT_TO_POINTER(0));
			}
	}

	/* Step two: Iterate through all properties of all objects and their
	 * children recursively. For each property using a reference ID lookup
	 * key (containing ID) in hashtable. If found do nothing and if was
	 * not found create key with integer value 1.
	 */
	if(success) _esdashboard_theme_layout_check_refids(inInterfaceObject, &checkRefID);

	/* Check if any key in hashtable has a value greater than zero which
	 * means that this referenced ID could not be resolved and is an error.
	 */
	if(success)
	{
		g_hash_table_iter_init(&iter, checkRefID.ids);
		while(success && g_hash_table_iter_next(&iter, &key, &value)) 
		{
			if(GPOINTER_TO_INT(value)>0)
			{
				g_set_error(outError,
							ESDASHBOARD_THEME_LAYOUT_ERROR,
							ESDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
							"Referenced ID '%s' could not be resolved",
							(const gchar*)key);
				success=FALSE;
			}
		}
	}

	/* Release allocated resources */
	g_hash_table_destroy(checkRefID.ids);
	g_object_unref(checkRefID.self);

	/* Return result of checks */
	return(success);
}

/* Parse XML from string */
static gboolean _esdashboard_theme_layout_parse_xml(EsdashboardThemeLayout *self,
													const gchar *inPath,
													const gchar *inContents,
													GError **outError)
{
	static GMarkupParser				parser=
										{
											_esdashboard_theme_layout_parse_general_start,
											_esdashboard_theme_layout_parse_general_end,
											_esdashboard_theme_layout_parse_general_no_text_nodes,
											NULL,
											NULL,
										};

	EsdashboardThemeLayoutPrivate		*priv;
	EsdashboardThemeLayoutParserData	*data;
	GMarkupParseContext					*context;
	GError								*error;
	gboolean							success;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_LAYOUT(self), FALSE);
	g_return_val_if_fail(inPath && *inPath, FALSE);
	g_return_val_if_fail(inContents && *inContents, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;
	success=TRUE;

	/* Create and set up parser instance */
	data=g_new0(EsdashboardThemeLayoutParserData, 1);
	if(!data)
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_THEME_LAYOUT_ERROR,
					ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
					"Could not set up parser data for file %s",
					inPath);
		return(FALSE);
	}

	context=g_markup_parse_context_new(&parser, 0, data, NULL);
	if(!context)
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_THEME_LAYOUT_ERROR,
					ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
					"Could not create parser for file %s",
					inPath);

		g_free(data);
		return(FALSE);
	}

	/* Now the parser and its context is set up and we can now
	 * safely initialize data.
	 */
	data->self=self;
	data->stackObjects=g_queue_new();
	data->stackTags=g_queue_new();
	data->lastLine=1;
	data->lastPosition=1;
	data->currentLine=1;
	data->currentPostition=1;
	data->currentPath=inPath;

	/* Parse XML string */
	if(success && !g_markup_parse_context_parse(context, inContents, -1, &error))
	{
		g_propagate_error(outError, error);
		success=FALSE;
	}

	if(success && !g_markup_parse_context_end_parse(context, &error))
	{
		g_prefix_error(&error,
						"File %s - ",
						inPath);
		g_propagate_error(outError, error);
		success=FALSE;
	}

	if(success && !data->interface)
	{
		g_set_error(outError,
					ESDASHBOARD_THEME_LAYOUT_ERROR,
					ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
					"File %s does not contain an interface",
					inPath);
		success=FALSE;
	}

	if(success && !data->interface->id)
	{
		g_set_error(outError,
					ESDASHBOARD_THEME_LAYOUT_ERROR,
					ESDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
					"Interface at file %s has no ID",
					inPath);
		success=FALSE;
	}

	if(success && !_esdashboard_theme_layout_check_ids_and_refids(self, data->interface, &error))
	{
		g_propagate_error(outError, error);
		success=FALSE;
	}

	/* Handle collected data if parsing was successful */
	if(success)
	{
		priv->interfaces=g_slist_append(priv->interfaces, _esdashboard_theme_layout_object_data_ref(data->interface));
	}

	/* Release allocated resources */
	g_markup_parse_context_free(context);

	if(data->interface) _esdashboard_theme_layout_object_data_unref(data->interface);

	if(!g_queue_is_empty(data->stackObjects))
	{
		g_assert(!success);

		g_queue_foreach(data->stackObjects, (GFunc)_esdashboard_theme_layout_object_data_free_foreach_callback, self);
	}
	g_queue_free(data->stackObjects);

	if(!g_queue_is_empty(data->stackTags))
	{
		g_assert(!success);

		g_queue_foreach(data->stackTags, (GFunc)_esdashboard_theme_layout_tag_data_free_foreach_callback, self);
	}
	g_queue_free(data->stackTags);

	if(data->focusables) g_ptr_array_unref(data->focusables);

	g_free(data);

	/* Return success result */
#ifdef DEBUG
	if(!success)
	{
		g_slist_foreach(priv->interfaces, (GFunc)_esdashboard_theme_layout_print_parsed_objects, "Interface:");
		ESDASHBOARD_DEBUG(self, THEME,
							"PARSER ERROR: %s",
							outError ? (*outError)->message : "unknown error");
	}
#endif

	return(success);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_theme_layout_dispose(GObject *inObject)
{
	EsdashboardThemeLayout				*self=ESDASHBOARD_THEME_LAYOUT(inObject);
	EsdashboardThemeLayoutPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->focusSelected)
	{
		_esdashboard_theme_layout_tag_data_unref(priv->focusSelected);
		priv->focusSelected=NULL;
	}

	if(priv->interfaces)
	{
		g_slist_foreach(priv->interfaces, _esdashboard_theme_layout_object_data_free_foreach_callback, NULL);
		g_slist_free(priv->interfaces);
		priv->interfaces=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_theme_layout_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_theme_layout_class_init(EsdashboardThemeLayoutClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_theme_layout_dispose;
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_theme_layout_init(EsdashboardThemeLayout *self)
{
	EsdashboardThemeLayoutPrivate		*priv;

	priv=self->priv=esdashboard_theme_layout_get_instance_private(self);

	/* Set default values */
	priv->interfaces=NULL;
	priv->focusSelected=NULL;
}

/* IMPLEMENTATION: Errors */

GQuark esdashboard_theme_layout_error_quark(void)
{
	return(g_quark_from_static_string("esdashboard-theme-layout-error-quark"));
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
EsdashboardThemeLayout* esdashboard_theme_layout_new(void)
{
	return(ESDASHBOARD_THEME_LAYOUT(g_object_new(ESDASHBOARD_TYPE_THEME_LAYOUT, NULL)));
}

/* Load a XML file into theme */
gboolean esdashboard_theme_layout_add_file(EsdashboardThemeLayout *self,
											const gchar *inPath,
											GError **outError)
{
	gchar								*contents;
	gsize								contentsLength;
	GError								*error;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_LAYOUT(self), FALSE);
	g_return_val_if_fail(inPath!=NULL && *inPath!=0, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	/* Load XML file, parse it and build objects from file */
	error=NULL;
	if(!g_file_get_contents(inPath, &contents, &contentsLength, &error))
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	_esdashboard_theme_layout_parse_xml(self, inPath, contents, &error);
	if(error)
	{
		g_propagate_error(outError, error);
		g_free(contents);
		return(FALSE);
	}

	/* Release allocated resources */
	g_free(contents);

	/* If we get here loading and parsing XML file was successful
	 * so return TRUE here
	 */
	return(TRUE);
}

/* Build requested interface */
ClutterActor* esdashboard_theme_layout_build_interface(EsdashboardThemeLayout *self,
														const gchar *inID,
														...)
{
	EsdashboardThemeLayoutPrivate				*priv;
	GSList										*iter;
	EsdashboardThemeLayoutParsedObject			*interfaceData;
	GObject										*actor;
	GHashTable									*ids;
	GSList										*unresolved;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME_LAYOUT(self), NULL);
	g_return_val_if_fail(inID!=NULL && *inID!=0, NULL);

	priv=self->priv;
	interfaceData=NULL;

	/* Find parsed object data for requested interface by its ID */
	iter=priv->interfaces;
	while(iter && !interfaceData)
	{
		EsdashboardThemeLayoutParsedObject		*objectData;

		/* Get interface object data currently iterated */
		objectData=(EsdashboardThemeLayoutParsedObject*)iter->data;

		/* Check if this object data is for the requested interface */
		if(g_strcmp0(objectData->id, inID)==0) interfaceData=objectData;

		/* Continue with next entry */
		iter=g_slist_next(iter);
	}

	/* If we get here and we did not found the requested interface
	 * object data, return NULL to indicate error.
	 */
	if(!interfaceData)
	{
		ESDASHBOARD_DEBUG(self, THEME,
							"Could not find object data for interface '%s'",
							inID);
		return(NULL);
	}

	/* Keep interface object data alive while building interface
	 * by taking an extra reference.
	 */
	_esdashboard_theme_layout_object_data_ref(interfaceData);

	/* Create hash-table to resolve IDs of objects created and
	 * initialize empty list of IDs to resolve.
	 */
	ids=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	unresolved=NULL;

	/* Create actor */
	actor=_esdashboard_theme_layout_create_object(self, interfaceData, ids, &unresolved);
	if(actor)
	{
		/* Check if really an actor was created */
		if(CLUTTER_IS_ACTOR(actor))
		{
			va_list								args;

			ESDASHBOARD_DEBUG(self, THEME,
								"Created actor %s for interface '%s'",
								G_OBJECT_TYPE_NAME(actor),
								inID);

			/* Resolved unresolved properties of newly created object */
			va_start(args, inID);
			_esdashboard_theme_layout_create_object_resolve_unresolved(self, ids, unresolved, args);
			va_end(args);
		}
			else
			{
				ESDASHBOARD_DEBUG(self, THEME,
									"Failed to create actor for interface '%s' because object of type %s is not derived from %s",
									inID,
									G_OBJECT_TYPE_NAME(actor),
									g_type_name(CLUTTER_TYPE_ACTOR));

				/* Release object which is not an actor */
				g_object_unref(actor);
				actor=NULL;
			}
	}
		else
		{
			ESDASHBOARD_DEBUG(self, THEME,
								"Failed to create actor for interface '%s'",
								inID);
		}

	/* Release allocated resources */
	if(ids) g_hash_table_destroy(ids);
	if(unresolved) g_slist_free_full(unresolved, _esdashboard_theme_layout_create_object_free_unresolved);

	/* Release extra reference taken at interface object data */
	_esdashboard_theme_layout_object_data_unref(interfaceData);

	/* Return created actor */
	return(actor ? CLUTTER_ACTOR(actor) : NULL);
}
