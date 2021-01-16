/*
 * window: A managed window of window manager
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

#define COGL_ENABLE_EXPERIMENTAL_API
#define CLUTTER_ENABLE_EXPERIMENTAL_API

#include <libesdashboard/x11/window-content-x11.h>

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>
#ifdef CLUTTER_WINDOWING_GDK
#include <clutter/gdk/clutter-gdk.h>
#endif
#include <cogl/cogl-texture-pixmap-x11.h>
#ifdef HAVE_XCOMPOSITE
#include <X11/extensions/Xcomposite.h>
#endif
#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif
#include <gdk/gdkx.h>

#include <libesdashboard/window-content.h>
#include <libesdashboard/x11/window-tracker-window-x11.h>
#include <libesdashboard/application.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/stylable.h>
#include <libesdashboard/window-tracker.h>
#include <libesdashboard/enums.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Definitions */
typedef enum /*< skip,prefix=ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE >*/
{
	ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_NONE=0,
	ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_UNMINIMIZING,
	ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_REMINIMIZING,
	ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_DONE
} EsdashboardWindowContentX11WorkaroundMode;

/* Define this class in GObject system */
static void _esdashboard_window_content_clutter_content_iface_init(ClutterContentIface *iface);
static void _esdashboard_window_content_x11_stylable_iface_init(EsdashboardStylableInterface *iface);

struct _EsdashboardWindowContentX11Private
{
	/* Properties related */
	EsdashboardWindowTrackerWindowX11			*window;
	ClutterColor								*outlineColor;
	gfloat										outlineWidth;
	gboolean									isSuspended;
	gboolean									includeWindowFrame;

	gboolean									unmappedWindowIconXFill;
	gboolean									unmappedWindowIconYFill;
	gfloat										unmappedWindowIconXAlign;
	gfloat										unmappedWindowIconYAlign;
	gfloat										unmappedWindowIconXScale;
	gfloat										unmappedWindowIconYScale;
	EsdashboardAnchorPoint						unmappedWindowIconAnchorPoint;

	gchar										*styleClasses;
	gchar										*stylePseudoClasses;

	/* Instance related */
	gboolean									isFallback;
	CoglTexture									*texture;
	Window										xWindowID;
	Pixmap										pixmap;
#ifdef HAVE_XDAMAGE
	Damage										damage;
#endif

	guint										suspendSignalID;
	gboolean									isMapped;
	gboolean									isAppSuspended;

	EsdashboardWindowTracker					*windowTracker;
	EsdashboardWindowContentX11WorkaroundMode	workaroundMode;
	guint										workaroundStateSignalID;

	gboolean									suspendAfterResumeOnIdle;

	guint										windowClosedSignalID;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardWindowContentX11,
						esdashboard_window_content_x11,
						ESDASHBOARD_TYPE_WINDOW_CONTENT,
						G_ADD_PRIVATE(EsdashboardWindowContentX11)
						G_IMPLEMENT_INTERFACE(CLUTTER_TYPE_CONTENT, _esdashboard_window_content_clutter_content_iface_init)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_STYLABLE, _esdashboard_window_content_x11_stylable_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW,

	PROP_SUSPENDED,

	PROP_OUTLINE_COLOR,
	PROP_OUTLINE_WIDTH,

	PROP_INCLUDE_WINDOW_FRAME,

	PROP_UNMAPPED_WINDOW_ICON_X_FILL,
	PROP_UNMAPPED_WINDOW_ICON_Y_FILL,
	PROP_UNMAPPED_WINDOW_ICON_X_ALIGN,
	PROP_UNMAPPED_WINDOW_ICON_Y_ALIGN,
	PROP_UNMAPPED_WINDOW_ICON_X_SCALE,
	PROP_UNMAPPED_WINDOW_ICON_Y_SCALE,
	PROP_UNMAPPED_WINDOW_ICON_ANCHOR_POINT,

	/* From interface: EsdashboardStylable */
	PROP_STYLE_CLASSES,
	PROP_STYLE_PSEUDO_CLASSES,

	PROP_LAST
};

static GParamSpec* EsdashboardWindowContentX11Properties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define COMPOSITE_VERSION_MIN_MAJOR		0
#define COMPOSITE_VERSION_MIN_MINOR		2

#define WORKAROUND_UNMAPPED_WINDOW_ESCONF_PROP				"/enable-unmapped-window-workaround"
#define DEFAULT_WORKAROUND_UNMAPPED_WINDOW					FALSE

#define WINDOW_CONTENT_CREATION_PRIORITY_ESCONF_PROP		"/window-content-creation-priority"
#define DEFAULT_WINDOW_CONTENT_X11_CREATION_PRIORITY		"immediate"

struct _EsdashboardWindowContentX11PriorityMap
{
	const gchar		*name;
	gint			priority;
};
typedef struct _EsdashboardWindowContentX11PriorityMap		EsdashboardWindowContentX11PriorityMap;

static gboolean									_esdashboard_window_content_x11_have_checked_extensions=FALSE;
static gboolean									_esdashboard_window_content_x11_have_composite_extension=FALSE;
static gboolean									_esdashboard_window_content_x11_have_damage_extension=FALSE;
static int										_esdashboard_window_content_x11_damage_event_base=0;

static GList*									_esdashboard_window_content_x11_resume_idle_queue=NULL;
static guint									_esdashboard_window_content_x11_resume_idle_id=0;
static guint									_esdashboard_window_content_x11_resume_shutdown_signal_id=0;

static guint									_esdashboard_window_content_x11_esconf_priority_notify_id=0;
static gint										_esdashboard_window_content_x11_window_creation_priority=-1;
static EsdashboardWindowContentX11PriorityMap	_esdashboard_window_content_x11_window_creation_priority_map[]=
												{
													{ "immediate", -1 }, /* First entry is default value */
													{ "high", G_PRIORITY_HIGH_IDLE },
													{ "normal", G_PRIORITY_DEFAULT_IDLE },
													{ "low", G_PRIORITY_LOW },
													{ NULL, 0 },
												};
static guint									_esdashboard_window_content_x11_window_creation_shutdown_signal_id=0;

/* Forward declarations */
static void _esdashboard_window_content_x11_suspend(EsdashboardWindowContentX11 *self);
static void _esdashboard_window_content_x11_resume(EsdashboardWindowContentX11 *self);
static gboolean _esdashboard_window_content_x11_resume_on_idle(gpointer inUserData);

/* Get X server display */
static Display* _esdashboard_window_content_x11_get_display(void)
{
	Display			*display;

	display=None;

#ifdef CLUTTER_WINDOWING_X11
	if(clutter_check_windowing_backend(CLUTTER_WINDOWING_X11))
	{
		display=clutter_x11_get_default_display();
	}
#endif

#ifdef CLUTTER_WINDOWING_GDK
	if(clutter_check_windowing_backend(CLUTTER_WINDOWING_GDK))
	{
		display=gdk_x11_display_get_xdisplay(clutter_gdk_get_default_display());
	}
#endif

	if(G_UNLIKELY(display==None))
	{
		g_critical("No default X11 display found in GDK to check X extensions");
	}

	return(display);
}

/* Remove all entries from resume queue and release all allocated resources */
static void _esdashboard_window_content_x11_destroy_resume_queue(void)
{
	EsdashboardApplication					*application;
	gint									queueSize;

	/* Disconnect application "shutdown" signal handler */
	if(_esdashboard_window_content_x11_resume_shutdown_signal_id)
	{
		ESDASHBOARD_DEBUG(NULL, WINDOWS,
							"Disconnecting shutdown signal handler %u because of resume queue destruction",
							_esdashboard_window_content_x11_resume_shutdown_signal_id);

		application=esdashboard_application_get_default();
		g_signal_handler_disconnect(application, _esdashboard_window_content_x11_resume_shutdown_signal_id);
		_esdashboard_window_content_x11_resume_shutdown_signal_id=0;
	}

	/* Remove idle source if available */
	if(_esdashboard_window_content_x11_resume_idle_id)
	{
		ESDASHBOARD_DEBUG(NULL, WINDOWS,
							"Removing resume window content idle source with ID %u",
							_esdashboard_window_content_x11_resume_idle_id);

		g_source_remove(_esdashboard_window_content_x11_resume_idle_id);
		_esdashboard_window_content_x11_resume_idle_id=0;
	}

	/* Destroy resume-on-idle queue if available*/
	if(_esdashboard_window_content_x11_resume_idle_queue)
	{
		queueSize=g_list_length(_esdashboard_window_content_x11_resume_idle_queue);
		if(queueSize>0) g_warning("Destroying window content resume queue containing %d windows.", queueSize);
#ifdef DEBUG
		if(queueSize>0)
		{
			GList								*iter;
			EsdashboardWindowContentX11			*content;
			EsdashboardWindowTrackerWindow		*window;

			for(iter=_esdashboard_window_content_x11_resume_idle_queue; iter; iter=g_list_next(iter))
			{
				content=ESDASHBOARD_WINDOW_CONTENT_X11(iter->data);
				window=esdashboard_window_content_x11_get_window(content);
				g_print("Window content in resume queue: Item %s@%p for window '%s'\n",
							G_OBJECT_TYPE_NAME(content), content,
							esdashboard_window_tracker_window_get_name(window));
			}
		}
#endif

		ESDASHBOARD_DEBUG(NULL, WINDOWS, "Destroying window content resume queue");
		g_list_free(_esdashboard_window_content_x11_resume_idle_queue);
		_esdashboard_window_content_x11_resume_idle_queue=NULL;
	}
}

/* Remove window content from resume on idle queue */
static void _esdashboard_window_content_x11_resume_on_idle_remove(EsdashboardWindowContentX11 *self)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));

	priv=self->priv;

	/* Remove window content from queue */
	if(_esdashboard_window_content_x11_resume_idle_queue)
	{
		GList								*queueEntry;

		/* Lookup window content in queue and remove it from queue. If queue is empty
		 * after removal, remove idle source also.
		 */
		queueEntry=g_list_find(_esdashboard_window_content_x11_resume_idle_queue, self);
		if(queueEntry)
		{
			/* Remove window content from queue */
			_esdashboard_window_content_x11_resume_idle_queue=g_list_delete_link(_esdashboard_window_content_x11_resume_idle_queue, queueEntry);
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"Removed queue entry %p for window '%s' because of releasing resources",
								queueEntry,
								esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
		}
	}

	/* If queue is empty remove idle source as well */
	if(!_esdashboard_window_content_x11_resume_idle_queue &&
		_esdashboard_window_content_x11_resume_idle_id)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Removing idle source with ID %u because queue is empty",
							_esdashboard_window_content_x11_resume_idle_id);

		g_source_remove(_esdashboard_window_content_x11_resume_idle_id);
		_esdashboard_window_content_x11_resume_idle_id=0;
	}
}

/* Add window content to resume on idle queue */
static void _esdashboard_window_content_x11_resume_on_idle_add(EsdashboardWindowContentX11 *self)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));

	priv=self->priv;

	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Using resume on idle for window '%s'",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));

	/* Only add callback to resume window content if no one was added */
	if(!g_list_find(_esdashboard_window_content_x11_resume_idle_queue, self))
	{
		/* Queue window content for resume */
		_esdashboard_window_content_x11_resume_idle_queue=g_list_append(_esdashboard_window_content_x11_resume_idle_queue, self);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Queued window resume of '%s'",
							esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
	}

	/* Create idle source for resuming queued window contents but with
	 * high priority to get window content created as soon as possible.
	 */
	if(_esdashboard_window_content_x11_resume_idle_queue &&
		!_esdashboard_window_content_x11_resume_idle_id)
	{
		_esdashboard_window_content_x11_resume_idle_id=clutter_threads_add_idle_full(_esdashboard_window_content_x11_window_creation_priority,
																				_esdashboard_window_content_x11_resume_on_idle,
																				NULL,
																				NULL);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Created idle source with ID %u with priority of %d because of new resume queue created for window resume of '%s'",
							_esdashboard_window_content_x11_resume_idle_id,
							_esdashboard_window_content_x11_window_creation_priority,
							esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
	}

	/* Connect to "shutdown" signal of application to clean up resume queue */
	if(!_esdashboard_window_content_x11_resume_shutdown_signal_id)
	{
		EsdashboardApplication			*application;

		application=esdashboard_application_get_default();
		_esdashboard_window_content_x11_resume_shutdown_signal_id=g_signal_connect(application,
																				"shutdown-final",
																				G_CALLBACK(_esdashboard_window_content_x11_destroy_resume_queue),
																				NULL);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Connected to shutdown signal with handler ID %u for resume queue destruction",
							_esdashboard_window_content_x11_resume_shutdown_signal_id);
	}
}

/* Value for window creation priority in esconf has changed */
static void _esdashboard_window_content_x11_on_window_creation_priority_value_changed(EsconfChannel *inChannel,
																					const gchar *inProperty,
																					const GValue *inValue,
																					gpointer inUserData)
{
	const gchar									*priorityValue;
	EsdashboardWindowContentX11PriorityMap		*found;

	g_return_if_fail(g_strcmp0(inProperty, WINDOW_CONTENT_CREATION_PRIORITY_ESCONF_PROP)==0);
	g_return_if_fail(inValue && G_VALUE_HOLDS_STRING(inValue));

	/* Determine priority from new value */
	priorityValue=g_value_get_string(inValue);
	found=_esdashboard_window_content_x11_window_creation_priority_map;
	while(found->name && g_strcmp0(priorityValue, found->name)!=0) found++;

	/* Set default value if no match was found in priority map was found */
	if(!found || !found->name)
	{
		/* Default value is the first one in mapping */
		found=_esdashboard_window_content_x11_window_creation_priority_map;

		g_warning("Unknown value '%s' for property '%s' - defaulting to '%s' with priority of %d",
					priorityValue,
					inProperty,
					found->name,
					found->priority);
	}

	/* Set priority */
	if(found)
	{
		_esdashboard_window_content_x11_window_creation_priority=found->priority;
		ESDASHBOARD_DEBUG(NULL, WINDOWS,
							"Setting window creation priority to '%s' with priority of %d",
							found->name,
							found->priority);
	}
}

/* Disconnect signal handler for esconf value change notification on window priority */
static void _esdashboard_window_content_x11_on_window_creation_priority_shutdown(void)
{
	EsdashboardApplication					*application;

	/* Disconnect application "shutdown" signal handler */
	if(_esdashboard_window_content_x11_window_creation_shutdown_signal_id)
	{
		ESDASHBOARD_DEBUG(NULL, WINDOWS,
							"Disconnecting shutdown signal handler %u for window creation priority value change notifications",
							_esdashboard_window_content_x11_window_creation_shutdown_signal_id);

		application=esdashboard_application_get_default();
		g_signal_handler_disconnect(application, _esdashboard_window_content_x11_window_creation_shutdown_signal_id);
		_esdashboard_window_content_x11_window_creation_shutdown_signal_id=0;
	}

	/* Disconnect property changed signal handler */
	if(_esdashboard_window_content_x11_esconf_priority_notify_id)
	{
		EsconfChannel					*esconfChannel;

		ESDASHBOARD_DEBUG(NULL, WINDOWS,
							"Disconnecting property changed signal handler %u for window creation priority value change notifications",
							_esdashboard_window_content_x11_esconf_priority_notify_id);

		esconfChannel=esdashboard_application_get_esconf_channel(NULL);
		g_signal_handler_disconnect(esconfChannel, _esdashboard_window_content_x11_esconf_priority_notify_id);
		_esdashboard_window_content_x11_esconf_priority_notify_id=0;
	}
}

/* Check if we should workaround unmapped window for requested window and set up workaround */
static void _esdashboard_window_content_x11_on_workaround_state_changed(EsdashboardWindowContentX11 *self,
																	gpointer inUserData)
{
	EsdashboardWindowContentX11Private		*priv;
	EsdashboardWindowTrackerWindowState		windowState;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	priv=self->priv;

	/* Handle state change of window */
	windowState=esdashboard_window_tracker_window_get_state(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window));

	switch(priv->workaroundMode)
	{
		case ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_UNMINIMIZING:
			/* Check if window is unminized now, then update content texture and
			 * minimize window again.
			 */
			if(!(windowState & ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED))
			{
				if(priv->texture &&
					priv->workaroundMode!=ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_NONE &&
					priv->isMapped==TRUE)
				{
					/* Copy current texture as it might get inaccessible. If we copy it now
					 * when can draw the last image known. If we can copy it successfully
					 * replace current texture with the copied one.
					 */
					CoglPixelFormat			textureFormat;
					guint					textureWidth;
					guint					textureHeight;
					gint					textureSize;
					guint8					*textureData;

					textureFormat=cogl_texture_get_format(priv->texture);
					textureSize=cogl_texture_get_data(priv->texture, textureFormat, 0, NULL);
					textureWidth=cogl_texture_get_width(priv->texture);
					textureHeight=cogl_texture_get_height(priv->texture);
					textureData=g_malloc(textureSize);
					if(textureData)
					{
						CoglTexture			*copyTexture;
						gint				copyTextureSize;
#if COGL_VERSION_CHECK(1, 18, 0)
						ClutterBackend		*backend;
						CoglContext			*context;
						CoglError			*error;
#endif

						/* Get texture data to copy */
						copyTextureSize=cogl_texture_get_data(priv->texture, textureFormat, 0, textureData);
						if(copyTextureSize)
						{
#if COGL_VERSION_CHECK(1, 18, 0)
							error=NULL;

							backend=clutter_get_default_backend();
							context=clutter_backend_get_cogl_context(backend);
							copyTexture=cogl_texture_2d_new_from_data(context,
																		textureWidth,
																		textureHeight,
																		textureFormat,
																		0,
																		textureData,
																		&error);

							if(!copyTexture || error)
							{
								/* Show warning */
								g_warning("Could not create copy of texture of mininized window '%s': %s",
											esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)),
											(error && error->message) ? error->message : "Unknown error");

								/* Release allocated resources */
								if(copyTexture)
								{
									cogl_object_unref(copyTexture);
									copyTexture=NULL;
								}

								if(error)
								{
									cogl_error_free(error);
									error=NULL;
								}
							}
#else
							copyTexture=cogl_texture_new_from_data(textureWidth,
																	textureHeight,
																	COGL_TEXTURE_NONE,
																	textureFormat,
																	textureFormat,
																	0,
																	textureData);
							if(!copyTexture)
							{
								/* Show warning */
								g_warning("Could not create copy of texture of mininized window '%s'",
											esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
							}
#endif

							if(copyTexture)
							{
								cogl_object_unref(priv->texture);
								priv->texture=copyTexture;
							}
						}
							else g_warning("Could not determine size of texture of minimized window '%s'",
											esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
					}
						else
						{
							g_warning("Could not allocate memory for copy of texture of mininized window '%s'",
										esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
						}
				}

				esdashboard_window_tracker_window_hide(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window));
				priv->workaroundMode=ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_REMINIMIZING;
			}
			break;

		case ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_REMINIMIZING:
			/* Check if window is now minized again, so stop workaround and
			 * disconnecting signals.
			 */
			if(windowState & ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED)
			{
				priv->workaroundMode=ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_DONE;
				if(priv->workaroundStateSignalID)
				{
					g_signal_handler_disconnect(priv->windowTracker, priv->workaroundStateSignalID);
					priv->workaroundStateSignalID=0;
				}
			}
			break;

		default:
			/* We should never get here but if we do it is more or less
			 * a critical error. Ensure that window is minimized (again)
			 * and stop esdashboard.
			 */
			esdashboard_window_tracker_window_hide(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window));
			g_assert_not_reached();
			break;
	}
}

static void _esdashboard_window_content_x11_setup_workaround(EsdashboardWindowContentX11 *self, EsdashboardWindowTrackerWindowX11 *inWindow)
{
	EsdashboardWindowContentX11Private		*priv;
	gboolean								doWorkaround;
	EsdashboardWindowTrackerWindowState		windowState;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inWindow!=NULL && ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	priv=self->priv;

	/* Check if should workaround unmapped windows at all */
	doWorkaround=esconf_channel_get_bool(esdashboard_application_get_esconf_channel(NULL),
											WORKAROUND_UNMAPPED_WINDOW_ESCONF_PROP,
											DEFAULT_WORKAROUND_UNMAPPED_WINDOW);
	if(!doWorkaround) return;

	/* Only workaround unmapped windows */
	windowState=esdashboard_window_tracker_window_get_state(ESDASHBOARD_WINDOW_TRACKER_WINDOW(inWindow));
	if(!(windowState & ESDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED)) return;

	/* Check if workaround is already set up */
	if(priv->workaroundMode!=ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_NONE) return;

	/* Set flag that workaround is (going to be) set up */
	priv->workaroundMode=ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_UNMINIMIZING;

	/* The workaround is as follows:
	 *
	 * 1.) Set up signal handlers to get notified about changes of window
	 * 2.) Unminimize window
	 * 3.) If window is visible it will be activated by design, so reactivate
	 *     last active window
	 * 4.) Minimize window again
	 * 5.) Stop watching for changes of window by disconnecting signal handlers
	 */
	priv->workaroundStateSignalID=g_signal_connect_swapped(priv->windowTracker,
															"window-state-changed",
															G_CALLBACK(_esdashboard_window_content_x11_on_workaround_state_changed),
															self);
	esdashboard_window_tracker_window_show(ESDASHBOARD_WINDOW_TRACKER_WINDOW(inWindow));
}

/* Check extension and set up basics */
static void _esdashboard_window_content_x11_check_extension(void)
{
	Display		*display G_GNUC_UNUSED;
#ifdef HAVE_XDAMAGE
	int			damageError=0;
#endif
#ifdef HAVE_XCOMPOSITE
	int			compositeEventBase=0;
	int			compositeError=0;
	int			compositeMajor, compositeMinor;
#endif

	/* Check if we have already checked extensions */
	if(_esdashboard_window_content_x11_have_checked_extensions!=FALSE) return;

	/* Mark that we have check for extensions regardless of any error */
	_esdashboard_window_content_x11_have_checked_extensions=TRUE;

	/* Get display */
	display=_esdashboard_window_content_x11_get_display();

	/* Check for composite extenstion */
	_esdashboard_window_content_x11_have_composite_extension=FALSE;
#ifdef HAVE_XCOMPOSITE
	if(G_LIKELY(display!=None) &&
		XCompositeQueryExtension(display, &compositeEventBase, &compositeError))
	{
		compositeMajor=compositeMinor=0;
		if(XCompositeQueryVersion(display, &compositeMajor, &compositeMinor))
		{
			if(compositeMajor>=COMPOSITE_VERSION_MIN_MAJOR && compositeMinor>=COMPOSITE_VERSION_MIN_MINOR)
			{
				_esdashboard_window_content_x11_have_composite_extension=TRUE;
			}
				else
				{
					g_warning("Need at least version %d.%d of composite extension but found %d.%d - using only fallback images",
								COMPOSITE_VERSION_MIN_MAJOR, COMPOSITE_VERSION_MIN_MINOR, compositeMajor, compositeMinor);
				}
		}
			else g_warning("Query for X composite extension failed - using only fallback imagess");
	}
		else g_warning("X does not support composite extension - using only fallback images");
#endif

	/* Get base of damage event in X */
	_esdashboard_window_content_x11_have_damage_extension=FALSE;
	_esdashboard_window_content_x11_damage_event_base=0;

#ifdef HAVE_XDAMAGE
	if(G_LIKELY(display!=None) &&
		XDamageQueryExtension(display, &_esdashboard_window_content_x11_damage_event_base, &damageError))
	{
		_esdashboard_window_content_x11_have_damage_extension=TRUE;
	}
		else
		{
			g_warning("Query for X damage extension resulted in error code %d - using only still images of windows",
						damageError);
		}
#endif
}

/* Suspension state of application changed */
static void _esdashboard_window_content_x11_on_application_suspended_changed(EsdashboardWindowContentX11 *self,
																				GParamSpec *inSpec,
																				gpointer inUserData)
{
	EsdashboardWindowContentX11Private		*priv;
	EsdashboardApplication					*app;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;
	app=ESDASHBOARD_APPLICATION(inUserData);

	/* Get application suspend state */
	priv->isAppSuspended=esdashboard_application_is_suspended(app);

	/* If application is suspended then suspend this window too ... */
	if(priv->isAppSuspended)
	{
		_esdashboard_window_content_x11_suspend(self);
	}
		/* ... otherwise resume window if it is mapped */
		else
		{
			if(priv->isMapped) _esdashboard_window_content_x11_resume(self);
		}
}

/* Filter X events for damages */
static void _esdashboard_window_content_x11_handle_x_event(EsdashboardWindowContentX11 *self,
															XEvent *inXEvent)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inXEvent);

	priv=self->priv;

	/* Check for mapped, unmapped related X events as pixmap, damage, texture etc.
	 * needs to get resumed (acquired) or suspended (released)
	 */
	if(inXEvent->xany.window==priv->xWindowID)
	{
		switch(inXEvent->type)
		{
			case MapNotify:
			case ConfigureNotify:
				priv->isMapped=TRUE;
				if(!priv->isAppSuspended) _esdashboard_window_content_x11_resume(self);
				break;

			case UnmapNotify:
			case DestroyNotify:
				priv->isMapped=FALSE;
				_esdashboard_window_content_x11_suspend(self);
				break;

			default:
				/* We do not handle this type of X event, drop through ... */
				break;
		}
	}

	/* Check for damage event */
#ifdef HAVE_XDAMAGE
	if(_esdashboard_window_content_x11_have_damage_extension &&
		_esdashboard_window_content_x11_damage_event_base &&
		inXEvent->type==(_esdashboard_window_content_x11_damage_event_base + XDamageNotify) &&
		((XDamageNotifyEvent*)inXEvent)->damage==priv->damage &&
		priv->workaroundMode==ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_NONE)
	{
		/* Update texture for live window content */
		clutter_content_invalidate(CLUTTER_CONTENT(self));
	}
#endif
}

static ClutterX11FilterReturn _esdashboard_window_content_x11_on_x_event(XEvent *inXEvent, ClutterEvent *inEvent, gpointer inUserData)
{
	EsdashboardWindowContentX11				*self;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(inUserData), CLUTTER_X11_FILTER_CONTINUE);

	self=ESDASHBOARD_WINDOW_CONTENT_X11(inUserData);

	/* Call X event handler */
	_esdashboard_window_content_x11_handle_x_event(self, inXEvent);

	/* Always return FILTER_CONTINUE value to let other components handle this
	 * event also.
	 */
	return(CLUTTER_X11_FILTER_CONTINUE);
}

#ifdef CLUTTER_WINDOWING_GDK
static GdkFilterReturn _esdashboard_window_content_x11_on_gdkx_event(GdkXEvent *inXEvent, GdkEvent *inEvent, gpointer inUserData)
{
	EsdashboardWindowContentX11				*self;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(inUserData), GDK_FILTER_CONTINUE);

	self=ESDASHBOARD_WINDOW_CONTENT_X11(inUserData);

	/* Call X event handler */
	_esdashboard_window_content_x11_handle_x_event(self, inXEvent);

	/* Always return FILTER_CONTINUE value to let other components handle this
	 * event also.
	 */
	return(GDK_FILTER_CONTINUE);
}
#endif

/* Release all resources used by this instance */
static void _esdashboard_window_content_x11_release_resources(EsdashboardWindowContentX11 *self)
{
	EsdashboardWindowContentX11Private		*priv;
	Display									*display;
	gint									trapError;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));

	priv=self->priv;

	/* This live update will be suspended so remove it from queue */
	_esdashboard_window_content_x11_resume_on_idle_remove(self);

	/* Get display as it used more than once ;) */
	display=_esdashboard_window_content_x11_get_display();

	/* Release resources. It might be important to release them
	 * in reverse order as they were created.
	 */
	clutter_x11_trap_x_errors();
	{
		if(priv->texture)
		{
			cogl_object_unref(priv->texture);
			priv->texture=NULL;
		}

#ifdef HAVE_XDAMAGE
		if(priv->damage!=None)
		{
			XDamageDestroy(display, priv->damage);
			XSync(display, False);
			priv->damage=None;
		}
#endif
		if(priv->pixmap!=None)
		{
			XFreePixmap(display, priv->pixmap);
			priv->pixmap=None;
		}

		if(priv->xWindowID!=None)
		{
#ifdef HAVE_XCOMPOSITE
			if(_esdashboard_window_content_x11_have_composite_extension)
			{
				XCompositeUnredirectWindow(display, priv->xWindowID, CompositeRedirectAutomatic);
				XSync(display, False);
			}
#endif
			priv->xWindowID=None;
		}

		/* Window is suspended now */
		if(priv->isSuspended!=TRUE)
		{
			priv->isSuspended=TRUE;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_SUSPENDED]);
		}
	}

	/* Check if everything went well */
	trapError=clutter_x11_untrap_x_errors();
	if(trapError!=0)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"X error %d occured while releasing resources for window '%s'",
							trapError,
							esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
		return;
	}

	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Released resources for window '%s' to handle live texture updates",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
}

/* Suspend from handling live updates */
static void _esdashboard_window_content_x11_suspend(EsdashboardWindowContentX11 *self)
{
	EsdashboardWindowContentX11Private		*priv;
	Display									*display;
	gint									trapError;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));

	priv=self->priv;

	/* This live update will be suspended so remove it from queue */
	_esdashboard_window_content_x11_resume_on_idle_remove(self);

	/* Get display as it used more than once ;) */
	display=_esdashboard_window_content_x11_get_display();

	/* Release resources */
	clutter_x11_trap_x_errors();
	{
		/* Suspend live updates from texture */
		if(priv->texture && !priv->isFallback)
		{
#ifdef HAVE_XDAMAGE
			cogl_texture_pixmap_x11_set_damage_object(COGL_TEXTURE_PIXMAP_X11(priv->texture), 0, 0);
#endif
		}

		/* Release damage */
#ifdef HAVE_XDAMAGE
		if(priv->damage!=None)
		{
			XDamageDestroy(display, priv->damage);
			XSync(display, False);
			priv->damage=None;
		}
#endif

		/* Release pixmap */
		if(priv->pixmap!=None)
		{
			XFreePixmap(display, priv->pixmap);
			priv->pixmap=None;
		}

		/* Window is suspended now */
		if(priv->isSuspended!=TRUE)
		{
			priv->isSuspended=TRUE;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_SUSPENDED]);
		}
	}

	/* Check if everything went well */
	trapError=clutter_x11_untrap_x_errors();
	if(trapError!=0)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"X error %d occured while suspending '%s",
							trapError,
							esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
		return;
	}

	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Successfully suspended live texture updates for window '%s'",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
}

/* Resume to handle live window updates */
static gboolean _esdashboard_window_content_x11_resume_on_idle(gpointer inUserData)
{
	EsdashboardWindowContentX11				*self;
	EsdashboardWindowContentX11Private		*priv;
	GList									*queueEntry;
	Display									*display;
	CoglContext								*context;
	GError									*error;
	gint									trapError;
	CoglTexture								*windowTexture;
	gboolean								doContinueSource;

	error=NULL;
	windowTexture=NULL;

	/* Get window content object from first entry in queue and remove it from queue */
	queueEntry=g_list_first(_esdashboard_window_content_x11_resume_idle_queue);
	if(!queueEntry)
	{
		g_warning("Resume handler called for empty queue.");

		/* Queue must be empty but ensure it will */
		if(_esdashboard_window_content_x11_resume_idle_queue)
		{
			ESDASHBOARD_DEBUG(NULL, WINDOWS, "Ensuring that window content resume queue is empty");
			g_list_free(_esdashboard_window_content_x11_resume_idle_queue);
			_esdashboard_window_content_x11_resume_idle_queue=NULL;
		}

		/* Queue must be empty so remove idle source */
		_esdashboard_window_content_x11_resume_idle_id=0;
		return(G_SOURCE_REMOVE);
	}

	self=ESDASHBOARD_WINDOW_CONTENT_X11(queueEntry->data);
	priv=self->priv;
	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Entering idle source with ID %u for window resume of '%s'",
						_esdashboard_window_content_x11_resume_idle_id,
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));

	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Removing queued entry %p for window resume of '%s'",
						queueEntry,
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
	_esdashboard_window_content_x11_resume_idle_queue=g_list_delete_link(_esdashboard_window_content_x11_resume_idle_queue, queueEntry);
	if(_esdashboard_window_content_x11_resume_idle_queue)
	{
		doContinueSource=G_SOURCE_CONTINUE;
	}
		else
		{
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"Resume idle source with ID %u will be remove because queue is empty",
								_esdashboard_window_content_x11_resume_idle_id);

			doContinueSource=G_SOURCE_REMOVE;
			_esdashboard_window_content_x11_resume_idle_id=0;
		}


	/* We need at least the X composite extension to display images of windows
	 * if still images or live updated ones
	 */
	if(!_esdashboard_window_content_x11_have_composite_extension)
	{
		return(doContinueSource);
	}

	/* Get display as it used more than once ;) */
	display=_esdashboard_window_content_x11_get_display();

	/* Set up resources */
	clutter_x11_trap_x_errors();
	while(1)
	{
#ifdef HAVE_XCOMPOSITE
		/* Get pixmap to render texture for */
		priv->pixmap=XCompositeNameWindowPixmap(display, priv->xWindowID);
		XSync(display, False);
		if(priv->pixmap==None)
		{
			g_warning("Could not get pixmap for window '%s", esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));

			/* Set flag to suspend window content after resuming because of error */
			priv->suspendAfterResumeOnIdle=TRUE;
			break;
		}
#else
		/* We should never get here as existance of composite extension was checked before */
		g_critical("Cannot resume window '%s' as composite extension is not available",
					esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
		break;
#endif

		/* Create cogl X11 texture for live updates */
		context=clutter_backend_get_cogl_context(clutter_get_default_backend());
		windowTexture=COGL_TEXTURE(cogl_texture_pixmap_x11_new(context, priv->pixmap, FALSE, &error));
		if(!windowTexture || error)
		{
			/* Creating texture may fail if window is _NOT_ on active workspace
			 * so display error message just as debug message (this time)
			 */
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"Could not create texture for window '%s': %s",
								esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)),
								error ? error->message : "Unknown error");
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}

			if(windowTexture)
			{
				cogl_object_unref(windowTexture);
				windowTexture=NULL;
			}

			/* Set flag to suspend window content after resuming because of error */
			priv->suspendAfterResumeOnIdle=TRUE;

			break;
		}

		/* Set up damage to get notified about changed in pixmap */
#ifdef HAVE_XDAMAGE
		if(_esdashboard_window_content_x11_have_damage_extension)
		{
			priv->damage=XDamageCreate(display, priv->pixmap, XDamageReportBoundingBox);
			XSync(display, False);
			if(priv->damage==None)
			{
				g_warning("Could not create damage for window '%s' - using still image of window", esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
			}
		}
#endif

		/* Release old texture (should be the fallback texture) and set new texture */
		if(priv->texture)
		{
			cogl_object_unref(priv->texture);
			priv->texture=windowTexture;
		}

		/* Set damage to new window texture */
#ifdef HAVE_XDAMAGE
		if(_esdashboard_window_content_x11_have_damage_extension &&
			priv->damage!=None)
		{
			cogl_texture_pixmap_x11_set_damage_object(COGL_TEXTURE_PIXMAP_X11(priv->texture), priv->damage, COGL_TEXTURE_PIXMAP_X11_DAMAGE_BOUNDING_BOX);
		}
#endif

		/* Now we use the window as texture and not the fallback texture anymore */
		priv->isFallback=FALSE;

		/* Window is not suspended anymore */
		if(priv->isSuspended!=FALSE)
		{
			priv->isSuspended=FALSE;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_SUSPENDED]);
		}

		/* Invalidate content to get it redrawn as soon as possible */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* We were able to set up window content so this window is definitely mapped */
		priv->isMapped=TRUE;

		/* End reached so break to get out of while loop */
		break;
	}

	/* Check if window content should be suspended again after resume was done,
	 * e.g. initial window content creation in suspended daemon mode.
	 */
	if(priv->suspendAfterResumeOnIdle)
	{
		_esdashboard_window_content_x11_suspend(self);
		priv->suspendAfterResumeOnIdle=FALSE;
	}

	/* Check if everything went well */
	trapError=clutter_x11_untrap_x_errors();
	if(trapError!=0)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"X error %d occured while resuming window '%s",
							trapError,
							esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
		return(doContinueSource);
	}

	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Resuming live texture updates for window '%s'",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
	return(doContinueSource);
}

static void _esdashboard_window_content_x11_resume(EsdashboardWindowContentX11 *self)
{
	EsdashboardWindowContentX11Private		*priv;
	Display									*display;
	CoglContext								*context;
	GError									*error;
	gint									trapError;
	CoglTexture								*windowTexture;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(self->priv->window);

	priv=self->priv;
	error=NULL;
	windowTexture=NULL;

	/* Check if to use new experimental code to resume window content
	 * in an idle source.
	 */
	if(_esdashboard_window_content_x11_window_creation_priority>0)
	{
		_esdashboard_window_content_x11_resume_on_idle_add(self);
		return;
	}

	/* We need at least the X composite extension to display images of windows
	 * if still images or live updated ones
	 */
	if(!_esdashboard_window_content_x11_have_composite_extension) return;

	/* Get display as it used more than once ;) */
	display=_esdashboard_window_content_x11_get_display();

	/* Set up resources */
	clutter_x11_trap_x_errors();
	while(1)
	{
#ifdef HAVE_XCOMPOSITE
		/* Get pixmap to render texture for */
		priv->pixmap=XCompositeNameWindowPixmap(display, priv->xWindowID);
		XSync(display, False);
		if(priv->pixmap==None)
		{
			g_warning("Could not get pixmap for window '%s", esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
			_esdashboard_window_content_x11_suspend(self);
			break;
		}
#else
		/* We should never get here as existance of composite extension was checked before */
		g_critical("Cannot resume window '%s' as composite extension is not available",
					esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
		break;
#endif

		/* Create cogl X11 texture for live updates */
		context=clutter_backend_get_cogl_context(clutter_get_default_backend());
		windowTexture=COGL_TEXTURE(cogl_texture_pixmap_x11_new(context, priv->pixmap, FALSE, &error));
		if(!windowTexture || error)
		{
			/* Creating texture may fail if window is _NOT_ on active workspace
			 * so display error message just as debug message (this time)
			 */
			ESDASHBOARD_DEBUG(self, WINDOWS,
								"Could not create texture for window '%s': %s",
								esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)),
								error ? error->message : "Unknown error");
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}

			if(windowTexture)
			{
				cogl_object_unref(windowTexture);
				windowTexture=NULL;
			}

			_esdashboard_window_content_x11_suspend(self);

			break;
		}

		/* Set up damage to get notified about changed in pixmap */
#ifdef HAVE_XDAMAGE
		if(_esdashboard_window_content_x11_have_damage_extension)
		{
			priv->damage=XDamageCreate(display, priv->pixmap, XDamageReportBoundingBox);
			XSync(display, False);
			if(priv->damage==None)
			{
				g_warning("Could not create damage for window '%s' - using still image of window", esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
			}
		}
#endif

		/* Release old texture (should be the fallback texture) and set new texture */
		if(priv->texture)
		{
			cogl_object_unref(priv->texture);
			priv->texture=windowTexture;
		}

		/* Set damage to new window texture */
#ifdef HAVE_XDAMAGE
		if(_esdashboard_window_content_x11_have_damage_extension &&
			priv->damage!=None)
		{
			cogl_texture_pixmap_x11_set_damage_object(COGL_TEXTURE_PIXMAP_X11(priv->texture), priv->damage, COGL_TEXTURE_PIXMAP_X11_DAMAGE_BOUNDING_BOX);
		}
#endif

		/* Now we use the window as texture and not the fallback texture anymore */
		priv->isFallback=FALSE;

		/* Window is not suspended anymore */
		if(priv->isSuspended!=FALSE)
		{
			priv->isSuspended=FALSE;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_SUSPENDED]);
		}

		/* Invalidate content to get it redrawn as soon as possible */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* We were able to set up window content so this window is definitely mapped */
		priv->isMapped=TRUE;

		/* End reached so break to get out of while loop */
		break;
	}

	/* Check if everything went well */
	trapError=clutter_x11_untrap_x_errors();
	if(trapError!=0)
	{
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"X error %d occured while resuming window '%s",
							trapError,
							esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
		return;
	}

	ESDASHBOARD_DEBUG(self, WINDOWS,
						"Resuming live texture updates for window '%s'",
						esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
}

/* Find X window for window frame of given X window content */
static Window _esdashboard_window_content_x11_get_window_frame_xid(Display *inDisplay,
																	EsdashboardWindowTrackerWindowX11 *inWindow)
{
	Window				xWindowID;
	Window				iterXWindowID;
	Window				rootXWindowID;
	Window				foundXWindowID;
	GdkDisplay			*gdkDisplay;
	GdkWindow			*gdkWindow;
	GdkWMDecoration		gdkWindowDecoration;

	g_return_val_if_fail(inDisplay, 0);
	g_return_val_if_fail(inWindow, 0);

	/* Get X window */
	xWindowID=esdashboard_window_tracker_window_x11_get_xid(inWindow);
	g_return_val_if_fail(xWindowID!=0, 0);

	/* Check if window is client side decorated and if it has no decorations
	 * so skip finding window frame and behave like we did not found it.
	 */
	XSync(inDisplay, False);

	gdkDisplay=gdk_x11_lookup_xdisplay(inDisplay);
	if(!gdkDisplay) gdkDisplay=gdk_display_get_default();

	gdkWindow=gdk_x11_window_foreign_new_for_display(gdkDisplay, xWindowID);
	if(gdkWindow)
	{
		if(!gdk_window_get_decorations(gdkWindow, &gdkWindowDecoration) ||
			gdkWindowDecoration==0)
		{
			ESDASHBOARD_DEBUG(inWindow, WINDOWS,
								"Window '%s' has either CSD not enabled or no decorations applied so skip finding window frame",
								esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(inWindow)));

			/* Release allocated resources */
			g_object_unref(gdkWindow);

			/* Skip finding window frame and return "not-found" result */
			return(0);
		}
		g_object_unref(gdkWindow);
	}
		else
		{
			ESDASHBOARD_DEBUG(inWindow, WINDOWS,
								"Could not get window decoration from window '%s'",
								esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(inWindow)));
		}

	/* Iterate through X window tree list upwards until root window reached.
	 * The last X window before root window is the one we are looking for.
	 */
	rootXWindowID=0;
	foundXWindowID=0;
	for(iterXWindowID=xWindowID; iterXWindowID && iterXWindowID!=rootXWindowID; )
	{
		Window	*children;
		guint	numberChildren;

		children=NULL;
		numberChildren=0;
		foundXWindowID=iterXWindowID;
		if(!XQueryTree(inDisplay, iterXWindowID, &rootXWindowID, &iterXWindowID, &children, &numberChildren))
		{
			iterXWindowID=0;
		}
		if(children) XFree(children);
	}

	/* Return found X window ID */
	return(foundXWindowID);
}

/* Window's was closed */
static void _esdashboard_window_content_x11_on_window_closed(EsdashboardWindowContentX11 *self,
																gpointer inUserData)
{
	EsdashboardWindowContentX11Private	*priv;
	EsdashboardWindowTrackerWindow		*window G_GNUC_UNUSED;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inUserData));

	priv=self->priv;
	window=ESDASHBOARD_WINDOW_TRACKER_WINDOW(inUserData);

	/* X11 window was closed so suspend this window content
	 * to keep current texture of window and prevent futher
	 * live updates.
	 */
	_esdashboard_window_content_x11_suspend(self);

	/* Disconnect all signal handler from window as it was closed */
	if(priv->windowClosedSignalID)
	{
		g_signal_handler_disconnect(priv->window, priv->windowClosedSignalID);
		priv->windowClosedSignalID=0;
	}

	g_signal_handlers_disconnect_by_data(priv->window, self);

	/* libwnck resources should never be freed. Just set to NULL */
	priv->window=NULL;
}

/* Set window to handle and to display */
static void _esdashboard_window_content_x11_set_window(EsdashboardWindowContentX11 *self, EsdashboardWindowTrackerWindowX11 *inWindow)
{
	EsdashboardWindowContentX11Private		*priv;
	EsdashboardApplication					*application;
	Display									*display;
	GdkPixbuf								*windowIcon;
	XWindowAttributes						windowAttrs;
#if COGL_VERSION_CHECK(1, 18, 0)
	ClutterBackend							*backend;
	CoglContext								*context;
	CoglError								*error;
#endif

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inWindow!=NULL && ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));
	g_return_if_fail(self->priv->window==NULL);
	g_return_if_fail(self->priv->xWindowID==0);
	g_return_if_fail(self->priv->windowClosedSignalID==0);

	priv=self->priv;

	/* Freeze notifications and collect them */
	g_object_freeze_notify(G_OBJECT(self));

	/* Get display as it used more than once ;) */
	display=_esdashboard_window_content_x11_get_display();

	/* Set new value */
	priv->window=inWindow;

	/* Connect signal handler to get notified if window was closed
	 * to suspend this window content.
	 */
	priv->windowClosedSignalID=g_signal_connect_swapped(priv->window,
														"closed",
														G_CALLBACK(_esdashboard_window_content_x11_on_window_closed),
														self);

	/* Create fallback texture first in case we cannot create
	 * a live updated texture for window in the next steps
	 */
	windowIcon=esdashboard_window_tracker_window_get_icon(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window));
#if COGL_VERSION_CHECK(1, 18, 0)
	error=NULL;

	backend=clutter_get_default_backend();
	context=clutter_backend_get_cogl_context(backend);
	priv->texture=cogl_texture_2d_new_from_data(context,
												gdk_pixbuf_get_width(windowIcon),
												gdk_pixbuf_get_height(windowIcon),
												gdk_pixbuf_get_has_alpha(windowIcon) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
												gdk_pixbuf_get_rowstride(windowIcon),
												gdk_pixbuf_get_pixels(windowIcon),
												&error);

	if(!priv->texture || error)
	{
		/* Show warning */
		g_warning("Could not create fallback texture for window '%s': %s",
					esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)),
					(error && error->message) ? error->message : "Unknown error");

		/* Release allocated resources */
		if(priv->texture)
		{
			cogl_object_unref(priv->texture);
			priv->texture=NULL;
		}

		if(error)
		{
			cogl_error_free(error);
			error=NULL;
		}
	}
#else
	priv->texture=cogl_texture_new_from_data(gdk_pixbuf_get_width(windowIcon),
												gdk_pixbuf_get_height(windowIcon),
												COGL_TEXTURE_NONE,
												gdk_pixbuf_get_has_alpha(windowIcon) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
												COGL_PIXEL_FORMAT_ANY,
												gdk_pixbuf_get_rowstride(windowIcon),
												gdk_pixbuf_get_pixels(windowIcon));
#endif

	priv->isFallback=TRUE;

	/* Get X window and its attributes */
	if(priv->includeWindowFrame)
	{
		priv->xWindowID=_esdashboard_window_content_x11_get_window_frame_xid(display, priv->window);
	}

	if(!priv->xWindowID)
	{
		priv->xWindowID=esdashboard_window_tracker_window_x11_get_xid(priv->window);
	}

	if(!XGetWindowAttributes(display, priv->xWindowID, &windowAttrs))
	{
		g_warning("Could not get attributes of window '%s'", esdashboard_window_tracker_window_get_name(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window)));
		XSync(display, False);
	}

	/* We need at least the X composite extension to display images of windows
	 * if still images or live updated ones by redirecting window
	 */
#ifdef HAVE_XCOMPOSITE
	if(_esdashboard_window_content_x11_have_composite_extension)
	{
		/* Redirect window */
		XCompositeRedirectWindow(display, priv->xWindowID, CompositeRedirectAutomatic);
		XSync(display, False);
	}
#endif

	/* We are interested in receiving mapping events of windows */
	XSelectInput(display, priv->xWindowID, windowAttrs.your_event_mask | StructureNotifyMask);

	/* Acquire new window and handle live updates */
	_esdashboard_window_content_x11_resume(self);
	priv->isMapped=!priv->isSuspended;

	/* But suspend window immediately again if application is suspended
	 * (esdashboard runs in daemon mode and is not active currently)
	 */
	application=esdashboard_application_get_default();
	if(esdashboard_application_is_suspended(application))
	{
		if(_esdashboard_window_content_x11_window_creation_priority>0)
		{
			priv->suspendAfterResumeOnIdle=TRUE;
		}
			else
			{
				_esdashboard_window_content_x11_suspend(self);
			}
	}

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_WINDOW]);

	/* Thaw notifications and send them now */
	g_object_thaw_notify(G_OBJECT(self));

	/* Set up workaround mechanism for unmapped windows if wanted and needed */
	_esdashboard_window_content_x11_setup_workaround(self, inWindow);
}


/* IMPLEMENTATION: ClutterContent */

/* Paint texture */
static void _xdashboard_window_content_clutter_content_iface_paint_content(ClutterContent *inContent,
																			ClutterActor *inActor,
																			ClutterPaintNode *inRootNode)
{
	EsdashboardWindowContentX11				*self=ESDASHBOARD_WINDOW_CONTENT_X11(inContent);
	EsdashboardWindowContentX11Private		*priv=self->priv;
	ClutterScalingFilter					minFilter, magFilter;
	ClutterPaintNode						*node;
	ClutterActorBox							textureAllocationBox;
	ClutterActorBox							textureCoordBox;
	ClutterActorBox							outlineBox;
	ClutterColor							color;
	guint8									opacity;
	ClutterColor							outlineColor;
	ClutterActorBox							outlinePath;

	/* Check if we have a texture to paint */
	if(priv->texture==NULL) return;

	/* Get needed data for painting */
	clutter_actor_box_init(&textureCoordBox, 0.0f, 0.0f, 1.0f, 1.0f);
	clutter_actor_get_content_box(inActor, &textureAllocationBox);
	clutter_actor_get_content_box(inActor, &outlineBox);
	clutter_actor_get_content_scaling_filters(inActor, &minFilter, &magFilter);
	opacity=clutter_actor_get_paint_opacity(inActor);

	color.red=opacity;
	color.green=opacity;
	color.blue=opacity;
	color.alpha=opacity;

	/* Draw background if texture is a fallback */
	if(priv->isFallback)
	{
		ClutterColor					backgroundColor;

		/* Set up background color */
		backgroundColor.red=0;
		backgroundColor.green=0;
		backgroundColor.blue=0;
		backgroundColor.alpha=opacity;

		/* Draw background */
		node=clutter_color_node_new(&backgroundColor);
		clutter_paint_node_set_name(node, "fallback-background");
		clutter_paint_node_add_rectangle(node, &outlineBox);
		clutter_paint_node_add_child(inRootNode, node);
		clutter_paint_node_unref(node);
	}

	/* Determine actor box allocation to draw texture into when unmapped window
	 * icon (fallback) will be drawn. We can skip calculation if unmapped window
	 * icon should be expanded in both (x and y) direction.
	 */
	if(priv->isFallback &&
		(!priv->unmappedWindowIconXFill || !priv->unmappedWindowIconYFill))
	{
		gfloat							allocationWidth;
		gfloat							allocationHeight;

		/* Get width and height of allocation */
		allocationWidth=(outlineBox.x2-outlineBox.x1);
		allocationHeight=(outlineBox.y2-outlineBox.y1);

		/* Determine left and right boundary of unmapped window icon
		 * if unmapped window icon should not expand in X axis.
		 */
		if(!priv->unmappedWindowIconXFill)
		{
			gfloat						offset;
			gfloat						textureWidth;
			gfloat						oversize;

			/* Get scaled width of unmapped window icon */
			textureWidth=cogl_texture_get_width(priv->texture);
			textureWidth*=priv->unmappedWindowIconXScale;

			/* Get boundary in X axis depending on gravity and scaled width */
			offset=(priv->unmappedWindowIconXAlign*allocationWidth);
			switch(priv->unmappedWindowIconAnchorPoint)
			{
				/* Align to left boundary.
				 * This is also the default if gravity is none or undefined.
				 */
				default:
				case ESDASHBOARD_ANCHOR_POINT_NONE:
				case ESDASHBOARD_ANCHOR_POINT_WEST:
				case ESDASHBOARD_ANCHOR_POINT_NORTH_WEST:
				case ESDASHBOARD_ANCHOR_POINT_SOUTH_WEST:
					break;

				/* Align to center of X axis */
				case ESDASHBOARD_ANCHOR_POINT_CENTER:
				case ESDASHBOARD_ANCHOR_POINT_NORTH:
				case ESDASHBOARD_ANCHOR_POINT_SOUTH:
					offset-=(textureWidth/2.0f);
					break;

				/* Align to right boundary */
				case ESDASHBOARD_ANCHOR_POINT_EAST:
				case ESDASHBOARD_ANCHOR_POINT_NORTH_EAST:
				case ESDASHBOARD_ANCHOR_POINT_SOUTH_EAST:
					offset-=textureWidth;
					break;
			}

			/* Set boundary in X axis */
			textureAllocationBox.x1=outlineBox.x1+offset;
			textureAllocationBox.x2=textureAllocationBox.x1+textureWidth;

			/* Clip texture in X axis if it does not fit into allocation */
			if(textureAllocationBox.x1<outlineBox.x1)
			{
				oversize=outlineBox.x1-textureAllocationBox.x1;
				textureCoordBox.x1=oversize/textureWidth;
				textureAllocationBox.x1=outlineBox.x1;
			}

			if(textureAllocationBox.x2>outlineBox.x2)
			{
				oversize=textureAllocationBox.x2-outlineBox.x2;
				textureCoordBox.x2=1.0f-(oversize/textureWidth);
				textureAllocationBox.x2=outlineBox.x2;
			}
		}

		/* Determine left and right boundary of unmapped window icon
		 * if unmapped window icon should not expand in X axis.
		 */
		if(!priv->unmappedWindowIconYFill)
		{
			gfloat						offset;
			gfloat						textureHeight;
			gfloat						oversize;

			/* Get scaled width of unmapped window icon */
			textureHeight=cogl_texture_get_height(priv->texture);
			textureHeight*=priv->unmappedWindowIconYScale;

			/* Get boundary in Y axis depending on gravity and scaled width */
			offset=(priv->unmappedWindowIconYAlign*allocationHeight);
			switch(priv->unmappedWindowIconAnchorPoint)
			{
				/* Align to upper boundary.
				 * This is also the default if gravity is none or undefined.
				 */
				default:
				case ESDASHBOARD_ANCHOR_POINT_NONE:
				case ESDASHBOARD_ANCHOR_POINT_NORTH:
				case ESDASHBOARD_ANCHOR_POINT_NORTH_WEST:
				case ESDASHBOARD_ANCHOR_POINT_NORTH_EAST:
					break;

				/* Align to center of Y axis */
				case ESDASHBOARD_ANCHOR_POINT_CENTER:
				case ESDASHBOARD_ANCHOR_POINT_WEST:
				case ESDASHBOARD_ANCHOR_POINT_EAST:
					offset-=(textureHeight/2.0f);
					break;

				/* Align to lower boundary */
				case ESDASHBOARD_ANCHOR_POINT_SOUTH:
				case ESDASHBOARD_ANCHOR_POINT_SOUTH_WEST:
				case ESDASHBOARD_ANCHOR_POINT_SOUTH_EAST:
					offset-=textureHeight;
					break;
			}

			/* Set boundary in Y axis */
			textureAllocationBox.y1=outlineBox.y1+offset;
			textureAllocationBox.y2=textureAllocationBox.y1+textureHeight;

			/* Clip texture in Y axis if it does not fit into allocation */
			if(textureAllocationBox.y1<outlineBox.y1)
			{
				oversize=outlineBox.y1-textureAllocationBox.y1;
				textureCoordBox.y1=oversize/textureHeight;
				textureAllocationBox.y1=outlineBox.y1;
			}

			if(textureAllocationBox.y2>outlineBox.y2)
			{
				oversize=textureAllocationBox.y2-outlineBox.y2;
				textureCoordBox.y2=1.0f-(oversize/textureHeight);
				textureAllocationBox.y2=outlineBox.y2;
			}
		}
	}

	/* Set up paint nodes for texture */
	node=clutter_texture_node_new(priv->texture, &color, minFilter, magFilter);
	clutter_paint_node_set_name(node, G_OBJECT_TYPE_NAME(self));
	clutter_paint_node_add_texture_rectangle(node,
												&textureAllocationBox,
												textureCoordBox.x1,
												textureCoordBox.y1,
												textureCoordBox.x2,
												textureCoordBox.y2);
	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);

	/* Draw outline (color is depending is texture is fallback or not.
	 * That should be done last to get outline always visible
	 */
	if(priv->isFallback || priv->outlineColor==NULL)
	{
		outlineColor.red=0xff;
		outlineColor.green=0xff;
		outlineColor.blue=0xff;
		outlineColor.alpha=opacity;
	}
		else
		{
			outlineColor.red=priv->outlineColor->red;
			outlineColor.green=priv->outlineColor->green;
			outlineColor.blue=priv->outlineColor->blue;
			outlineColor.alpha=opacity;
		}

	node=clutter_color_node_new(&outlineColor);
	clutter_paint_node_set_name(node, "outline-top");
	clutter_actor_box_init_rect(&outlinePath, outlineBox.x1, 0.0f, outlineBox.x2-outlineBox.x1, priv->outlineWidth);
	clutter_paint_node_add_rectangle(node, &outlinePath);
	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);

	node=clutter_color_node_new(&outlineColor);
	clutter_paint_node_set_name(node, "outline-bottom");
	clutter_actor_box_init_rect(&outlinePath, outlineBox.x1, outlineBox.y2-priv->outlineWidth, outlineBox.x2-outlineBox.x1, priv->outlineWidth);
	clutter_paint_node_add_rectangle(node, &outlinePath);
	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);

	node=clutter_color_node_new(&outlineColor);
	clutter_paint_node_set_name(node, "outline-left");
	clutter_actor_box_init_rect(&outlinePath, outlineBox.x1, outlineBox.y1, priv->outlineWidth, outlineBox.y2-outlineBox.y1);
	clutter_paint_node_add_rectangle(node, &outlinePath);
	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);

	node=clutter_color_node_new(&outlineColor);
	clutter_paint_node_set_name(node, "outline-right");
	clutter_actor_box_init_rect(&outlinePath, outlineBox.x2-priv->outlineWidth, outlineBox.y1, priv->outlineWidth, outlineBox.y2-outlineBox.y1);
	clutter_paint_node_add_rectangle(node, &outlinePath);
	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);
}

/* Get preferred size of texture */
static gboolean _xdashboard_window_content_clutter_content_iface_get_preferred_size(ClutterContent *inContent,
																					gfloat *outWidth,
																					gfloat *outHeight)
{
	EsdashboardWindowContentX11Private		*priv=ESDASHBOARD_WINDOW_CONTENT_X11(inContent)->priv;
	gfloat									w, h;

	/* No texture - no size to retrieve */
	if(priv->texture==NULL) return(FALSE);

	/* If window is suspended or if we use the fallback image
	 * get real window size ...
	 */
	if(priv->isFallback || priv->isSuspended)
	{
		/* Is a fallback texture so get real window size */
		gint							windowW, windowH;

		esdashboard_window_tracker_window_get_geometry(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window), NULL, NULL, &windowW, &windowH);
		w=windowW;
		h=windowH;
	}
		else
		{
			/* ... otherwise get size of texture */
			w=cogl_texture_get_width(priv->texture);
			h=cogl_texture_get_height(priv->texture);
		}

	/* Set result values */
	if(outWidth) *outWidth=w;
	if(outHeight) *outHeight=h;

	return(TRUE);
}

/* Initialize interface of type ClutterContent */
static void _esdashboard_window_content_clutter_content_iface_init(ClutterContentIface *iface)
{
	iface->get_preferred_size=_xdashboard_window_content_clutter_content_iface_get_preferred_size;
	iface->paint_content=_xdashboard_window_content_clutter_content_iface_paint_content;
}

/* IMPLEMENTATION: Interface EsdashboardStylable */

/* Get stylable properties of stage */
static void _esdashboard_window_content_x11_stylable_get_stylable_properties(EsdashboardStylable *self,
																			GHashTable *ioStylableProperties)
{
	g_return_if_fail(ESDASHBOARD_IS_STYLABLE(self));

	/* Add stylable properties to hashtable */
	esdashboard_stylable_add_stylable_property(self, ioStylableProperties, "include-window-frame");
	esdashboard_stylable_add_stylable_property(self, ioStylableProperties, "unmapped-window-icon-x-fill");
	esdashboard_stylable_add_stylable_property(self, ioStylableProperties, "unmapped-window-icon-y-fill");
	esdashboard_stylable_add_stylable_property(self, ioStylableProperties, "unmapped-window-icon-x-align");
	esdashboard_stylable_add_stylable_property(self, ioStylableProperties, "unmapped-window-icon-y-align");
	esdashboard_stylable_add_stylable_property(self, ioStylableProperties, "unmapped-window-icon-x-scale");
	esdashboard_stylable_add_stylable_property(self, ioStylableProperties, "unmapped-window-icon-y-scale");
	esdashboard_stylable_add_stylable_property(self, ioStylableProperties, "unmapped-window-icon-anchor-point");
}

/* Get/set style classes of stage */
static const gchar* _esdashboard_window_content_x11_stylable_get_classes(EsdashboardStylable *inStylable)
{
	/* Not implemented */
	return(NULL);
}

static void _esdashboard_window_content_x11_stylable_set_classes(EsdashboardStylable *inStylable, const gchar *inStyleClasses)
{
	/* Not implemented */
}

/* Get/set style pseudo-classes of stage */
static const gchar* _esdashboard_window_content_x11_stylable_get_pseudo_classes(EsdashboardStylable *inStylable)
{
	/* Not implemented */
	return(NULL);
}

static void _esdashboard_window_content_x11_stylable_set_pseudo_classes(EsdashboardStylable *inStylable, const gchar *inStylePseudoClasses)
{
	/* Not implemented */
}

/* Interface initialization
 * Set up default functions
 */
void _esdashboard_window_content_x11_stylable_iface_init(EsdashboardStylableInterface *iface)
{
	iface->get_stylable_properties=_esdashboard_window_content_x11_stylable_get_stylable_properties;
	iface->get_classes=_esdashboard_window_content_x11_stylable_get_classes;
	iface->set_classes=_esdashboard_window_content_x11_stylable_set_classes;
	iface->get_pseudo_classes=_esdashboard_window_content_x11_stylable_get_pseudo_classes;
	iface->set_pseudo_classes=_esdashboard_window_content_x11_stylable_set_pseudo_classes;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_window_content_x11_dispose(GObject *inObject)
{
	EsdashboardWindowContentX11				*self=ESDASHBOARD_WINDOW_CONTENT_X11(inObject);
	EsdashboardWindowContentX11Private		*priv=self->priv;

	/* Dispose allocated resources */
#ifdef CLUTTER_WINDOWING_X11
	if(clutter_check_windowing_backend(CLUTTER_WINDOWING_X11))
	{
		clutter_x11_remove_filter(_esdashboard_window_content_x11_on_x_event, self);
	}
#endif

#ifdef CLUTTER_WINDOWING_GDK
	if(clutter_check_windowing_backend(CLUTTER_WINDOWING_GDK))
	{
		gdk_window_remove_filter(NULL, _esdashboard_window_content_x11_on_gdkx_event, self);
	}
#endif

	_esdashboard_window_content_x11_release_resources(self);

	if(priv->workaroundStateSignalID)
	{
		g_signal_handler_disconnect(priv->windowTracker, priv->workaroundStateSignalID);
		priv->workaroundStateSignalID=0;

		/* This signal was still connected to the window tracker so the window may be unminized
		 * and need to ensure it is minimized again. We need to do this now, before we release
		 * our handle to the window (priv->window).
		 */
		esdashboard_window_tracker_window_hide(ESDASHBOARD_WINDOW_TRACKER_WINDOW(priv->window));
	}

	if(priv->windowTracker)
	{
		g_signal_handlers_disconnect_by_data(priv->windowTracker, self);
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->window)
	{
		/* Disconnect signal handler to get notified when window is closed */
		if(priv->windowClosedSignalID)
		{
			g_signal_handler_disconnect(priv->window, priv->windowClosedSignalID);
			priv->windowClosedSignalID=0;
		}

		/* Disconnect signals */
		g_signal_handlers_disconnect_by_data(priv->window, self);

		/* libwnck resources should never be freed. Just set to NULL */
		priv->window=NULL;
	}

	if(priv->suspendSignalID)
	{
		g_signal_handler_disconnect(esdashboard_application_get_default(), priv->suspendSignalID);
		priv->suspendSignalID=0;
	}

	if(priv->outlineColor)
	{
		clutter_color_free(priv->outlineColor);
		priv->outlineColor=NULL;
	}

	if(priv->styleClasses)
	{
		g_free(priv->styleClasses);
		priv->styleClasses=NULL;
	}

	if(priv->stylePseudoClasses)
	{
		g_free(priv->stylePseudoClasses);
		priv->stylePseudoClasses=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_window_content_x11_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_window_content_x11_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	EsdashboardWindowContentX11		*self=ESDASHBOARD_WINDOW_CONTENT_X11(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW:
			_esdashboard_window_content_x11_set_window(self, ESDASHBOARD_WINDOW_TRACKER_WINDOW_X11(g_value_get_object(inValue)));
			break;

		case PROP_OUTLINE_COLOR:
			esdashboard_window_content_x11_set_outline_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_OUTLINE_WIDTH:
			esdashboard_window_content_x11_set_outline_width(self, g_value_get_float(inValue));
			break;

		case PROP_INCLUDE_WINDOW_FRAME:
			esdashboard_window_content_x11_set_include_window_frame(self, g_value_get_boolean(inValue));
			break;

		case PROP_UNMAPPED_WINDOW_ICON_X_FILL:
			esdashboard_window_content_x11_set_unmapped_window_icon_x_fill(self, g_value_get_boolean(inValue));
			break;

		case PROP_UNMAPPED_WINDOW_ICON_Y_FILL:
			esdashboard_window_content_x11_set_unmapped_window_icon_y_fill(self, g_value_get_boolean(inValue));
			break;

		case PROP_UNMAPPED_WINDOW_ICON_X_ALIGN:
			esdashboard_window_content_x11_set_unmapped_window_icon_x_align(self, g_value_get_float(inValue));
			break;

		case PROP_UNMAPPED_WINDOW_ICON_Y_ALIGN:
			esdashboard_window_content_x11_set_unmapped_window_icon_y_align(self, g_value_get_float(inValue));
			break;

		case PROP_UNMAPPED_WINDOW_ICON_X_SCALE:
			esdashboard_window_content_x11_set_unmapped_window_icon_x_scale(self, g_value_get_float(inValue));
			break;

		case PROP_UNMAPPED_WINDOW_ICON_Y_SCALE:
			esdashboard_window_content_x11_set_unmapped_window_icon_y_scale(self, g_value_get_float(inValue));
			break;

		case PROP_UNMAPPED_WINDOW_ICON_ANCHOR_POINT:
			esdashboard_window_content_x11_set_unmapped_window_icon_anchor_point(self, g_value_get_enum(inValue));
			break;

		case PROP_STYLE_CLASSES:
			_esdashboard_window_content_x11_stylable_set_classes(ESDASHBOARD_STYLABLE(self), g_value_get_string(inValue));
			break;

		case PROP_STYLE_PSEUDO_CLASSES:
			_esdashboard_window_content_x11_stylable_set_pseudo_classes(ESDASHBOARD_STYLABLE(self), g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_window_content_x11_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	EsdashboardWindowContentX11				*self=ESDASHBOARD_WINDOW_CONTENT_X11(inObject);
	EsdashboardWindowContentX11Private		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_WINDOW:
			g_value_set_object(outValue, priv->window);
			break;

		case PROP_SUSPENDED:
			g_value_set_boolean(outValue, priv->isSuspended);
			break;

		case PROP_OUTLINE_COLOR:
			clutter_value_set_color(outValue, priv->outlineColor);
			break;

		case PROP_OUTLINE_WIDTH:
			g_value_set_float(outValue, priv->outlineWidth);
			break;

		case PROP_INCLUDE_WINDOW_FRAME:
			g_value_set_boolean(outValue, priv->includeWindowFrame);
			break;

		case PROP_UNMAPPED_WINDOW_ICON_X_FILL:
			g_value_set_boolean(outValue, priv->unmappedWindowIconXFill);
			break;

		case PROP_UNMAPPED_WINDOW_ICON_Y_FILL:
			g_value_set_boolean(outValue, priv->unmappedWindowIconYFill);
			break;

		case PROP_UNMAPPED_WINDOW_ICON_X_ALIGN:
			g_value_set_float(outValue, priv->unmappedWindowIconXAlign);
			break;

		case PROP_UNMAPPED_WINDOW_ICON_Y_ALIGN:
			g_value_set_float(outValue, priv->unmappedWindowIconYAlign);
			break;

		case PROP_UNMAPPED_WINDOW_ICON_X_SCALE:
			g_value_set_float(outValue, priv->unmappedWindowIconXScale);
			break;

		case PROP_UNMAPPED_WINDOW_ICON_Y_SCALE:
			g_value_set_float(outValue, priv->unmappedWindowIconYScale);
			break;

		case PROP_UNMAPPED_WINDOW_ICON_ANCHOR_POINT:
			g_value_set_enum(outValue, priv->unmappedWindowIconAnchorPoint);
			break;

		case PROP_STYLE_CLASSES:
			g_value_set_string(outValue, priv->styleClasses);
			break;

		case PROP_STYLE_PSEUDO_CLASSES:
			g_value_set_string(outValue, priv->stylePseudoClasses);
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
void esdashboard_window_content_x11_class_init(EsdashboardWindowContentX11Class *klass)
{
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);
	EsdashboardStylableInterface	*stylableIface;
	GParamSpec						*paramSpec;

	/* Override functions */
	gobjectClass->dispose=_esdashboard_window_content_x11_dispose;
	gobjectClass->set_property=_esdashboard_window_content_x11_set_property;
	gobjectClass->get_property=_esdashboard_window_content_x11_get_property;

	stylableIface=g_type_default_interface_ref(ESDASHBOARD_TYPE_STYLABLE);

	/* Define properties */
	EsdashboardWindowContentX11Properties[PROP_WINDOW]=
		g_param_spec_object("window",
							"Window",
							"The window to handle and display",
							ESDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	EsdashboardWindowContentX11Properties[PROP_SUSPENDED]=
		g_param_spec_boolean("suspended",
							"Suspended",
							"Is this window suspended",
							TRUE,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_OUTLINE_COLOR]=
		clutter_param_spec_color("outline-color",
									"Outline color",
									"Color to draw outline of mapped windows with",
									CLUTTER_COLOR_Black,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_OUTLINE_WIDTH]=
		g_param_spec_float("outline-width",
							"Outline width",
							"Width of line used to draw outline of mapped windows",
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_INCLUDE_WINDOW_FRAME]=
		g_param_spec_boolean("include-window-frame",
							"Include window frame",
							"Whether the window frame should be included or only the window content should be shown",
							FALSE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_X_FILL]=
		g_param_spec_boolean("unmapped-window-icon-x-fill",
							"Unmapped window icon X fill",
							"Whether the unmapped window icon should fill up horizontal space",
							TRUE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_Y_FILL]=
		g_param_spec_boolean("unmapped-window-icon-y-fill",
							"Unmapped window icon y fill",
							"Whether the unmapped window icon should fill up vertical space",
							TRUE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_X_ALIGN]=
		g_param_spec_float("unmapped-window-icon-x-align",
							"Unmapped window icon X align",
							"The alignment of the unmapped window icon on the X axis within the allocation in normalized coordinate between 0 and 1",
							0.0f, 1.0f,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_Y_ALIGN]=
		g_param_spec_float("unmapped-window-icon-y-align",
							"Unmapped window icon Y align",
							"The alignment of the unmapped window icon on the Y axis within the allocation in normalized coordinate between 0 and 1",
							0.0f, 1.0f,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_X_SCALE]=
		g_param_spec_float("unmapped-window-icon-x-scale",
							"Unmapped window icon X scale",
							"Scale factor of unmapped window icon on the X axis",
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_Y_SCALE]=
		g_param_spec_float("unmapped-window-icon-y-scale",
							"Unmapped window icon Y scale",
							"Scale factor of unmapped window icon on the Y axis",
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_ANCHOR_POINT]=
		g_param_spec_enum("unmapped-window-icon-anchor-point",
							"Unmapped window icon anchor point",
							"The anchor point of unmapped window icon",
							ESDASHBOARD_TYPE_ANCHOR_POINT,
							ESDASHBOARD_ANCHOR_POINT_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	paramSpec=g_object_interface_find_property(stylableIface, "style-classes");
	EsdashboardWindowContentX11Properties[PROP_STYLE_CLASSES]=
		g_param_spec_override("style-classes", paramSpec);

	paramSpec=g_object_interface_find_property(stylableIface, "style-pseudo-classes");
	EsdashboardWindowContentX11Properties[PROP_STYLE_PSEUDO_CLASSES]=
		g_param_spec_override("style-pseudo-classes", paramSpec);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardWindowContentX11Properties);

	/* Release allocated resources */
	g_type_default_interface_unref(stylableIface);
}

/* Object initialization
 * Create private structure and set up default values
 */
void esdashboard_window_content_x11_init(EsdashboardWindowContentX11 *self)
{
	EsdashboardWindowContentX11Private		*priv;
	EsdashboardApplication				*app;

	priv=self->priv=esdashboard_window_content_x11_get_instance_private(self);

	/* Set default values */
	priv->window=NULL;
	priv->texture=NULL;
	priv->xWindowID=None;
	priv->pixmap=None;
#ifdef HAVE_XDAMAGE
	priv->damage=None;
#endif
	priv->isFallback=FALSE;
	priv->outlineColor=clutter_color_copy(CLUTTER_COLOR_Black);
	priv->outlineWidth=1.0f;
	priv->isSuspended=TRUE;
	priv->suspendSignalID=0;
	priv->isMapped=FALSE;
	priv->includeWindowFrame=FALSE;
	priv->styleClasses=NULL;
	priv->stylePseudoClasses=NULL;
	priv->windowTracker=esdashboard_window_tracker_get_default();
	priv->workaroundMode=ESDASHBOARD_WINDOW_CONTENT_X11_WORKAROUND_MODE_NONE;
	priv->workaroundStateSignalID=0;
	priv->unmappedWindowIconXFill=FALSE;
	priv->unmappedWindowIconYFill=FALSE;
	priv->unmappedWindowIconXAlign=0.0f;
	priv->unmappedWindowIconYAlign=0.0f;
	priv->unmappedWindowIconXScale=1.0f;
	priv->unmappedWindowIconYScale=1.0f;
	priv->unmappedWindowIconAnchorPoint=ESDASHBOARD_ANCHOR_POINT_NONE;
	priv->suspendAfterResumeOnIdle=FALSE;
	priv->windowClosedSignalID=0;

	/* Check extensions (will only be done once) */
	_esdashboard_window_content_x11_check_extension();

	/* Add event filter for this instance */
#ifdef CLUTTER_WINDOWING_X11
	if(clutter_check_windowing_backend(CLUTTER_WINDOWING_X11))
	{
		clutter_x11_add_filter(_esdashboard_window_content_x11_on_x_event, self);
	}
#endif

#ifdef CLUTTER_WINDOWING_GDK
	if(clutter_check_windowing_backend(CLUTTER_WINDOWING_GDK))
	{
		gdk_window_add_filter(NULL, _esdashboard_window_content_x11_on_gdkx_event, self);
	}
#endif

	/* Style content */
	esdashboard_stylable_invalidate(ESDASHBOARD_STYLABLE(self));

	/* Handle suspension signals from application */
	app=esdashboard_application_get_default();
	priv->suspendSignalID=g_signal_connect_swapped(app,
													"notify::is-suspended",
													G_CALLBACK(_esdashboard_window_content_x11_on_application_suspended_changed),
													self);
	priv->isAppSuspended=esdashboard_application_is_suspended(app);

	/* Register global signal handler for esconf value change notification
	 * if not done already.
	 */
	if(!_esdashboard_window_content_x11_esconf_priority_notify_id)
	{
		EsconfChannel					*esconfChannel;
		gchar							*detailedSignal;

		/* Connect to property changed signal in esconf */
		esconfChannel=esdashboard_application_get_esconf_channel(NULL);
		detailedSignal=g_strconcat("property-changed::", WINDOW_CONTENT_CREATION_PRIORITY_ESCONF_PROP, NULL);
		_esdashboard_window_content_x11_esconf_priority_notify_id=g_signal_connect(esconfChannel,
																				detailedSignal,
																				G_CALLBACK(_esdashboard_window_content_x11_on_window_creation_priority_value_changed),
																				NULL);
		if(detailedSignal) g_free(detailedSignal);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Connected to property changed signal with handler ID %u for esconf value change notifications",
							_esdashboard_window_content_x11_esconf_priority_notify_id);

		/* Connect to application shutdown signal for esconf value change notification */
		_esdashboard_window_content_x11_window_creation_shutdown_signal_id=g_signal_connect(app,
																				"shutdown-final",
																				G_CALLBACK(_esdashboard_window_content_x11_on_window_creation_priority_shutdown),
																				NULL);
		ESDASHBOARD_DEBUG(self, WINDOWS,
							"Connected to shutdown signal with handler ID %u for esconf value change notifications",
							_esdashboard_window_content_x11_window_creation_shutdown_signal_id);
	}
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterContent* esdashboard_window_content_x11_new_for_window(EsdashboardWindowTrackerWindowX11 *inWindow)
{
	ClutterContent		*content;

	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	/* Create window content */
	content=CLUTTER_CONTENT(g_object_new(ESDASHBOARD_TYPE_WINDOW_CONTENT_X11,
											"window", inWindow,
											NULL));

	return(content);
}

/* Get window to handle and to display */
EsdashboardWindowTrackerWindow* esdashboard_window_content_x11_get_window(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), NULL);

	return(ESDASHBOARD_WINDOW_TRACKER_WINDOW(self->priv->window));
}

/* Get state of suspension */
gboolean esdashboard_window_content_x11_is_suspended(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), TRUE);

	return(self->priv->isSuspended);
}

/* Get/set color to draw outline with */
const ClutterColor* esdashboard_window_content_x11_get_outline_color(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), NULL);

	return(self->priv->outlineColor);
}

void esdashboard_window_content_x11_set_outline_color(EsdashboardWindowContentX11 *self, const ClutterColor *inColor)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->outlineColor==NULL || clutter_color_equal(inColor, priv->outlineColor)==FALSE)
	{
		/* Set value */
		if(priv->outlineColor) clutter_color_free(priv->outlineColor);
		priv->outlineColor=clutter_color_copy(inColor);

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_OUTLINE_COLOR]);
	}
}

/* Get/set line width for outline */
gfloat esdashboard_window_content_x11_get_outline_width(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), 0.0f);

	return(self->priv->outlineWidth);
}

void esdashboard_window_content_x11_set_outline_width(EsdashboardWindowContentX11 *self, const gfloat inWidth)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inWidth>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->outlineWidth!=inWidth)
	{
		/* Set value */
		priv->outlineWidth=inWidth;

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_OUTLINE_WIDTH]);
	}
}

/* Get/set flag to indicate whether to include the window frame or not */
gboolean esdashboard_window_content_x11_get_include_window_frame(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), TRUE);

	return(self->priv->includeWindowFrame);
}

void esdashboard_window_content_x11_set_include_window_frame(EsdashboardWindowContentX11 *self, const gboolean inIncludeFrame)
{
	EsdashboardWindowContentX11Private				*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->includeWindowFrame!=inIncludeFrame)
	{
		/* Set value */
		priv->includeWindowFrame=inIncludeFrame;

		/* (Re-)Setup window content */
		if(priv->window)
		{
			EsdashboardWindowTrackerWindowX11		*window;

			/* Re-setup window by releasing all resources first and unsetting window
			 * but remember window to set it again.
			 */
			_esdashboard_window_content_x11_release_resources(self);

			/* libwnck resources should never be freed. Just set to NULL */
			window=priv->window;
			priv->window=NULL;

			/* Now set same window again which causes this object to set up all
			 * needed X resources again.
			 */
			_esdashboard_window_content_x11_set_window(self, window);
		}

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_INCLUDE_WINDOW_FRAME]);
	}
}

/* Get/set x fill of unmapped window icon */
gboolean esdashboard_window_content_x11_get_unmapped_window_icon_x_fill(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), FALSE);

	return(self->priv->unmappedWindowIconXFill);
}

void esdashboard_window_content_x11_set_unmapped_window_icon_x_fill(EsdashboardWindowContentX11 *self, const gboolean inFill)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->unmappedWindowIconXFill!=inFill)
	{
		/* Set value */
		priv->unmappedWindowIconXFill=inFill;

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_X_FILL]);
	}
}

/* Get/set y fill of unmapped window icon */
gboolean esdashboard_window_content_x11_get_unmapped_window_icon_y_fill(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), FALSE);

	return(self->priv->unmappedWindowIconYFill);
}

void esdashboard_window_content_x11_set_unmapped_window_icon_y_fill(EsdashboardWindowContentX11 *self, const gboolean inFill)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->unmappedWindowIconYFill!=inFill)
	{
		/* Set value */
		priv->unmappedWindowIconYFill=inFill;

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_Y_FILL]);
	}
}

/* Get/set x align of unmapped window icon */
gfloat esdashboard_window_content_x11_get_unmapped_window_icon_x_align(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), 0.0f);

	return(self->priv->unmappedWindowIconXAlign);
}

void esdashboard_window_content_x11_set_unmapped_window_icon_x_align(EsdashboardWindowContentX11 *self, const gfloat inAlign)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inAlign>=0.0f && inAlign<=1.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->unmappedWindowIconXAlign!=inAlign)
	{
		/* Set value */
		priv->unmappedWindowIconXAlign=inAlign;

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_X_ALIGN]);
	}
}

/* Get/set y align of unmapped window icon */
gfloat esdashboard_window_content_x11_get_unmapped_window_icon_y_align(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), 0.0f);

	return(self->priv->unmappedWindowIconYAlign);
}

void esdashboard_window_content_x11_set_unmapped_window_icon_y_align(EsdashboardWindowContentX11 *self, const gfloat inAlign)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inAlign>=0.0f && inAlign<=1.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->unmappedWindowIconYAlign!=inAlign)
	{
		/* Set value */
		priv->unmappedWindowIconYAlign=inAlign;

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_Y_ALIGN]);
	}
}

/* Get/set x scale of unmapped window icon */
gfloat esdashboard_window_content_x11_get_unmapped_window_icon_x_scale(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), 0.0f);

	return(self->priv->unmappedWindowIconXScale);
}

void esdashboard_window_content_x11_set_unmapped_window_icon_x_scale(EsdashboardWindowContentX11 *self, const gfloat inScale)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inScale>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->unmappedWindowIconXScale!=inScale)
	{
		/* Set value */
		priv->unmappedWindowIconXScale=inScale;

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_X_SCALE]);
	}
}

/* Get/set y scale of unmapped window icon */
gfloat esdashboard_window_content_x11_get_unmapped_window_icon_y_scale(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), 0.0f);

	return(self->priv->unmappedWindowIconYScale);
}

void esdashboard_window_content_x11_set_unmapped_window_icon_y_scale(EsdashboardWindowContentX11 *self, const gfloat inScale)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inScale>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->unmappedWindowIconYScale!=inScale)
	{
		/* Set value */
		priv->unmappedWindowIconYScale=inScale;

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_Y_SCALE]);
	}
}

/* Get/set gravity (anchor point) of unmapped window icon */
EsdashboardAnchorPoint esdashboard_window_content_x11_get_unmapped_window_icon_anchor_point(EsdashboardWindowContentX11 *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self), ESDASHBOARD_ANCHOR_POINT_NONE);

	return(self->priv->unmappedWindowIconAnchorPoint);
}

void esdashboard_window_content_x11_set_unmapped_window_icon_anchor_point(EsdashboardWindowContentX11 *self, const EsdashboardAnchorPoint inAnchorPoint)
{
	EsdashboardWindowContentX11Private		*priv;

	g_return_if_fail(ESDASHBOARD_IS_WINDOW_CONTENT_X11(self));
	g_return_if_fail(inAnchorPoint>=ESDASHBOARD_ANCHOR_POINT_NONE);
	g_return_if_fail(inAnchorPoint<=ESDASHBOARD_ANCHOR_POINT_CENTER);

	priv=self->priv;

	/* Set value if changed */
	if(priv->unmappedWindowIconAnchorPoint!=inAnchorPoint)
	{
		/* Set value */
		priv->unmappedWindowIconAnchorPoint=inAnchorPoint;

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardWindowContentX11Properties[PROP_UNMAPPED_WINDOW_ICON_ANCHOR_POINT]);
	}
}
