/*
 * theme: Top-level theme object (parses key file and manages loading
 *        resources like css style files, xml layout files etc.)
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

#include <libesdashboard/theme.h>

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>

#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
struct _EsdashboardThemePrivate
{
	/* Properties related */
	gchar						*themePath;

	gchar						*themeName;
	gchar						*themeDisplayName;
	gchar						*themeComment;

	/* Instance related */
	gboolean					loaded;

	EsdashboardThemeCSS			*styling;
	EsdashboardThemeLayout		*layout;
	EsdashboardThemeEffects		*effects;
	EsdashboardThemeAnimation	*animation;

	gchar						*userThemeStyleFile;
	gchar						*userGlobalStyleFile;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardTheme,
							esdashboard_theme,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_PATH,

	PROP_NAME,
	PROP_DISPLAY_NAME,
	PROP_COMMENT,

	PROP_LAST
};

static GParamSpec* EsdashboardThemeProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define ESDASHBOARD_THEME_SUBPATH						"esdashboard-1.0"
#define ESDASHBOARD_THEME_FILE							"esdashboard.theme"
#define ESDASHBOARD_USER_GLOBAL_CSS_FILE				"global.css"

#define ESDASHBOARD_THEME_GROUP							"Esdashboard Theme"
#define ESDASHBOARD_THEME_GROUP_KEY_NAME				"Name"
#define ESDASHBOARD_THEME_GROUP_KEY_COMMENT				"Comment"
#define ESDASHBOARD_THEME_GROUP_KEY_STYLE				"Style"
#define ESDASHBOARD_THEME_GROUP_KEY_LAYOUT				"Layout"
#define ESDASHBOARD_THEME_GROUP_KEY_EFFECTS				"Effects"
#define ESDASHBOARD_THEME_GROUP_KEY_ANIMATIONS			"Animations"


/* Load theme file and all listed resources in this file */
static gboolean _esdashboard_theme_load_resources(EsdashboardTheme *self,
													GError **outError)
{
	EsdashboardThemePrivate		*priv;
	GError						*error;
	gchar						*themeFile;
	GKeyFile					*themeKeyFile;
	gchar						**resources, **resource;
	gchar						*resourceFile;
	gint						counter;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Check that theme was found */
	if(!priv->themePath)
	{
		/* Set error */
		g_set_error(outError,
					ESDASHBOARD_THEME_ERROR,
					ESDASHBOARD_THEME_ERROR_THEME_NOT_FOUND,
					"Theme '%s' not found",
					priv->themeName);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Load theme file */
	themeFile=g_build_filename(priv->themePath, ESDASHBOARD_THEME_FILE, NULL);

	themeKeyFile=g_key_file_new();
	if(!g_key_file_load_from_file(themeKeyFile,
									themeFile,
									G_KEY_FILE_NONE,
									&error))
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeFile) g_free(themeFile);
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	g_free(themeFile);

	/* Get display name and notify about property change (regardless of success result) */
	priv->themeDisplayName=g_key_file_get_locale_string(themeKeyFile,
														ESDASHBOARD_THEME_GROUP,
														ESDASHBOARD_THEME_GROUP_KEY_NAME,
														NULL,
														&error);
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardThemeProperties[PROP_DISPLAY_NAME]);

	if(!priv->themeDisplayName)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Get comment and notify about property change (regardless of success result) */
	priv->themeComment=g_key_file_get_locale_string(themeKeyFile,
														ESDASHBOARD_THEME_GROUP,
														ESDASHBOARD_THEME_GROUP_KEY_COMMENT,
														NULL,
														&error);
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardThemeProperties[PROP_COMMENT]);

	if(!priv->themeComment)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Create CSS parser, load style resources first and user stylesheets (theme
	 * unrelated "global.css" and theme related "user-[THEME_NAME].css" in this
	 * order) at last to allow user to override theme styles.
	 */
	resources=g_key_file_get_string_list(themeKeyFile,
											ESDASHBOARD_THEME_GROUP,
											ESDASHBOARD_THEME_GROUP_KEY_STYLE,
											NULL,
											&error);
	if(!resources)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	counter=0;
	resource=resources;
	while(*resource)
	{
		/* Get path and file for style resource */
		resourceFile=g_build_filename(priv->themePath, *resource, NULL);

		/* Try to load style resource */
		ESDASHBOARD_DEBUG(self, THEME,
							"Loading CSS file %s for theme %s with priority %d",
							resourceFile,
							priv->themeName,
							counter);

		if(!esdashboard_theme_css_add_file(priv->styling, resourceFile, counter, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(resources) g_strfreev(resources);
			if(resourceFile) g_free(resourceFile);
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		/* Release allocated resources */
		if(resourceFile) g_free(resourceFile);

		/* Continue with next entry */
		resource++;
		counter++;
	}
	g_strfreev(resources);

	if(priv->userGlobalStyleFile)
	{
		ESDASHBOARD_DEBUG(self, THEME,
							"Loading user's global CSS file %s for theme %s with priority %d",
							priv->userGlobalStyleFile,
							priv->themeName,
							counter);

		/* Load user's theme unrelated (global) stylesheet as it exists */
		if(!esdashboard_theme_css_add_file(priv->styling, priv->userGlobalStyleFile, counter, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		/* Increase counter used for CSS priority for next user CSS file */
		counter++;
	}

	if(priv->userThemeStyleFile)
	{
		ESDASHBOARD_DEBUG(self, THEME,
							"Loading user's theme CSS file %s for theme %s with priority %d",
							priv->userThemeStyleFile,
							priv->themeName,
							counter);

		/* Load user's theme related stylesheet as it exists */
		if(!esdashboard_theme_css_add_file(priv->styling, priv->userThemeStyleFile, counter, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		/* Increase counter used for CSS priority for next user CSS file */
		counter++;
	}

	/* Create XML parser and load layout resources */
	resources=g_key_file_get_string_list(themeKeyFile,
											ESDASHBOARD_THEME_GROUP,
											ESDASHBOARD_THEME_GROUP_KEY_LAYOUT,
											NULL,
											&error);
	if(!resources)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	resource=resources;
	while(*resource)
	{
		/* Get path and file for style resource */
		resourceFile=g_build_filename(priv->themePath, *resource, NULL);

		/* Try to load layout resource */
		ESDASHBOARD_DEBUG(self, THEME,
							"Loading XML layout file %s for theme %s",
							resourceFile,
							priv->themeName);

		if(!esdashboard_theme_layout_add_file(priv->layout, resourceFile, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(resources) g_strfreev(resources);
			if(resourceFile) g_free(resourceFile);
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		/* Release allocated resources */
		if(resourceFile) g_free(resourceFile);

		/* Continue with next entry */
		resource++;
		counter++;
	}
	g_strfreev(resources);

	/* Create XML parser and load effect resources which are optional */
	if(g_key_file_has_key(themeKeyFile,
							ESDASHBOARD_THEME_GROUP,
							ESDASHBOARD_THEME_GROUP_KEY_EFFECTS,
							NULL))
	{
		resources=g_key_file_get_string_list(themeKeyFile,
												ESDASHBOARD_THEME_GROUP,
												ESDASHBOARD_THEME_GROUP_KEY_EFFECTS,
												NULL,
												&error);
		if(!resources)
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		resource=resources;
		while(*resource)
		{
			/* Get path and file for effect resource */
			resourceFile=g_build_filename(priv->themePath, *resource, NULL);

			/* Try to load effects resource */
			ESDASHBOARD_DEBUG(self, THEME,
								"Loading XML effects file %s for theme %s",
								resourceFile,
								priv->themeName);

			if(!esdashboard_theme_effects_add_file(priv->effects, resourceFile, &error))
			{
				/* Set error */
				g_propagate_error(outError, error);

				/* Release allocated resources */
				if(resources) g_strfreev(resources);
				if(resourceFile) g_free(resourceFile);
				if(themeKeyFile) g_key_file_free(themeKeyFile);

				/* Return FALSE to indicate error */
				return(FALSE);
			}

			/* Release allocated resources */
			if(resourceFile) g_free(resourceFile);

			/* Continue with next entry */
			resource++;
			counter++;
		}
		g_strfreev(resources);
	}

	/* Create XML parser and load animation resources which are optional */
	if(g_key_file_has_key(themeKeyFile,
							ESDASHBOARD_THEME_GROUP,
							ESDASHBOARD_THEME_GROUP_KEY_ANIMATIONS,
							NULL))
	{
		resources=g_key_file_get_string_list(themeKeyFile,
												ESDASHBOARD_THEME_GROUP,
												ESDASHBOARD_THEME_GROUP_KEY_ANIMATIONS,
												NULL,
												&error);
		if(!resources)
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		resource=resources;
		while(*resource)
		{
			/* Get path and file for animation resource */
			resourceFile=g_build_filename(priv->themePath, *resource, NULL);

			/* Try to load animation resource */
			ESDASHBOARD_DEBUG(self, THEME,
								"Loading XML animation file %s for theme %s",
								resourceFile,
								priv->themeName);

			if(!esdashboard_theme_animation_add_file(priv->animation, resourceFile, &error))
			{
				/* Set error */
				g_propagate_error(outError, error);

				/* Release allocated resources */
				if(resources) g_strfreev(resources);
				if(resourceFile) g_free(resourceFile);
				if(themeKeyFile) g_key_file_free(themeKeyFile);

				/* Return FALSE to indicate error */
				return(FALSE);
			}

			/* Release allocated resources */
			if(resourceFile) g_free(resourceFile);

			/* Continue with next entry */
			resource++;
			counter++;
		}
		g_strfreev(resources);
	}

	/* Release allocated resources */
	if(themeKeyFile) g_key_file_free(themeKeyFile);

	/* Return TRUE to indicate success */
	return(TRUE);
}

/* Lookup path for named theme.
 * Caller must free returned path with g_free if not needed anymore.
 */
static gchar* _esdashboard_theme_lookup_path_for_theme(EsdashboardTheme *self,
														const gchar *inThemeName)
{
	gchar				*themeFile;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), FALSE);
	g_return_val_if_fail(inThemeName!=NULL && *inThemeName!=0, FALSE);

	themeFile=NULL;

	/* Search theme file in given environment variable if set.
	 * This makes development easier when theme changes are needed
	 * without changing theme or changing symlinks in any of below
	 * searched paths.
	 */
	if(!themeFile)
	{
		const gchar		*envPath;

		envPath=g_getenv("ESDASHBOARD_THEME_PATH");
		if(envPath)
		{
			themeFile=g_build_filename(envPath, ESDASHBOARD_THEME_FILE, NULL);
			ESDASHBOARD_DEBUG(self, THEME,
								"Trying theme file: %s",
								themeFile);
			if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_free(themeFile);
				themeFile=NULL;
			}
		}
	}

	/* If file not found search in user's config directory */
	if(!themeFile)
	{
		themeFile=g_build_filename(g_get_user_data_dir(), "themes", inThemeName, ESDASHBOARD_THEME_SUBPATH, ESDASHBOARD_THEME_FILE, NULL);
		ESDASHBOARD_DEBUG(self, THEME,
							"Trying theme file: %s",
							themeFile);
		if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			g_free(themeFile);
			themeFile=NULL;
		}
	}

	/* If file not found search in user's home directory */
	if(!themeFile)
	{
		const gchar		*homeDirectory;

		homeDirectory=g_get_home_dir();
		if(homeDirectory)
		{
			themeFile=g_build_filename(homeDirectory, ".themes", inThemeName, ESDASHBOARD_THEME_SUBPATH, ESDASHBOARD_THEME_FILE, NULL);
			ESDASHBOARD_DEBUG(self, THEME,
								"Trying theme file: %s",
								themeFile);
			if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_free(themeFile);
				themeFile=NULL;
			}
		}
	}

	/* If file not found search in system-wide paths */
	if(!themeFile)
	{
		themeFile=g_build_filename(PACKAGE_DATADIR, "themes", inThemeName, ESDASHBOARD_THEME_SUBPATH, ESDASHBOARD_THEME_FILE, NULL);
		ESDASHBOARD_DEBUG(self, THEME,
							"Trying theme file: %s",
							themeFile);
		if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			g_free(themeFile);
			themeFile=NULL;
		}
	}

	/* If file was found get path contaning file and return it */
	if(themeFile)
	{
		gchar			*themePath;

		themePath=g_path_get_dirname(themeFile);
		g_free(themeFile);

		return(themePath);
	}

	/* If we get here theme was not found so return NULL */
	return(NULL);
}

/* Theme's name was set so lookup pathes and initialize but do not load resources */
static void _esdashboard_theme_set_theme_name(EsdashboardTheme *self, const gchar *inThemeName)
{
	EsdashboardThemePrivate		*priv;
	gchar						*themePath;
	gchar						*resourceFile;
	gchar						*userThemeStylesheet;

	g_return_if_fail(ESDASHBOARD_IS_THEME(self));
	g_return_if_fail(inThemeName && *inThemeName);

	priv=self->priv;

	/* The theme name must not be set already */
	if(priv->themeName)
	{
		/* Show error message */
		g_critical("Cannot change theme name to '%s' because it is already set to '%s'",
					inThemeName,
					priv->themeName);

		return;
	}

	/* Lookup path of theme by lookup at all possible paths for theme file */
	themePath=_esdashboard_theme_lookup_path_for_theme(self, inThemeName);
	if(!themePath)
	{
		g_critical("Theme '%s' not found", inThemeName);

		/* Return here because looking up path failed */
		return;
	}

	/* Set up internal variable and notify about property changes */
	priv->themeName=g_strdup(inThemeName);
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardThemeProperties[PROP_NAME]);

	priv->themePath=g_strdup(themePath);
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardThemeProperties[PROP_PATH]);

	/* Initialize theme resources */
	priv->styling=esdashboard_theme_css_new(priv->themePath);
	priv->layout=esdashboard_theme_layout_new();
	priv->effects=esdashboard_theme_effects_new();
	priv->animation=esdashboard_theme_animation_new();

	/* Check for user resource files */
	resourceFile=g_build_filename(g_get_user_config_dir(), "esdashboard", "themes", ESDASHBOARD_USER_GLOBAL_CSS_FILE, NULL);
	if(g_file_test(resourceFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		priv->userGlobalStyleFile=g_strdup(resourceFile);
	}
		else
		{
			ESDASHBOARD_DEBUG(self, THEME,
								"No user global stylesheet found at %s for theme %s - skipping",
								resourceFile,
								priv->themeName);
		}
	g_free(resourceFile);

	userThemeStylesheet=g_strdup_printf("user-%s.css", priv->themeName);
	resourceFile=g_build_filename(g_get_user_config_dir(), "esdashboard", "themes", userThemeStylesheet, NULL);
	if(g_file_test(resourceFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		priv->userThemeStyleFile=g_strdup(resourceFile);
	}
		else
		{
			ESDASHBOARD_DEBUG(self, THEME,
								"No user theme stylesheet found at %s for theme %s - skipping",
								resourceFile,
								priv->themeName);
		}
	g_free(resourceFile);

	/* Release allocated resources */
	if(themePath) g_free(themePath);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_theme_dispose(GObject *inObject)
{
	EsdashboardTheme			*self=ESDASHBOARD_THEME(inObject);
	EsdashboardThemePrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->themeName)
	{
		g_free(priv->themeName);
		priv->themeName=NULL;
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardThemeProperties[PROP_NAME]);
	}

	if(priv->themePath)
	{
		g_free(priv->themePath);
		priv->themePath=NULL;
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardThemeProperties[PROP_PATH]);
	}

	if(priv->themeDisplayName)
	{
		g_free(priv->themeDisplayName);
		priv->themeDisplayName=NULL;
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardThemeProperties[PROP_DISPLAY_NAME]);
	}

	if(priv->themeComment)
	{
		g_free(priv->themeComment);
		priv->themeComment=NULL;
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardThemeProperties[PROP_COMMENT]);
	}

	if(priv->userThemeStyleFile)
	{
		g_free(priv->userThemeStyleFile);
		priv->userThemeStyleFile=NULL;
	}

	if(priv->userGlobalStyleFile)
	{
		g_free(priv->userGlobalStyleFile);
		priv->userGlobalStyleFile=NULL;
	}

	if(priv->styling)
	{
		g_object_unref(priv->styling);
		priv->styling=NULL;
	}

	if(priv->layout)
	{
		g_object_unref(priv->layout);
		priv->layout=NULL;
	}

	if(priv->effects)
	{
		g_object_unref(priv->effects);
		priv->effects=NULL;
	}

	if(priv->animation)
	{
		g_object_unref(priv->animation);
		priv->animation=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_theme_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_theme_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	EsdashboardTheme			*self=ESDASHBOARD_THEME(inObject);

	switch(inPropID)
	{
		case PROP_NAME:
			_esdashboard_theme_set_theme_name(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_theme_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	EsdashboardTheme			*self=ESDASHBOARD_THEME(inObject);
	EsdashboardThemePrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_PATH:
			g_value_set_string(outValue, priv->themePath);
			break;

		case PROP_NAME:
			g_value_set_string(outValue, priv->themeName);
			break;

		case PROP_DISPLAY_NAME:
			g_value_set_string(outValue, priv->themeDisplayName);
			break;

		case PROP_COMMENT:
			g_value_set_string(outValue, priv->themeComment);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void esdashboard_theme_class_init(EsdashboardThemeClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_theme_dispose;
	gobjectClass->set_property=_esdashboard_theme_set_property;
	gobjectClass->get_property=_esdashboard_theme_get_property;

	/* Define properties */
	EsdashboardThemeProperties[PROP_NAME]=
		g_param_spec_string("theme-name",
							"Theme name",
							"Short name of theme which was used to lookup theme and folder name where theme is stored in",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	EsdashboardThemeProperties[PROP_PATH]=
		g_param_spec_string("theme-path",
							"Theme path",
							"Path where theme was found and loaded from",
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	EsdashboardThemeProperties[PROP_DISPLAY_NAME]=
		g_param_spec_string("theme-display-name",
							"Theme display name",
							"The name of theme",
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	EsdashboardThemeProperties[PROP_COMMENT]=
		g_param_spec_string("theme-comment",
							"Theme comment",
							"The comment of theme used as description",
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardThemeProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_theme_init(EsdashboardTheme *self)
{
	EsdashboardThemePrivate		*priv;

	priv=self->priv=esdashboard_theme_get_instance_private(self);

	/* Set default values */
	priv->loaded=FALSE;
	priv->themeName=NULL;
	priv->themePath=NULL;
	priv->themeDisplayName=NULL;
	priv->themeComment=NULL;
	priv->userThemeStyleFile=NULL;
	priv->userGlobalStyleFile=NULL;
	priv->styling=NULL;
	priv->layout=NULL;
	priv->effects=NULL;
	priv->animation=NULL;
}

/* IMPLEMENTATION: Errors */

GQuark esdashboard_theme_error_quark(void)
{
	return(g_quark_from_static_string("esdashboard-theme-error-quark"));
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
EsdashboardTheme* esdashboard_theme_new(const gchar *inThemeName)
{
	return(ESDASHBOARD_THEME(g_object_new(ESDASHBOARD_TYPE_THEME,
											"theme-name", inThemeName,
											NULL)));
}

/* Get path where this theme was found and loaded from */
const gchar* esdashboard_theme_get_path(EsdashboardTheme *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themePath);
}

/* Get theme name (as used when loading theme) */
const gchar* esdashboard_theme_get_theme_name(EsdashboardTheme *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themeName);
}

/* Get display name of theme */
const gchar* esdashboard_theme_get_display_name(EsdashboardTheme *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themeDisplayName);
}

/* Get comment of theme */
const gchar* esdashboard_theme_get_comment(EsdashboardTheme *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themeComment);
}

/* Lookup named theme and load resources */
gboolean esdashboard_theme_load(EsdashboardTheme *self,
								GError **outError)
{
	EsdashboardThemePrivate		*priv;
	GError						*error;

	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Check if a theme was already loaded */
	if(priv->loaded)
	{
		g_set_error(outError,
					ESDASHBOARD_THEME_ERROR,
					ESDASHBOARD_THEME_ERROR_ALREADY_LOADED,
					"Theme '%s' was already loaded",
					priv->themeName);

		return(FALSE);
	}

	/* We set the loaded flag regardless if loading will be successful or not
	 * because if loading theme fails this object is in undefined state for
	 * re-using it to load theme again.
	 */
	priv->loaded=TRUE;

	/* Load theme key file */
	if(!_esdashboard_theme_load_resources(self, &error))
	{
		/* Set returned error */
		g_propagate_error(outError, error);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* If we found named themed and could load all resources successfully */
	return(TRUE);
}

/* Get theme CSS */
EsdashboardThemeCSS* esdashboard_theme_get_css(EsdashboardTheme *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->styling);
}

/* Get theme layout */
EsdashboardThemeLayout* esdashboard_theme_get_layout(EsdashboardTheme *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->layout);
}

/* Get theme effects */
EsdashboardThemeEffects* esdashboard_theme_get_effects(EsdashboardTheme *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->effects);
}

/* Get theme animation */
EsdashboardThemeAnimation* esdashboard_theme_get_animation(EsdashboardTheme *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->animation);
}
