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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/image-content.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>

#include <libesdashboard/application.h>
#include <libesdashboard/stylable.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


#if GTK_CHECK_VERSION(3, 14 ,0)
#undef USE_GTK_BUILTIN_ICONS
#else
#define USE_GTK_BUILTIN_ICONS 1
#endif


/* Local definitions */
typedef enum /*< skip,prefix=ESDASHBOARD_IMAGE_TYPE >*/
{
	ESDASHBOARD_IMAGE_TYPE_NONE=0,
	ESDASHBOARD_IMAGE_TYPE_FILE,
	ESDASHBOARD_IMAGE_TYPE_ICON_NAME,
	ESDASHBOARD_IMAGE_TYPE_GICON,
} EsdashboardImageType;

/* Define this class in GObject system */
static void _esdashboard_image_content_stylable_iface_init(EsdashboardStylableInterface *iface);

struct _EsdashboardImageContentPrivate
{
	/* Properties related */
	gchar								*key;
	gchar								*missingIconName;

	/* Instance related */
	EsdashboardImageType				type;
	EsdashboardImageContentLoadingState	loadState;
	GtkIconTheme						*iconTheme;
	gchar								*iconName;
	GIcon								*gicon;
	gint								iconSize;

	GList								*actors;

	guint								contentAttachedSignalID;
	guint								contentDetachedSignalID;
	guint								iconThemeChangedSignalID;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardImageContent,
						esdashboard_image_content,
						CLUTTER_TYPE_IMAGE,
						G_ADD_PRIVATE(EsdashboardImageContent)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_STYLABLE, _esdashboard_image_content_stylable_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_KEY,
	PROP_MISSING_ICON_NAME,

	/* From interface: EsdashboardStylable */
	PROP_STYLE_CLASSES,
	PROP_STYLE_PSEUDO_CLASSES,

	PROP_LAST
};

static GParamSpec* EsdashboardImageContentProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_LOADED,
	SIGNAL_LOADING_FAILED,

	SIGNAL_LAST
};

static guint EsdashboardImageContentSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
static GHashTable*	_esdashboard_image_content_cache=NULL;
static guint		_esdashboard_image_content_cache_shutdownSignalID=0;

#define ESDASHBOARD_IMAGE_CONTENT_DEFAULT_FALLBACK_ICON_NAME		"image-missing"

/* Get image from cache if available */
static ClutterImage* _esdashboard_image_content_get_cached_image(const gchar *inKey)
{
	ClutterImage		*image;

	/* If no key is given the image is not stored */
	if(!inKey || *inKey==0) return(NULL);

	/* If we have no hash table no image is cached */
	if(!_esdashboard_image_content_cache) return(NULL);

	/* Lookup key in cache and return image if found */
	if(!g_hash_table_contains(_esdashboard_image_content_cache, inKey)) return(NULL);

	/* Get loaded image and reference it */
	image=CLUTTER_IMAGE(g_hash_table_lookup(_esdashboard_image_content_cache, inKey));
	g_object_ref(image);
	ESDASHBOARD_DEBUG(image, IMAGES,
						"Using cached image '%s' - ref-count is now %d" ,
						inKey,
						G_OBJECT(image)->ref_count);

	return(image);
}

/* Destroy cache hashtable */
static void _esdashboard_image_content_destroy_cache(void)
{
	EsdashboardApplication		*application;
	gint						cacheSize;

	/* Only an existing cache can be destroyed */
	if(!_esdashboard_image_content_cache) return;

	/* Disconnect application "shutdown" signal handler */
	application=esdashboard_application_get_default();
	g_signal_handler_disconnect(application, _esdashboard_image_content_cache_shutdownSignalID);
	_esdashboard_image_content_cache_shutdownSignalID=0;

	/* Destroy cache hashtable */
	cacheSize=g_hash_table_size(_esdashboard_image_content_cache);
	if(cacheSize>0) g_warning("Destroying image cache still containing %d images.", cacheSize);
#ifdef DEBUG
	if(cacheSize>0)
	{
		GHashTableIter						iter;
		gpointer							hashKey, hashValue;
		const gchar							*key;
		EsdashboardImageContent				*content;

		g_hash_table_iter_init(&iter, _esdashboard_image_content_cache);
		while(g_hash_table_iter_next (&iter, &hashKey, &hashValue))
		{
			key=(const gchar*)hashKey;
			content=ESDASHBOARD_IMAGE_CONTENT(hashValue);
			g_print("Image content in cache: Item %s@%p for key '%s' (used by %d actors)\n",
						G_OBJECT_TYPE_NAME(content), content,
						key,
						g_list_length(content->priv->actors));
		}
	}
#endif

	ESDASHBOARD_DEBUG(NULL, IMAGES, "Destroying image cache hashtable");
	g_hash_table_destroy(_esdashboard_image_content_cache);
	_esdashboard_image_content_cache=NULL;
}

/* Create cache hashtable if not already set up */
static void _esdashboard_image_content_create_cache(void)
{
	EsdashboardApplication		*application;

	/* Cache was already set up */
	if(_esdashboard_image_content_cache) return;

	/* Create create hashtable */
	_esdashboard_image_content_cache=g_hash_table_new(g_str_hash, g_str_equal);
	ESDASHBOARD_DEBUG(NULL, IMAGES, "Created image cache hashtable");

	/* Connect to "shutdown" signal of application to
	 * clean up hashtable
	 */
	application=esdashboard_application_get_default();
	_esdashboard_image_content_cache_shutdownSignalID=g_signal_connect(application, "shutdown-final", G_CALLBACK(_esdashboard_image_content_destroy_cache), NULL);
}

/* Remove image from cache */
static void _esdashboard_image_content_remove_from_cache(EsdashboardImageContent *self)
{
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));

	priv=self->priv;

	/* Cannot remove image if cache was not set up yet */
	if(!_esdashboard_image_content_cache) return;

	/* Remove from cache */
	ESDASHBOARD_DEBUG(self, IMAGES,
						"Removing image '%s' with ref-count %d",
						priv->key,
						G_OBJECT(self)->ref_count);
	g_hash_table_remove(_esdashboard_image_content_cache, priv->key);
}

/* Store image in cache */
static void _esdashboard_image_content_store_in_cache(EsdashboardImageContent *self, const gchar *inKey)
{
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));
	g_return_if_fail(inKey && *inKey!=0);

	priv=self->priv;

	/* Create cache hashtable */
	if(!_esdashboard_image_content_cache) _esdashboard_image_content_create_cache();

	/* Set key */
	if(priv->key)
	{
		g_critical("Image has already key '%s' set and will be replaced with '%s'",
					priv->key,
					inKey);
		g_free(priv->key);
		priv->key=NULL;
	}
	priv->key=g_strdup(inKey);

	/* Store image in cache */
	if(g_hash_table_contains(_esdashboard_image_content_cache, priv->key))
	{
		ClutterContent		*content;

		g_critical("An image with key '%s' is already cache and will be replaced.", priv->key);

		/* Unreference current cached image */
		content=CLUTTER_CONTENT(g_hash_table_lookup(_esdashboard_image_content_cache, inKey));
		if(content)
		{
			g_object_unref(content);
			ESDASHBOARD_DEBUG(self, IMAGES,
								"Replacing image '%s' which had ref-count %u",
								priv->key,
								G_OBJECT(content)->ref_count);
		}
	}

	/* Store new image in cache */
	g_hash_table_insert(_esdashboard_image_content_cache, priv->key, self);
	ESDASHBOARD_DEBUG(self, IMAGES,
						"Added image '%s' with ref-count %d",
						priv->key,
						G_OBJECT(self)->ref_count);
}

/* Set an empty image of size 1x1 pixels (e.g. when loading asynchronously) */
static void _esdashboard_image_content_set_empty_image(EsdashboardImageContent *self)
{
	static guchar		empty[]={ 0, 0, 0, 0xff };

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));

	clutter_image_set_data(CLUTTER_IMAGE(self),
							empty,
							COGL_PIXEL_FORMAT_RGBA_8888,
							1, /* width */
							1, /* height */
							1, /* row-stride */
							NULL);
}

/* Callback when loading icon asynchronously has finished */
static void _esdashboard_image_content_loading_async_callback(GObject *inSource, GAsyncResult *inResult, gpointer inUserData)
{
	EsdashboardImageContent				*self=ESDASHBOARD_IMAGE_CONTENT(inUserData);
	EsdashboardImageContentPrivate		*priv=self->priv;
	GdkPixbuf							*pixbuf;
	GError								*error=NULL;

	priv->loadState=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_SUCCESSFULLY;

	/* Get pixbuf loaded */
	pixbuf=gdk_pixbuf_new_from_stream_finish(inResult, &error);
	if(pixbuf)
	{
		/* Set image data into content */
		if(!clutter_image_set_data(CLUTTER_IMAGE(self),
									gdk_pixbuf_get_pixels(pixbuf),
									gdk_pixbuf_get_has_alpha(pixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
									gdk_pixbuf_get_width(pixbuf),
									gdk_pixbuf_get_height(pixbuf),
									gdk_pixbuf_get_rowstride(pixbuf),
									&error))
		{
			g_warning("Failed to load image data into content for key '%s': %s",
						priv->key ? priv->key : "<nil>",
						error ? error->message : "Unknown error");
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}

			/* Set failed state and empty image */
			_esdashboard_image_content_set_empty_image(self);
			priv->loadState=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_FAILED;
		}
	}
		else
		{
			g_warning("Failed to load image for key '%s': %s",
						priv->key ? priv->key : "<nil>",
						error ? error->message : "Unknown error");
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}

			/* Set failed state and empty image */
			_esdashboard_image_content_set_empty_image(self);
			priv->loadState=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_FAILED;
		}

	/* Release allocated resources */
	if(pixbuf) g_object_unref(pixbuf);

	/* Emit "loaded" signal if loading was successful ... */
	if(priv->loadState==ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_SUCCESSFULLY)
	{
		g_signal_emit(self, EsdashboardImageContentSignals[SIGNAL_LOADED], 0);
		ESDASHBOARD_DEBUG(self, IMAGES,
							"Successfully loaded image for key '%s' asynchronously",
							priv->key ? priv->key : "<nil>");
	}
		/* ... or emit "loading-failed" signal if loading has failed. */
		else
		{
			g_signal_emit(self, EsdashboardImageContentSignals[SIGNAL_LOADING_FAILED], 0);
			ESDASHBOARD_DEBUG(self, IMAGES,
								"Failed to load image for key '%s' asynchronously",
								priv->key ? priv->key : "<nil>");
		}

	/* Now release the extra reference we took to keep this instance alive
	 * while loading asynchronously.
	 */
	g_object_unref(self);
}

/* Load image from file */
static void _esdashboard_image_content_load_from_file(EsdashboardImageContent *self)
{
	EsdashboardImageContentPrivate		*priv;
	gchar								*lookupFilename;
	gchar								*filename;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));

	priv=self->priv;
	filename=NULL;

	/* Check if type of image is valid and all needed parameters are set */
	g_return_if_fail(priv->type==ESDASHBOARD_IMAGE_TYPE_FILE);
	g_return_if_fail(priv->iconName);
	g_return_if_fail(priv->iconSize>0);

	/* If path of icon filename is relative build absolute path by prefixing theme path ... */
	if(!g_path_is_absolute(priv->iconName))
	{
		EsdashboardTheme				*theme;
		const gchar						*themePath;

		/* Get theme path */
		theme=esdashboard_application_get_theme(NULL);
		g_object_ref(theme);

		themePath=esdashboard_theme_get_path(theme);

		/* Build absolute path from theme path and given relative path to icon */
		lookupFilename=g_build_filename(themePath, priv->iconName, NULL);

		/* Release allocated resources */
		g_object_unref(theme);
	}
		/* ... otherwise it is an absolute path already so just copy it */
		else lookupFilename=g_strdup(priv->iconName);

	/* If file does not exists then load fallback icon */
	if(!g_file_test(lookupFilename, G_FILE_TEST_EXISTS))
	{
		GtkIconInfo						*iconInfo;

		g_warning("Icon file '%s' does not exist - trying fallback icon '%s'",
					priv->iconName,
					priv->missingIconName);

		iconInfo=gtk_icon_theme_lookup_icon(priv->iconTheme,
											priv->missingIconName,
											priv->iconSize,
#ifdef USE_GTK_BUILTIN_ICONS
											GTK_ICON_LOOKUP_USE_BUILTIN);
#else
											0);
#endif

		if(!iconInfo)
		{
			g_error("Could not load fallback icon for file '%s'", priv->iconName);
			_esdashboard_image_content_set_empty_image(self);
			g_free(lookupFilename);
			return;
		}

		/* Check if have to use built-in GdkPixbuf for icon ... */
		filename=g_strdup(gtk_icon_info_get_filename(iconInfo));
#ifdef USE_GTK_BUILTIN_ICONS
		if(!filename)
		{
			GdkPixbuf					*iconPixbuf;
			GError						*error=NULL;

			iconPixbuf=gtk_icon_info_get_builtin_pixbuf(iconInfo);
			if(!clutter_image_set_data(CLUTTER_IMAGE(self),
										gdk_pixbuf_get_pixels(iconPixbuf),
										gdk_pixbuf_get_has_alpha(iconPixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
										gdk_pixbuf_get_width(iconPixbuf),
										gdk_pixbuf_get_height(iconPixbuf),
										gdk_pixbuf_get_rowstride(iconPixbuf),
										&error))
			{
				g_warning("Failed to load image data into content for icon '%s': %s",
							priv->iconName,
							error ? error->message : "Unknown error");
				if(error)
				{
					g_error_free(error);
					error=NULL;
				}
			}
				else
				{
					ESDASHBOARD_DEBUG(self, IMAGES,
										"Loaded fallback icon for file '%s' from built-in pixbuf",
										priv->iconName);
				}

			g_object_unref(iconPixbuf);
		}
#endif

		/* Release allocated resources */
		g_object_unref(iconInfo);
	}
		/* ... otherwise set up to load icon async */
		else filename=g_strdup(lookupFilename);

	/* Load image asynchronously if filename is given */
	if(filename)
	{
		GFile						*file;
		GInputStream				*stream;
		GError						*error=NULL;

		/* Create stream for loading async */
		file=g_file_new_for_path(filename);
		stream=G_INPUT_STREAM(g_file_read(file, NULL, &error));
		if(!stream)
		{
			g_warning("Could not create stream for file '%s' of icon '%s': %s",
						filename,
						priv->iconName,
						error ? error->message : "Unknown error");

			if(error!=NULL)
			{
				g_error_free(error);
				error=NULL;
			}

			/* Release allocated resources */
			g_object_unref(file);
			g_free(lookupFilename);
			g_free(filename);

			return;
		}

		/* We are going to load the icon asynchronously. To keep this
		 * image instance alive until async loading finishs and calls
		 * the callback function we take an extra reference on this
		 * instance. It will be release in callback function.
		 */
		gdk_pixbuf_new_from_stream_at_scale_async(stream,
													priv->iconSize,
													priv->iconSize,
													TRUE,
													NULL,
													(GAsyncReadyCallback)_esdashboard_image_content_loading_async_callback,
													g_object_ref(self));

		ESDASHBOARD_DEBUG(self, IMAGES,
							"Loading icon '%s' from file %s",
							priv->iconName,
							filename);

		/* Release allocated resources */
		g_object_unref(stream);
		g_object_unref(file);
		g_free(filename);
	}

	/* Release allocated resources */
	g_free(lookupFilename);
}

/* Load image from icon theme */
static gboolean _esdashboard_image_content_load_from_icon_name_is_supported_suffix(GdkPixbufFormat *inFormat,
																					const gchar *inExtension)
{
	gchar			**extensions, **entry;
	gboolean		isSupported;

	g_return_val_if_fail(inFormat, FALSE);
	g_return_val_if_fail(inExtension && *inExtension=='.' && *(inExtension+1), FALSE);

	/* Get extensions supported by gdk-pixbuf format */
	extensions=gdk_pixbuf_format_get_extensions(inFormat);

	/* Iterate through list of extensions supported by format and check
	 * if any of them matches given extension.
	 */
	isSupported=FALSE;
	for(entry=extensions; *entry && !isSupported; entry++)
	{
		if(g_strcmp0(inExtension+1, *entry)==0)
		{
			isSupported=TRUE;
			ESDASHBOARD_DEBUG(NULL, IMAGES,
								"Extension '%s' is supported by '%s'",
								inExtension+1,
								gdk_pixbuf_format_get_description(inFormat));
		}
	}

	/* Free allocated resources */
	g_strfreev(extensions);

	/* Return status if extension is known and supported */
	return(isSupported);
}

static void _esdashboard_image_content_load_from_icon_name(EsdashboardImageContent *self)
{
	EsdashboardImageContentPrivate		*priv;
	GtkIconInfo							*iconInfo;
	const gchar							*filename;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));

	priv=self->priv;

	/* Check if type of image is valid and all needed parameters are set */
	g_return_if_fail(priv->type==ESDASHBOARD_IMAGE_TYPE_ICON_NAME);
	g_return_if_fail(priv->iconName);
	g_return_if_fail(priv->iconSize>0);

	/* Get icon info for icon name */
	iconInfo=gtk_icon_theme_lookup_icon(priv->iconTheme,
										priv->iconName,
										priv->iconSize,
#ifdef USE_GTK_BUILTIN_ICONS
										GTK_ICON_LOOKUP_USE_BUILTIN);
#else
										0);
#endif

	/* If we got no icon info but a filename (icon name with suffix like
	 * .png etc.) was given, retry without file extension/suffix.
	 */
	if(!iconInfo)
	{
		gchar							*extensionPosition;

		extensionPosition=g_strrstr(priv->iconName, ".");
		if(extensionPosition)
		{
			gchar						*extension;
			GSList						*supportedFormats, *entry;
			GdkPixbufFormat				*format;
			gboolean					isSupported;

			/* Get suffix - the file extension */
			extension=g_utf8_casefold(extensionPosition, -1);
			ESDASHBOARD_DEBUG(self, IMAGES,
								"Checking if icon filename '%s' with suffix '%s' is supported by gdk-pixbuf",
								priv->iconName,
								extensionPosition);

			/* Get all formats supported by gdk-pixbuf and check if
			 * suffix matches any of them.
			 */
			isSupported=FALSE;
			supportedFormats=gdk_pixbuf_get_formats();
			for(entry=supportedFormats; entry && !isSupported; entry=g_slist_next(entry))
			{
				format=(GdkPixbufFormat*)entry->data;

				if(_esdashboard_image_content_load_from_icon_name_is_supported_suffix(format, extension))
				{
					isSupported=TRUE;
				}
			}
			g_slist_free(supportedFormats);

			/* If extension is supported truncate filename by extension
			 * and try again to retrieve icon info.
			 */
			if(isSupported)
			{
				gchar					*iconName;

				/* Get icon name from icon filename without extension */
				iconName=g_strndup(priv->iconName, extensionPosition-priv->iconName);

				/* Try to get icon info for this icon name */
				iconInfo=gtk_icon_theme_lookup_icon(priv->iconTheme,
													iconName,
													priv->iconSize,
#ifdef USE_GTK_BUILTIN_ICONS
													GTK_ICON_LOOKUP_USE_BUILTIN);
#else
													0);
#endif

				if(!iconInfo)
				{
					g_warning("Could not lookup icon name '%s' for icon '%s'",
								iconName,
								priv->iconName);
				}
					else
					{
						ESDASHBOARD_DEBUG(self, IMAGES,
											"Extension '%s' is supported and loaded icon name '%s' for icon '%s'",
											extension,
											iconName,
											priv->iconName);
					}

				/* Release allocated resources */
				g_free(iconName);
			}
				else
				{
					ESDASHBOARD_DEBUG(self, IMAGES,
										"Extension '%s' is not supported by gdk-pixbuf",
										extension);
				}

			/* Release allocated resources */
			g_free(extension);
		}
	}

	/* If we got no icon info we try to fallback icon next */
	if(!iconInfo)
	{
		g_warning("Could not lookup themed icon '%s' - trying fallback icon '%s'",
					priv->iconName,
					priv->missingIconName);

		iconInfo=gtk_icon_theme_lookup_icon(priv->iconTheme,
											priv->missingIconName,
											priv->iconSize,
											GTK_ICON_LOOKUP_USE_BUILTIN);
	}

	/* If we still got no icon info then we cannot load icon at all */
	if(!iconInfo)
	{
		g_warning("Could not lookup fallback icon '%s' for icon '%s'",
					priv->missingIconName,
					priv->iconName);
		return;
	}

	/* Check if have to use built-in GdkPixbuf for icon ... */
	filename=gtk_icon_info_get_filename(iconInfo);
#ifdef USE_GTK_BUILTIN_ICONS
	if(!filename)
	{
		GdkPixbuf						*iconPixbuf;
		GError							*error=NULL;

		iconPixbuf=gtk_icon_info_get_builtin_pixbuf(iconInfo);
		if(!clutter_image_set_data(CLUTTER_IMAGE(self),
									gdk_pixbuf_get_pixels(iconPixbuf),
									gdk_pixbuf_get_has_alpha(iconPixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
									gdk_pixbuf_get_width(iconPixbuf),
									gdk_pixbuf_get_height(iconPixbuf),
									gdk_pixbuf_get_rowstride(iconPixbuf),
									&error))
		{
			g_warning("Failed to load image data into content for icon '%s': %s",
						priv->iconName,
						error ? error->message : "Unknown error");
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}
		}
			else
			{
				ESDASHBOARD_DEBUG(self, IMAGES,
									"Loaded image data into content for icon '%s' from built-in pixbuf",
									priv->iconName);
			}

		g_object_unref(iconPixbuf);
	}
		/* ... otherwise set up to load icon async */
		else
#endif
		{
			GFile						*file;
			GInputStream				*stream;
			GError						*error=NULL;

			/* Create stream for loading async */
			file=g_file_new_for_path(filename);
			stream=G_INPUT_STREAM(g_file_read(file, NULL, &error));
			if(!stream)
			{
				g_warning("Could not create stream for icon file %s of icon '%s': %s",
							filename,
							priv->iconName,
							error ? error->message : "Unknown error");

				if(error!=NULL)
				{
					g_error_free(error);
					error=NULL;
				}

				/* Release allocated resources */
				g_object_unref(file);

				return;
			}

			/* We are going to load the icon asynchronously. To keep this
			 * image instance alive until async loading finishs and calls
			 * the callback function we take an extra reference on this
			 * instance. It will be release in callback function.
			 */
			gdk_pixbuf_new_from_stream_at_scale_async(stream,
														priv->iconSize,
														priv->iconSize,
														TRUE,
														NULL,
														(GAsyncReadyCallback)_esdashboard_image_content_loading_async_callback,
														g_object_ref(self));

			/* Release allocated resources */
			g_object_unref(stream);
			g_object_unref(file);

			ESDASHBOARD_DEBUG(self, IMAGES,
								"Loading icon '%s' from icon file %s",
								priv->iconName,
								filename);
		}

	/* Release allocated resources */
	g_object_unref(iconInfo);
}

/* Load image from GIcon */
static void _esdashboard_image_content_load_from_gicon(EsdashboardImageContent *self)
{
	EsdashboardImageContentPrivate		*priv;
	GtkIconInfo							*iconInfo;
	const gchar							*filename;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));

	priv=self->priv;

	/* Check if type of image is valid and all needed parameters are set */
	g_return_if_fail(priv->type==ESDASHBOARD_IMAGE_TYPE_GICON);
	g_return_if_fail(priv->gicon);
	g_return_if_fail(priv->iconSize>0);

	/* Get icon info for icon name */
	iconInfo=gtk_icon_theme_lookup_by_gicon(priv->iconTheme,
											priv->gicon,
											priv->iconSize,
#ifdef USE_GTK_BUILTIN_ICONS
											GTK_ICON_LOOKUP_USE_BUILTIN);
#else
											0);
#endif

	/* If we got no icon info we try to fallback icon next */
	if(!iconInfo)
	{
		g_warning("Could not lookup gicon '%s'", g_icon_to_string(priv->gicon));

		iconInfo=gtk_icon_theme_lookup_icon(priv->iconTheme,
											priv->missingIconName,
											priv->iconSize,
											GTK_ICON_LOOKUP_USE_BUILTIN);
	}

	/* If we still got no icon info then we cannot load icon at all */
	if(!iconInfo)
	{
		g_error("Could not lookup fallback icon for gicon '%s'", g_icon_to_string(priv->gicon));
		return;
	}

	/* Check if have to use built-in GdkPixbuf for icon ... */
	filename=gtk_icon_info_get_filename(iconInfo);
#ifdef USE_GTK_BUILTIN_ICONS
	if(!filename)
	{
		GdkPixbuf						*iconPixbuf;
		GError							*error=NULL;

		iconPixbuf=gtk_icon_info_get_builtin_pixbuf(iconInfo);
		if(!clutter_image_set_data(CLUTTER_IMAGE(self),
									gdk_pixbuf_get_pixels(iconPixbuf),
									gdk_pixbuf_get_has_alpha(iconPixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
									gdk_pixbuf_get_width(iconPixbuf),
									gdk_pixbuf_get_height(iconPixbuf),
									gdk_pixbuf_get_rowstride(iconPixbuf),
									&error))
		{
			g_warning("Failed to load image data into content for gicon '%s': %s",
						g_icon_to_string(priv->gicon),
						error ? error->message : "Unknown error");
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}
		}
			else
			{
				ESDASHBOARD_DEBUG(self, IMAGES,
									"Loaded image data into content for gicon '%s' from built-in pixbuf",
									g_icon_to_string(priv->gicon));
			}

		g_object_unref(iconPixbuf);
	}
		/* ... otherwise set up to load icon async */
		else
#endif
		{
			GFile						*file;
			GInputStream				*stream;
			GError						*error=NULL;

			/* Create stream for loading async */
			file=g_file_new_for_path(filename);
			stream=G_INPUT_STREAM(g_file_read(file, NULL, &error));
			if(!stream)
			{
				g_warning("Could not create stream for file %s of gicon '%s': %s",
							filename,
							g_icon_to_string(priv->gicon),
							error ? error->message : "Unknown error");

				if(error!=NULL)
				{
					g_error_free(error);
					error=NULL;
				}

				/* Release allocated resources */
				g_object_unref(file);

				return;
			}

			/* We are going to load the icon asynchronously. To keep this
			 * image instance alive until async loading finishs and calls
			 * the callback function we take an extra reference on this
			 * instance. It will be release in callback function.
			 */
			gdk_pixbuf_new_from_stream_async(stream,
												NULL,
												(GAsyncReadyCallback)_esdashboard_image_content_loading_async_callback,
												g_object_ref(self));

			/* Release allocated resources */
			g_object_unref(stream);
			g_object_unref(file);

			ESDASHBOARD_DEBUG(self, IMAGES,
								"Loading gicon '%s' from file %s",
								g_icon_to_string(priv->gicon),
								filename);
		}

	/* Release allocated resources */
	g_object_unref(iconInfo);
}

/* Icon theme has changed */
static void _esdashboard_image_content_on_icon_theme_changed(EsdashboardImageContent *self,
																gpointer inUserData)
{
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));

	priv=self->priv;

	/* If icon has not been loaded yet then there is no need to do it now */
	if(priv->loadState!=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_SUCCESSFULLY &&
		priv->loadState!=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_FAILED) return;

	/* Set empty image - just for the case loading failed at any point */
	_esdashboard_image_content_set_empty_image(self);

	/* Reload image */
	switch(priv->type)
	{
		case ESDASHBOARD_IMAGE_TYPE_NONE:
			g_warning("Cannot load image '%s' without type", priv->key);
			break;

		case ESDASHBOARD_IMAGE_TYPE_FILE:
			_esdashboard_image_content_load_from_file(self);
			break;

		case ESDASHBOARD_IMAGE_TYPE_ICON_NAME:
			_esdashboard_image_content_load_from_icon_name(self);
			break;

		case ESDASHBOARD_IMAGE_TYPE_GICON:
			_esdashboard_image_content_load_from_gicon(self);
			break;

		default:
			g_warning("Cannot load image '%s' of unknown type %d", priv->key, priv->type);
			break;
	}
}

/* Setup image for loading icon from icon theme by name or absolute file name */
static void _esdashboard_image_content_setup_for_icon(EsdashboardImageContent *self,
														const gchar *inIconName,
														guint inSize)
{
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));
	g_return_if_fail(inIconName && *inIconName);
	g_return_if_fail(inSize>0);

	priv=self->priv;

	/* Image must not be setup already */
	g_return_if_fail(priv->type==ESDASHBOARD_IMAGE_TYPE_NONE);

	/* Determine type of image to load icon from absolute path or theme or file */
	if(g_path_is_absolute(inIconName)) priv->type=ESDASHBOARD_IMAGE_TYPE_FILE;
		else
		{
			EsdashboardTheme			*theme;
			gchar						*iconFilename;

			/* Build absolute path from theme path and given relative path to icon */
			theme=esdashboard_application_get_theme(NULL);
			g_object_ref(theme);

			iconFilename=g_build_filename(esdashboard_theme_get_path(theme),
											inIconName,
											NULL);

			/* Check if image at absolute path build from theme path and relative path exists */
			if(g_file_test(iconFilename, G_FILE_TEST_EXISTS)) priv->type=ESDASHBOARD_IMAGE_TYPE_FILE;
				else priv->type=ESDASHBOARD_IMAGE_TYPE_ICON_NAME;

			/* Release allocated resources */
			g_free(iconFilename);
			g_object_unref(theme);
		}

	/* Set up image */
	priv->iconName=g_strdup(inIconName);
	priv->iconSize=inSize;
}

/* Setup image for loading icon from GIcon */
static void _esdashboard_image_content_setup_for_gicon(EsdashboardImageContent *self,
														GIcon *inIcon,
														guint inSize)
{
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));
	g_return_if_fail(G_IS_ICON(inIcon));
	g_return_if_fail(inSize>0);

	priv=self->priv;

	/* Image must not be setup already */
	g_return_if_fail(priv->type==ESDASHBOARD_IMAGE_TYPE_NONE);

	/* Set up image */
	priv->type=ESDASHBOARD_IMAGE_TYPE_GICON;
	priv->gicon=G_ICON(g_object_ref(inIcon));
	priv->iconSize=inSize;
}

/* Load image */
static void _esdashboard_image_content_load(EsdashboardImageContent *self)
{
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));

	priv=self->priv;

	/* If image is in state of getting loaded or finished loading then do nothing */
	if(priv->loadState!=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_NONE) return;

	ESDASHBOARD_DEBUG(self, IMAGES,
						"Begin loading image with key '%s'",
						priv->key);

	/* Mark image being loaded */
	priv->loadState=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADING;

	/* Set empty image - just for the case loading failed at any point */
	_esdashboard_image_content_set_empty_image(self);

	/* Load icon */
	switch(priv->type)
	{
		case ESDASHBOARD_IMAGE_TYPE_NONE:
			g_warning("Cannot load image '%s' without type", priv->key);
			break;

		case ESDASHBOARD_IMAGE_TYPE_FILE:
			_esdashboard_image_content_load_from_file(self);
			break;

		case ESDASHBOARD_IMAGE_TYPE_ICON_NAME:
			_esdashboard_image_content_load_from_icon_name(self);
			break;

		case ESDASHBOARD_IMAGE_TYPE_GICON:
			_esdashboard_image_content_load_from_gicon(self);
			break;

		default:
			g_warning("Cannot load image '%s' of unknown type %d", priv->key, priv->type);
			break;
	}
}

/* Callback function for g_list_foreach() at list of known actors using this
 * image content. This callback function will only disconnect all signal handlers
 * connected by this image content.
 */
static void _esdashboard_image_content_disconnect_signals_handlers_from_actor(gpointer inData,
																				gpointer inUserData)
{
	EsdashboardImageContent				*self;
	ClutterActor						*actor;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(inUserData));
	g_return_if_fail(CLUTTER_IS_ACTOR(inData));

	self=ESDASHBOARD_IMAGE_CONTENT(inUserData);
	actor=CLUTTER_ACTOR(inData);

	/* Disconnect signal handlers */
	g_signal_handlers_disconnect_by_data(actor, self);
}

/* An actor using this image as content was mapped or unmapped */
static void _esdashboard_image_content_on_actor_mapped(EsdashboardImageContent *self,
														GParamSpec *inSpec,
														gpointer inUserData)
{
	EsdashboardImageContentPrivate		*priv;
	ClutterActor						*actor;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	priv=self->priv;
	actor=CLUTTER_ACTOR(inUserData);

	/* If actor was mapped then load image now and disconnect signal handlers
	 * from all known actors using this image content.
	 */
	if(clutter_actor_is_mapped(actor))
	{
		/* Remove signal handlers from all known actors using this image content */
		g_list_foreach(priv->actors, _esdashboard_image_content_disconnect_signals_handlers_from_actor, self);

		/* Load image now */
		ESDASHBOARD_DEBUG(self, IMAGES,
							"Image with key '%s' will be loaded now because actor %s@%p is mapped now",
							priv->key,
							actor ? G_OBJECT_TYPE_NAME(actor) : "<nil>",
							actor);

		_esdashboard_image_content_load(self);
	}
}


/* IMPLEMENTATION: ClutterContent */

/* Image was attached to an actor */
static void _esdashboard_image_content_on_attached(ClutterContent *inContent,
													ClutterActor *inActor,
													gpointer inUserData)
{
	EsdashboardImageContent				*self;
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(inContent));
	g_return_if_fail(!inActor || CLUTTER_IS_ACTOR(inActor));

	self=ESDASHBOARD_IMAGE_CONTENT(inContent);
	priv=self->priv;

	/* If we got an actor then add it to list of actors known to have this image
	 * attached.
	 */
	if(inActor &&
		CLUTTER_IS_ACTOR(inActor))
	{
		ESDASHBOARD_DEBUG(self, IMAGES,
							"Attached image with key '%s' to %s actor %s@%p",
							priv->key,
							(inActor && clutter_actor_is_mapped(inActor)) ? "mapped" : "unmapped",
							inActor ? G_OBJECT_TYPE_NAME(inActor) : "<nil>",
							inActor);

		/* Add actor to list of known actors using this image content. Avoid
		 * duplicates in this list although it should never happen.
		 */
		if(!g_list_find(priv->actors, inActor))
		{
			priv->actors=g_list_prepend(priv->actors, inActor);
		}
	}

	/* If image is being loaded then do nothing */
	if(priv->loadState==ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADING) return;

	/* Check if image was already loaded then emit signal
	 * appropiate for last load status.
	 */
	if(priv->loadState==ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_SUCCESSFULLY ||
		priv->loadState==ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_FAILED)
	{
		/* Emit "loaded" signal if loading was successful ... */
		if(priv->loadState==ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_SUCCESSFULLY)
		{
			g_signal_emit(self, EsdashboardImageContentSignals[SIGNAL_LOADED], 0);
		}
			/* ... or emit "loading-failed" signal if loading has failed. */
			else
			{
				g_signal_emit(self, EsdashboardImageContentSignals[SIGNAL_LOADING_FAILED], 0);
			}

		return;
	}

	/* If we got an actor then check if it is mapped. If it is not mapped then
	 * connect a signal handler to get notified when the actor is mapped. If the
	 * actor is mapped then call the signal handler directly. The signal handler
	 * called, either when the actor is mapped or called directly, will load the
	 * image.
	 */
	if(inActor &&
		CLUTTER_IS_ACTOR(inActor))
	{
		/* If actor is not mapped then connect signal handler and return here
		 * as the signal handler will load the image once the actor was mapped.
		 */
		if(!clutter_actor_is_mapped(inActor))
		{
			g_signal_connect_swapped(inActor,
										"notify::mapped",
										G_CALLBACK(_esdashboard_image_content_on_actor_mapped),
										self);

			return;
		}
	}

	/* If we get here then either the actor is mapped or we got not an actor at
	 * all, so start loading image now. Otherwise it will never get loaded.
	 */
	ESDASHBOARD_DEBUG(self, IMAGES,
						"Attached image with key '%s' need to get loaded immediately",
						priv->key);
	_esdashboard_image_content_load(self);
}

/* Image was detached from an actor */
static void _esdashboard_image_content_on_detached(ClutterContent *inContent,
													ClutterActor *inActor,
													gpointer inUserData)
{
	EsdashboardImageContent				*self;
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(inContent));
	g_return_if_fail(!inActor || CLUTTER_IS_ACTOR(inActor));

	self=ESDASHBOARD_IMAGE_CONTENT(inContent);
	priv=self->priv;


	/* Remove actor from list of actors known to have this image attached and
	 * disconnect signal handlers.
	 */
	if(inActor &&
		CLUTTER_IS_ACTOR(inActor))
	{
		GList							*iter;

		/* Remove actor from list of known actors using this image content */
		iter=g_list_find(priv->actors, inActor);
		if(iter) priv->actors=g_list_delete_link(priv->actors, iter);

		/* Disconnect signal handler */
		g_signal_handlers_disconnect_by_data(inActor, self);

		ESDASHBOARD_DEBUG(self, IMAGES,
							"Detached image with key '%s' from actor %s@%p",
							priv->key,
							inActor ? G_OBJECT_TYPE_NAME(inActor) : "<nil>",
							inActor);
	}
}

/* IMPLEMENTATION: Interface EsdashboardStylable */

/* Get stylable properties of stage */
static void _esdashboard_image_content_stylable_get_stylable_properties(EsdashboardStylable *inStylable,
																			GHashTable *ioStylableProperties)
{
	g_return_if_fail(ESDASHBOARD_IS_STYLABLE(inStylable));

	/* Add stylable properties to hashtable */
	esdashboard_stylable_add_stylable_property(inStylable, ioStylableProperties, "missing-icon-name");
}

/* Get/set style classes of stage */
static const gchar* _esdashboard_image_content_stylable_get_classes(EsdashboardStylable *inStylable)
{
	/* Not implemented */
	return(NULL);
}

static void _esdashboard_image_content_stylable_set_classes(EsdashboardStylable *inStylable, const gchar *inStyleClasses)
{
	/* Not implemented */
}

/* Get/set style pseudo-classes of stage */
static const gchar* _esdashboard_image_content_stylable_get_pseudo_classes(EsdashboardStylable *inStylable)
{
	/* Not implemented */
	return(NULL);
}

static void _esdashboard_image_content_stylable_set_pseudo_classes(EsdashboardStylable *inStylable, const gchar *inStylePseudoClasses)
{
	/* Not implemented */
}

/* Interface initialization
 * Set up default functions
 */
void _esdashboard_image_content_stylable_iface_init(EsdashboardStylableInterface *iface)
{
	iface->get_stylable_properties=_esdashboard_image_content_stylable_get_stylable_properties;
	iface->get_classes=_esdashboard_image_content_stylable_get_classes;
	iface->set_classes=_esdashboard_image_content_stylable_set_classes;
	iface->get_pseudo_classes=_esdashboard_image_content_stylable_get_pseudo_classes;
	iface->set_pseudo_classes=_esdashboard_image_content_stylable_set_pseudo_classes;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_image_content_dispose(GObject *inObject)
{
	EsdashboardImageContent				*self=ESDASHBOARD_IMAGE_CONTENT(inObject);
	EsdashboardImageContentPrivate		*priv=self->priv;

	/* Release allocated resources */
	priv->type=ESDASHBOARD_IMAGE_TYPE_NONE;

	if(priv->actors)
	{
		g_list_foreach(priv->actors, _esdashboard_image_content_disconnect_signals_handlers_from_actor, self);
		g_list_free(priv->actors);
		priv->actors=NULL;
	}

	if(priv->contentAttachedSignalID)
	{
		g_signal_handler_disconnect(self, priv->contentAttachedSignalID);
		priv->contentAttachedSignalID=0;
	}

	if(priv->contentDetachedSignalID)
	{
		g_signal_handler_disconnect(self, priv->contentDetachedSignalID);
		priv->contentDetachedSignalID=0;
	}

	if(priv->iconThemeChangedSignalID)
	{
		g_signal_handler_disconnect(priv->iconTheme, priv->iconThemeChangedSignalID);
		priv->iconThemeChangedSignalID=0;
	}

	if(priv->key)
	{
		_esdashboard_image_content_remove_from_cache(self);
		g_free(priv->key);
		priv->key=NULL;
	}

	if(priv->iconName)
	{
		g_free(priv->iconName);
		priv->iconName=NULL;
	}

	if(priv->gicon)
	{
		g_object_unref(priv->gicon);
		priv->gicon=NULL;
	}

	if(priv->missingIconName)
	{
		g_free(priv->missingIconName);
		priv->missingIconName=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_image_content_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_image_content_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardImageContent			*self=ESDASHBOARD_IMAGE_CONTENT(inObject);

	switch(inPropID)
	{
		case PROP_KEY:
			_esdashboard_image_content_store_in_cache(self, g_value_get_string(inValue));
			break;

		case PROP_MISSING_ICON_NAME:
			esdashboard_image_content_set_missing_icon_name(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_image_content_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardImageContent			*self=ESDASHBOARD_IMAGE_CONTENT(inObject);
	EsdashboardImageContentPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_MISSING_ICON_NAME:
			g_value_set_string(outValue, priv->missingIconName);
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
void esdashboard_image_content_class_init(EsdashboardImageContentClass *klass)
{
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);
	EsdashboardStylableInterface	*stylableIface;
	GParamSpec						*paramSpec;

	/* Override functions */
	gobjectClass->dispose=_esdashboard_image_content_dispose;
	gobjectClass->set_property=_esdashboard_image_content_set_property;
	gobjectClass->get_property=_esdashboard_image_content_get_property;

	stylableIface=g_type_default_interface_ref(ESDASHBOARD_TYPE_STYLABLE);

	/* Define properties */
	EsdashboardImageContentProperties[PROP_KEY]=
		g_param_spec_string("key",
							"Key",
							"The hash key for caching this image",
							N_(""),
							G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	EsdashboardImageContentProperties[PROP_MISSING_ICON_NAME]=
		g_param_spec_string("missing-icon-name",
							"Missing icon name",
							"The icon's name to use when requested image cannot be loaded",
							ESDASHBOARD_IMAGE_CONTENT_DEFAULT_FALLBACK_ICON_NAME,
							G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);

	paramSpec=g_object_interface_find_property(stylableIface, "style-classes");
	EsdashboardImageContentProperties[PROP_STYLE_CLASSES]=
		g_param_spec_override("style-classes", paramSpec);

	paramSpec=g_object_interface_find_property(stylableIface, "style-pseudo-classes");
	EsdashboardImageContentProperties[PROP_STYLE_PSEUDO_CLASSES]=
		g_param_spec_override("style-pseudo-classes", paramSpec);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardImageContentProperties);

	/* Define signals */
	EsdashboardImageContentSignals[SIGNAL_LOADED]=
		g_signal_new("loaded",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardImageContentClass, loaded),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	EsdashboardImageContentSignals[SIGNAL_LOADING_FAILED]=
		g_signal_new("loading-failed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardImageContentClass, loading_failed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/* Release allocated resources */
	g_type_default_interface_unref(stylableIface);
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_image_content_init(EsdashboardImageContent *self)
{
	EsdashboardImageContentPrivate		*priv;

	priv=self->priv=esdashboard_image_content_get_instance_private(self);

	/* Set up default values */
	priv->key=NULL;
	priv->type=ESDASHBOARD_IMAGE_TYPE_NONE;
	priv->iconName=NULL;
	priv->gicon=NULL;
	priv->iconSize=0;
	priv->loadState=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_NONE;
	priv->iconTheme=gtk_icon_theme_get_default();
	priv->missingIconName=g_strdup(ESDASHBOARD_IMAGE_CONTENT_DEFAULT_FALLBACK_ICON_NAME);
	priv->actors=NULL;

	/* Style content */
	esdashboard_stylable_invalidate(ESDASHBOARD_STYLABLE(self));

	/* Connect to "attached" and "detached" signal of ClutterContent to get
	 * notified when this image is used or released. We will check if the actor
	 * using this image is mapped and load image or we wait until any actor having
	 * this image attached is mapped and then load the image.
	 */
	priv->contentAttachedSignalID=g_signal_connect(self,
													"attached",
													G_CALLBACK(_esdashboard_image_content_on_attached),
													NULL);

	priv->contentDetachedSignalID=g_signal_connect(self,
													"detached",
													G_CALLBACK(_esdashboard_image_content_on_detached),
													NULL);

	/* Connect to "changed" signal of GtkIconTheme to get notified
	 * when icon theme has changed to reload loaded images.
	 */
	priv->iconThemeChangedSignalID=g_signal_connect_swapped(priv->iconTheme,
															"changed",
															G_CALLBACK(_esdashboard_image_content_on_icon_theme_changed),
															self);
}

/* IMPLEMENTATION: Public API */

/* Create new instance or use cached one for themed icon name or absolute icon filename.
 * If icon does not exists a themed fallback icon will be used.
 * If even the themed fallback icon cannot be found we set an empty image.
 * In all cases a valid ClutterImage object is returned and must be unreffed with
 * g_object_unref().
 */
ClutterContent* esdashboard_image_content_new_for_icon_name(const gchar *inIconName, gint inSize)
{
	ClutterImage			*image;
	gchar					*key;

	g_return_val_if_fail(inIconName!=NULL, NULL);
	g_return_val_if_fail(inSize>0, NULL);

	/* Check if we have a cache image for icon otherwise create image instance */
	key=g_strdup_printf("icon-name:%s,%d", inIconName, inSize);
	if(!key)
	{
		g_warning("Could not create key for icon name '%s' at size %u", inIconName, inSize);
		return(NULL);
	}

	image=_esdashboard_image_content_get_cached_image(key);
	if(!image)
	{
		image=CLUTTER_IMAGE(g_object_new(ESDASHBOARD_TYPE_IMAGE_CONTENT,
											"key", key,
											NULL));
		_esdashboard_image_content_setup_for_icon(ESDASHBOARD_IMAGE_CONTENT(image), inIconName, inSize);
	}

	g_free(key);

	/* Return ClutterImage */
	return(CLUTTER_CONTENT(image));
}

/* Create new instance or use cached one for GIcon object.
 * If icon does not exists a themed fallback icon will be used.
 * If even the themed fallback icon cannot be found we set an empty image.
 * In all cases a valid ClutterImage object is returned and must be unreffed with
 * g_object_unref().
 */
ClutterContent* esdashboard_image_content_new_for_gicon(GIcon *inIcon, gint inSize)
{
	ClutterImage			*image;
	gchar					*key;

	g_return_val_if_fail(G_IS_ICON(inIcon), NULL);
	g_return_val_if_fail(inSize>0, NULL);

	image=NULL;

	/* If GIcon is a file icon get filename and redirect to create function for
	 * icon-names/file-names to share images created with this function. If we
	 * have problems in getting the filename fallthrough to default behaviour.
	 */
	if(G_IS_FILE_ICON(inIcon))
	{
		GFile				*iconFile;
		gchar				*iconFilename;

		/* Get file object of icon*/
		iconFile=g_file_icon_get_file(G_FILE_ICON(inIcon));
		if(iconFile)
		{
			iconFilename=g_file_get_path(iconFile);
			if(iconFilename)
			{
				/* Redirect to create function for icon-names/file-names */
				image=CLUTTER_IMAGE(esdashboard_image_content_new_for_icon_name(iconFilename, inSize));

				/* Release allocated resources */
				g_free(iconFilename);

				/* Return image */
				return(CLUTTER_CONTENT(image));
			}
		}
	}

	/* If GIcon is a themed icon with exactly one name associated, get name
	 * and redirect to create function for icon-names/file-names to share
	 * images created with this function. If we have problems in getting
	 * the icon-name fallthrough to default behaviour.
	 */
	if(G_IS_THEMED_ICON(inIcon))
	{
		const gchar* const	*iconNames;

		iconNames=g_themed_icon_get_names(G_THEMED_ICON(inIcon));
		if(g_strv_length((gchar**)iconNames)==1)
		{
			/* Redirect to create function for icon-names/file-names */
			image=CLUTTER_IMAGE(esdashboard_image_content_new_for_icon_name(*iconNames, inSize));

			/* Return image */
			return(CLUTTER_CONTENT(image));
		}
	}

	/* Check if we have a cache image for icon otherwise create image instance */
	key=g_strdup_printf("gicon:%s-%u,%d", G_OBJECT_TYPE_NAME(inIcon), g_icon_hash(inIcon), inSize);
	if(!key)
	{
		g_warning("Could not create key for gicon '%s' at size %u", g_icon_to_string(inIcon), inSize);
		return(NULL);
	}

	image=_esdashboard_image_content_get_cached_image(key);
	if(!image)
	{
		image=CLUTTER_IMAGE(g_object_new(ESDASHBOARD_TYPE_IMAGE_CONTENT, 
											"key", key,
											NULL));
		_esdashboard_image_content_setup_for_gicon(ESDASHBOARD_IMAGE_CONTENT(image), inIcon, inSize);
	}

	g_free(key);

	/* Return ClutterImage */
	return(CLUTTER_CONTENT(image));
}

/* Create a new instance for GdkPixbuf object.
 * An image of GdkPixbuf will never be cached as the pixbuf at given
 * pointer may change and we do not get notified. The pointer may also
 * be re-used for a completely new and difference instance of GdkPixbuf.
 * The return ClutterImage object (if not NULL) must be unreffed with
 * g_object_unref().
 */
ClutterContent* esdashboard_image_content_new_for_pixbuf(GdkPixbuf *inPixbuf)
{
	ClutterContent			*image;
	GError					*error;

	g_return_val_if_fail(GDK_IS_PIXBUF(inPixbuf), NULL);

	error=NULL;

	/* Create ClutterImage for pixbuf directly because images
	 * from GdkPixbuf will not be cached
	 */
	image=clutter_image_new();
	if(!clutter_image_set_data(CLUTTER_IMAGE(image),
								gdk_pixbuf_get_pixels(inPixbuf),
								gdk_pixbuf_get_has_alpha(inPixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
								gdk_pixbuf_get_width(inPixbuf),
								gdk_pixbuf_get_height(inPixbuf),
								gdk_pixbuf_get_rowstride(inPixbuf),
								&error))
	{
		g_warning("Failed to load image data from pixbuf into content: %s",
					error ? error->message : "Unknown error");

		if(error)
		{
			g_error_free(error);
			error=NULL;
		}

		/* Set empty image */
		_esdashboard_image_content_set_empty_image(ESDASHBOARD_IMAGE_CONTENT(image));
	}

	/* Return ClutterImage */
	return(CLUTTER_CONTENT(image));
}

/* Get/set icon name to use when requested icon cannot be loaded */
const gchar* esdashboard_image_content_get_missing_icon_name(EsdashboardImageContent *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self), NULL);

	return(self->priv->missingIconName);
}

void esdashboard_image_content_set_missing_icon_name(EsdashboardImageContent *self, const gchar *inMissingIconName)
{
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));
	g_return_if_fail(inMissingIconName && *inMissingIconName);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->missingIconName, inMissingIconName)!=0)
	{
		/* Set value */
		if(priv->missingIconName)
		{
			g_free(priv->missingIconName);
			priv->missingIconName=NULL;
		}

		if(inMissingIconName) priv->missingIconName=g_strdup(inMissingIconName);

		/* If this image content is a failed one then reload it */
		if(priv->loadState==ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_LOADED_FAILED)
		{
			/* Set state of image to "not-loaded" */
			priv->loadState=ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_NONE;

			/* Try to load image again. It will set up an empty image first and
			 * then try to load the image.  It is likely that it will fail again
			 * but then it will show the new missing icon instead of the old one.
			 */
			ESDASHBOARD_DEBUG(self, IMAGES,
								"Reload failed  image with key '%s' because of changed missing-icon property",
								priv->key);
			_esdashboard_image_content_load(self);
		}

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardImageContentProperties[PROP_MISSING_ICON_NAME]);
	}
}

/* Get loading state of image */
EsdashboardImageContentLoadingState esdashboard_image_content_get_state(EsdashboardImageContent *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self), ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_NONE);

	return(self->priv->loadState);
}

/* Force loading image if not available */
void esdashboard_image_content_force_load(EsdashboardImageContent *self)
{
	EsdashboardImageContentPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_IMAGE_CONTENT(self));

	priv=self->priv;

	/* If loading state is none then image was not loaded yet and is also
	 * not being loaded currently. So enforce loading now.
	 */
	if(priv->loadState==ESDASHBOARD_IMAGE_CONTENT_LOADING_STATE_NONE)
	{
		ESDASHBOARD_DEBUG(self, IMAGES,
							"Need to enforce loading image with key '%s'",
							priv->key);
		_esdashboard_image_content_load(self);
	}
}
