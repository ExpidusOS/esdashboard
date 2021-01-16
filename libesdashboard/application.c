/*
 * application: Single-instance managing application and single-instance
 *              objects like window manager and so on.
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

/**
 * SECTION:application
 * @short_description: The core application class
 * @include: esdashboard/application.h
 *
 * #EsdashboardApplication is a single instance object. Its main purpose
 * is to setup and start-up the application and also to manage other
 * (mainly single instance) objects.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/application.h>

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtk.h>
#include <garcon/garcon.h>
#include <libexpidus1ui/libexpidus1ui.h>

#include <libesdashboard/stage.h>
#include <libesdashboard/types.h>
#include <libesdashboard/view-manager.h>
#include <libesdashboard/applications-view.h>
#include <libesdashboard/windows-view.h>
#include <libesdashboard/search-view.h>
#include <libesdashboard/search-manager.h>
#include <libesdashboard/applications-search-provider.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/theme.h>
#include <libesdashboard/focus-manager.h>
#include <libesdashboard/bindings-pool.h>
#include <libesdashboard/application-database.h>
#include <libesdashboard/application-tracker.h>
#include <libesdashboard/plugins-manager.h>
#include <libesdashboard/window-tracker-backend.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
struct _EsdashboardApplicationPrivate
{
	/* Properties related */
	gboolean							isDaemon;
	gboolean							isSuspended;
	gchar								*themeName;

	/* Instance related */
	gboolean							initialized;
	gboolean							isQuitting;
	gboolean							forcedNewInstance;

	EsconfChannel						*esconfChannel;
	EsdashboardStage					*stage;
	EsdashboardViewManager				*viewManager;
	EsdashboardSearchManager			*searchManager;
	EsdashboardFocusManager				*focusManager;

	EsdashboardTheme					*theme;
	gulong								esconfThemeChangedSignalID;

	EsdashboardBindingsPool				*bindings;

	EsdashboardApplicationDatabase		*appDatabase;
	EsdashboardApplicationTracker		*appTracker;

	ExpidusSMClient						*sessionManagementClient;

	EsdashboardPluginsManager			*pluginManager;

	EsdashboardWindowTrackerBackend		*windowTrackerBackend;
};

G_DEFINE_TYPE_WITH_PRIVATE(EsdashboardApplication,
							esdashboard_application,
							G_TYPE_APPLICATION)

/* Properties */
enum
{
	PROP_0,

	PROP_DAEMONIZED,
	PROP_SUSPENDED,
	PROP_THEME_NAME,
	PROP_STAGE,

	PROP_LAST
};

static GParamSpec* EsdashboardApplicationProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_INITIALIZED,
	SIGNAL_QUIT,
	SIGNAL_SHUTDOWN_FINAL,

	SIGNAL_SUSPEND,
	SIGNAL_RESUME,

	SIGNAL_THEME_LOADING,
	SIGNAL_THEME_LOADED,
	SIGNAL_THEME_CHANGED,

	SIGNAL_APPLICATION_LAUNCHED,

	/* Actions */
	ACTION_EXIT,

	SIGNAL_LAST
};

static guint EsdashboardApplicationSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define ESDASHBOARD_APP_ID					"de.froevel.nomad.esdashboard"
#define ESDASHBOARD_ESCONF_CHANNEL			"esdashboard"

#define THEME_NAME_ESCONF_PROP				"/theme"
#define DEFAULT_THEME_NAME					"esdashboard"

/* Single instance of application */
static EsdashboardApplication*		_esdashboard_application=NULL;

/* Forward declarations */
static void _esdashboard_application_activate(GApplication *inApplication);

/* Quit application depending on daemon mode and force parameter */
static void _esdashboard_application_quit(EsdashboardApplication *self, gboolean inForceQuit)
{
	EsdashboardApplicationPrivate	*priv;
	gboolean						shouldQuit;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(self));

	priv=self->priv;
	shouldQuit=FALSE;

	/* Check if we should really quit this instance */
	if(inForceQuit==TRUE || priv->isDaemon==FALSE) shouldQuit=TRUE;

	/* Do nothing if application is already quitting. This can happen if
	 * application is running in daemon mode (primary instance) and another
	 * instance was called with "quit" or "restart" parameter which would
	 * cause this function to be called twice.
	 */
	if(priv->isQuitting) return;

	/* If application is not in daemon mode or if forced is set to TRUE
	 * destroy all stage windows ...
	 */
	if(shouldQuit==TRUE)
	{
		/* Set flag that application is going to quit */
		priv->isQuitting=TRUE;

		/* If application is told to quit, set the restart style to something
		 * when it won't restart itself.
		 */
		if(priv->sessionManagementClient &&
			EXPIDUS_IS_SM_CLIENT(priv->sessionManagementClient))
		{
			expidus_sm_client_set_restart_style(priv->sessionManagementClient, EXPIDUS_SM_CLIENT_RESTART_NORMAL);
		}

		/* Emit "quit" signal */
		g_signal_emit(self, EsdashboardApplicationSignals[SIGNAL_QUIT], 0);

		/* Destroy stage */
		if(priv->stage)
		{
			clutter_actor_destroy(CLUTTER_ACTOR(priv->stage));
			priv->stage=NULL;
		}

		/* Really quit application here and now */
		if(priv->initialized)
		{
			/* Release extra reference on application which causes g_application_run()
			 * to exit when returning.
			 */
			g_application_release(G_APPLICATION(self));
		}
	}
		/* ... otherwise emit "suspend" signal */
		else
		{
			/* Only send signal if not suspended already */
			if(!priv->isSuspended)
			{
				/* Send signal */
				g_signal_emit(self, EsdashboardApplicationSignals[SIGNAL_SUSPEND], 0);

				/* Set flag for suspension */
				priv->isSuspended=TRUE;
				g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationProperties[PROP_SUSPENDED]);
			}
		}
}

/* Action "exit" was called at application */
static gboolean _esdashboard_application_action_exit(EsdashboardApplication *self,
														EsdashboardFocusable *inSource,
														const gchar *inAction,
														ClutterEvent *inEvent)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATION(self), CLUTTER_EVENT_PROPAGATE);

	/* Quit application */
	_esdashboard_application_quit(self, FALSE);

	/* Prevent the default handler being called */
	return(CLUTTER_EVENT_STOP);
}

/* The session is going to quit */
static void _esdashboard_application_on_session_quit(EsdashboardApplication *self,
														gpointer inUserData)
{
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(self));
	g_return_if_fail(EXPIDUS_IS_SM_CLIENT(inUserData));

	/* Force application to quit */
	ESDASHBOARD_DEBUG(self, MISC, "Received 'quit' from session management client - initiating shutdown");
	_esdashboard_application_quit(self, TRUE);
}

/* A stage window should be destroyed */
static gboolean _esdashboard_application_on_delete_stage(EsdashboardApplication *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATION(self), CLUTTER_EVENT_PROPAGATE);

	/* Quit application */
	_esdashboard_application_quit(self, FALSE);

	/* Prevent the default handler being called */
	return(CLUTTER_EVENT_STOP);
}

/* Set theme name and reload theme */
static void _esdashboard_application_set_theme_name(EsdashboardApplication *self, const gchar *inThemeName)
{
	EsdashboardApplicationPrivate	*priv;
	GError							*error;
	EsdashboardTheme				*theme;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(self));
	g_return_if_fail(inThemeName && *inThemeName);

	priv=self->priv;
	error=NULL;

	/* Set value if changed */
	if(g_strcmp0(priv->themeName, inThemeName)!=0)
	{
		/* Create new theme instance */
		theme=esdashboard_theme_new(inThemeName);

		/* Emit signal that theme is going to be loaded */
		g_signal_emit(self, EsdashboardApplicationSignals[SIGNAL_THEME_LOADING], 0, theme);

		/* Load theme */
		if(!esdashboard_theme_load(theme, &error))
		{
			/* Show critical warning at console */
			g_critical("Could not load theme '%s': %s",
						inThemeName,
						(error && error->message) ? error->message : "unknown error");

			/* Show warning as notification */
			esdashboard_notify(NULL,
								"dialog-error",
								_("Could not load theme '%s': %s"),
								inThemeName,
								(error && error->message) ? error->message : _("unknown error"));

			/* Release allocated resources */
			if(error!=NULL) g_error_free(error);
			g_object_unref(theme);

			return;
		}

		/* Emit signal that theme was loaded successfully and will soon be applied */
		g_signal_emit(self, EsdashboardApplicationSignals[SIGNAL_THEME_LOADED], 0, theme);

		/* Set value */
		if(priv->themeName) g_free(priv->themeName);
		priv->themeName=g_strdup(inThemeName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationProperties[PROP_THEME_NAME]);

		/* Release current theme and store new one */
		if(priv->theme) g_object_unref(priv->theme);
		priv->theme=theme;

		/* Emit signal that theme has changed to get all top-level actors
		 * to apply new theme.
		 */
		g_signal_emit(self, EsdashboardApplicationSignals[SIGNAL_THEME_CHANGED], 0, priv->theme);
	}
}

/* Perform full initialization of this application instance */
static gboolean _esdashboard_application_initialize_full(EsdashboardApplication *self)
{
	EsdashboardApplicationPrivate	*priv;
	GError							*error;
#if !GARCON_CHECK_VERSION(0,3,0)
	const gchar						*desktop;
#endif
	ExpidusSMClientRestartStyle		sessionManagementRestartStyle;

	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATION(self), FALSE);

	priv=self->priv;
	error=NULL;

	/* Initialize garcon for current desktop environment */
#if !GARCON_CHECK_VERSION(0,3,0)
	desktop=g_getenv("XDG_CURRENT_DESKTOP");
	if(G_LIKELY(desktop==NULL))
	{
		/* If we could not determine current desktop environment
		 * assume Expidus as this application is developed for this DE.
		 */
		desktop="EXPIDUS";
	}
		/* If desktop environment was found but has no name
		 * set NULL to get all menu items shown.
		 */
		else if(*desktop==0) desktop=NULL;

	garcon_set_environment(desktop);
#else
	garcon_set_environment_xdg(GARCON_ENVIRONMENT_EXPIDUS);
#endif

	/* Setup the session management */
	sessionManagementRestartStyle=EXPIDUS_SM_CLIENT_RESTART_IMMEDIATELY;
	if(priv->forcedNewInstance) sessionManagementRestartStyle=EXPIDUS_SM_CLIENT_RESTART_NORMAL;

	priv->sessionManagementClient=expidus_sm_client_get();
	expidus_sm_client_set_priority(priv->sessionManagementClient, EXPIDUS_SM_CLIENT_PRIORITY_DEFAULT);
	expidus_sm_client_set_restart_style(priv->sessionManagementClient, sessionManagementRestartStyle);
	g_signal_connect_swapped(priv->sessionManagementClient, "quit", G_CALLBACK(_esdashboard_application_on_session_quit), self);

	if(!expidus_sm_client_connect(priv->sessionManagementClient, &error))
	{
		g_warning("Failed to connect to session manager: %s",
					(error && error->message) ? error->message : "unknown error");
		g_clear_error(&error);
	}

	/* Initialize esconf */
	if(!esconf_init(&error))
	{
		g_critical("Could not initialize esconf: %s",
					(error && error->message) ? error->message : "unknown error");
		if(error) g_error_free(error);
		return(FALSE);
	}

	priv->esconfChannel=esconf_channel_get(ESDASHBOARD_ESCONF_CHANNEL);

	/* Set up keyboard and pointer bindings */
	priv->bindings=esdashboard_bindings_pool_get_default();
	if(!priv->bindings)
	{
		g_critical("Could not initialize bindings");
		return(FALSE);
	}

	if(!esdashboard_bindings_pool_load(priv->bindings, &error))
	{
		g_critical("Could not load bindings: %s",
					(error && error->message) ? error->message : "unknown error");
		if(error!=NULL) g_error_free(error);
		return(FALSE);
	}

	/* Create single-instance of window tracker backend to keep it alive while
	 * application is running and to avoid multiple reinitializations. It must
	 * be create before any class using a window tracker.
	 */
	priv->windowTrackerBackend=esdashboard_window_tracker_backend_get_default();
	if(!priv->windowTrackerBackend)
	{
		g_critical("Could not setup window tracker backend");
		return(FALSE);
	}

	/* Set up application database */
	priv->appDatabase=esdashboard_application_database_get_default();
	if(!priv->appDatabase)
	{
		g_critical("Could not initialize application database");
		return(FALSE);
	}

	if(!esdashboard_application_database_load(priv->appDatabase, &error))
	{
		g_critical("Could not load application database: %s",
					(error && error->message) ? error->message : "unknown error");
		if(error!=NULL) g_error_free(error);
		return(FALSE);
	}

	/* Set up application tracker */
	priv->appTracker=esdashboard_application_tracker_get_default();
	if(!priv->appTracker)
	{
		g_critical("Could not initialize application tracker");
		return(FALSE);
	}

	/* Register built-in views (order of registration is important) */
	priv->viewManager=esdashboard_view_manager_get_default();

	esdashboard_view_manager_register(priv->viewManager, "builtin.windows", ESDASHBOARD_TYPE_WINDOWS_VIEW);
	esdashboard_view_manager_register(priv->viewManager, "builtin.applications", ESDASHBOARD_TYPE_APPLICATIONS_VIEW);
	esdashboard_view_manager_register(priv->viewManager, "builtin.search", ESDASHBOARD_TYPE_SEARCH_VIEW);

	/* Register built-in search providers */
	priv->searchManager=esdashboard_search_manager_get_default();

	esdashboard_search_manager_register(priv->searchManager, "builtin.applications", ESDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER);

	/* Create single-instance of focus manager to keep it alive while
	 * application is running.
	 */
	priv->focusManager=esdashboard_focus_manager_get_default();

	/* Create single-instance of plugin manager to keep it alive while
	 * application is running.
	 */
	priv->pluginManager=esdashboard_plugins_manager_get_default();
	if(!priv->pluginManager)
	{
		g_critical("Could not initialize plugin manager");
		return(FALSE);
	}

	if(!esdashboard_plugins_manager_setup(priv->pluginManager))
	{
		g_critical("Could not setup plugin manager");
		return(FALSE);
	}

	/* Set up and load theme */
	priv->esconfThemeChangedSignalID=esconf_g_property_bind(priv->esconfChannel,
															THEME_NAME_ESCONF_PROP,
															G_TYPE_STRING,
															self,
															"theme-name");
	if(!priv->esconfThemeChangedSignalID)
	{
		g_warning("Could not create binding between esconf property and local resource for theme change notification.");
	}

	/* Set up default theme in Xfcond if property in channel does not exist
	 * because it indicates first start.
	 */
	if(!esconf_channel_has_property(priv->esconfChannel, THEME_NAME_ESCONF_PROP))
	{
		esconf_channel_set_string(priv->esconfChannel,
									THEME_NAME_ESCONF_PROP,
									DEFAULT_THEME_NAME);
	}

	/* At this time the theme must have been loaded, either because we
	 * set the default theme name because of missing theme property in
	 * esconf channel or the value of esconf channel property has been read
	 * and set when setting up binding (between esconf property and local property)
	 * what caused a call to function to set theme name in this object
	 * and also caused a reload of theme.
	 * So if no theme object is set in this object then loading theme has
	 * failed and we have to return FALSE.
	 */
	if(!priv->theme) return(FALSE);

	/* Create stage containing all monitors */
	priv->stage=ESDASHBOARD_STAGE(esdashboard_stage_new());
	g_signal_connect_swapped(priv->stage, "delete-event", G_CALLBACK(_esdashboard_application_on_delete_stage), self);

	/* Emit signal 'theme-changed' to get current theme loaded at each stage created */
	g_signal_emit(self, EsdashboardApplicationSignals[SIGNAL_THEME_CHANGED], 0, priv->theme);

	/* Initialization was successful so send signal and return TRUE */
	g_signal_emit(self, EsdashboardApplicationSignals[SIGNAL_INITIALIZED], 0);

#ifdef DEBUG
	esdashboard_notify(NULL, NULL, _("Welcome to %s (%s)!"), PACKAGE_NAME, PACKAGE_VERSION);
#else
	esdashboard_notify(NULL, NULL, _("Welcome to %s!"), PACKAGE_NAME);
#endif

	return(TRUE);
}

/* Switch to requested view */
static void _esdashboard_application_switch_to_view(EsdashboardApplication *self, const gchar *inInternalViewName)
{
	EsdashboardApplicationPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(self));

	priv=self->priv;

	/* If no view name was specified then do nothing and return immediately */
	if(!inInternalViewName ||
		!inInternalViewName[0])
	{
		ESDASHBOARD_DEBUG(self, MISC, "No view to switch to specified");
		return;
	}

	/* Tell stage to switch requested view */
	ESDASHBOARD_DEBUG(self, MISC,
						"Trying to switch to view '%s'",
						inInternalViewName);
	esdashboard_stage_set_switch_to_view(priv->stage, inInternalViewName);
}

/* Handle command-line on primary instance */
static gint _esdashboard_application_handle_command_line_arguments(EsdashboardApplication *self,
																	gint inArgc,
																	gchar **inArgv)
{
	EsdashboardApplicationPrivate	*priv;
	GOptionContext					*context;
	gboolean						result;
	GError							*error;
	gboolean						optionDaemonize;
	gboolean						optionQuit;
	gboolean						optionRestart;
	gboolean						optionToggle;
	gchar							*optionSwitchToView;
	gboolean						optionVersion;
	GOptionEntry					entries[]=
									{
										{ "daemonize", 'd', 0, G_OPTION_ARG_NONE, &optionDaemonize, N_("Fork to background"), NULL },
										{ "quit", 'q', 0, G_OPTION_ARG_NONE, &optionQuit, N_("Quit running instance"), NULL },
										{ "restart", 'r', 0, G_OPTION_ARG_NONE, &optionRestart, N_("Restart running instance"), NULL },
										{ "toggle", 't', 0, G_OPTION_ARG_NONE, &optionToggle, N_("Toggles visibility if running instance was started in daemon mode otherwise it quits running non-daemon instance"), NULL },
										{ "view", 0, 0, G_OPTION_ARG_STRING, &optionSwitchToView, N_("The ID of view to switch to on startup or resume"), "ID" },
										{ "version", 'v', 0, G_OPTION_ARG_NONE, &optionVersion, N_("Show version"), NULL },
										{ NULL }
									};

	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATION(self), ESDASHBOARD_APPLICATION_ERROR_FAILED);

	priv=self->priv;
	error=NULL;

	/* Set up options */
	optionDaemonize=FALSE;
	optionQuit=FALSE;
	optionRestart=FALSE;
	optionToggle=FALSE;
	optionSwitchToView=NULL;
	optionVersion=FALSE;

	/* Setup command-line options */
	context=g_option_context_new(N_(""));
	g_option_context_set_summary(context, N_("A Gnome Shell like dashboard for Expidus1 - version " PACKAGE_VERSION));
	g_option_context_set_translation_domain(context, GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_add_group(context, clutter_get_option_group_without_init());
	g_option_context_add_group(context, expidus_sm_client_get_option_group(inArgc, inArgv));
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

#ifdef DEBUG
#ifdef ESDASHBOARD_ENABLE_DEBUG
	g_print("** Use environment variable ESDASHBOARD_DEBUG to enable debug messages\n");
	g_print("** To get a list of debug categories set ESDASHBOARD_DEBUG=help\n");
#endif
#endif

	if(!g_option_context_parse(context, &inArgc, &inArgv, &error))
	{
		/* Show error */
		g_print(N_("%s\n"),
				(error && error->message) ? error->message : _("unknown error"));
		if(error)
		{
			g_error_free(error);
			error=NULL;
		}

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		return(ESDASHBOARD_APPLICATION_ERROR_FAILED);
	}

	/* Set up debug flags */
#ifdef ESDASHBOARD_ENABLE_DEBUG
	{
		const gchar					*environment;
		static const GDebugKey		debugKeys[]=
									{
										{ "misc", ESDASHBOARD_DEBUG_MISC },
										{ "actor", ESDASHBOARD_DEBUG_ACTOR },
										{ "style", ESDASHBOARD_DEBUG_STYLE },
										{ "styling", ESDASHBOARD_DEBUG_STYLE },
										{ "theme", ESDASHBOARD_DEBUG_THEME },
										{ "apps", ESDASHBOARD_DEBUG_APPLICATIONS },
										{ "applications", ESDASHBOARD_DEBUG_APPLICATIONS },
										{ "images", ESDASHBOARD_DEBUG_IMAGES },
										{ "windows", ESDASHBOARD_DEBUG_WINDOWS },
										{ "window-tracker", ESDASHBOARD_DEBUG_WINDOWS },
										{ "animation", ESDASHBOARD_DEBUG_ANIMATION },
										{ "animations", ESDASHBOARD_DEBUG_ANIMATION },
										{ "plugin", ESDASHBOARD_DEBUG_PLUGINS },
										{ "plugins", ESDASHBOARD_DEBUG_PLUGINS },
									};

		/* Parse debug flags */
		environment=g_getenv("ESDASHBOARD_DEBUG");
		if(environment)
		{
			esdashboard_debug_flags=
				g_parse_debug_string(environment,
										debugKeys,
										G_N_ELEMENTS(debugKeys));
			environment=NULL;
		}

		/* Parse object names to debug */
		environment=g_getenv("ESDASHBOARD_DEBUG");
		if(environment)
		{
			if(G_UNLIKELY(esdashboard_debug_classes))
			{
				g_strfreev(esdashboard_debug_classes);
				esdashboard_debug_classes=NULL;
			}

			esdashboard_debug_classes=g_strsplit(environment, ",", -1);
			environment=NULL;
		}
	}
#endif

	/* If this application instance is a remote instance do not handle any
	 * command-line argument. The arguments will be sent to the primary instance,
	 * handled there and the exit code will be sent back to the remote instance.
	 * We can check for remote instance with g_application_get_is_remote() because
	 * the application was tried to get registered either by local_command_line
	 * signal handler or by g_application_run().
	 */
	if(g_application_get_is_remote(G_APPLICATION(self)))
	{
		ESDASHBOARD_DEBUG(self, MISC, "Do not handle command-line parameters on remote application instance");

		/* One exception is "--version" */
		if(optionVersion)
		{
			g_print("Remote instance: %s-%s\n", PACKAGE_NAME, PACKAGE_VERSION);
		}

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		/* No errors so far and no errors will happen as we do not handle
		 * any command-line arguments here.
		 */
		return(ESDASHBOARD_APPLICATION_ERROR_NONE);
	}
	ESDASHBOARD_DEBUG(self, MISC, "Handling command-line parameters on primary application instance");

	/* Handle options: restart
	 *
	 * First handle option 'restart' to quit this application early. Also return
	 * immediately with status ESDASHBOARD_APPLICATION_ERROR_RESTART to tell
	 * remote instance to perform a restart and to become the new primary instance.
	 * This option requires that application was initialized.
	 */
	if(optionRestart &&
		priv->initialized)
	{
		/* Quit existing instance for restart */
		ESDASHBOARD_DEBUG(self, MISC, "Received request to restart application!");
		_esdashboard_application_quit(self, TRUE);

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		/* Return state to restart this applicationa */
		return(ESDASHBOARD_APPLICATION_ERROR_RESTART);
	}

	/* Handle options: quit
	 *
	 * Now check for the next application stopping option 'quit'. We check for it
	 * to quit this application early and to prevent further and unnecessary check
	 * for other options.
	 */
	if(optionQuit)
	{
		/* Quit existing instance */
		ESDASHBOARD_DEBUG(self, MISC, "Received request to quit running instance!");
		_esdashboard_application_quit(self, TRUE);

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		return(ESDASHBOARD_APPLICATION_ERROR_QUIT);
	}

	/* Handle options: toggle
	 *
	 * Now check if we should toggle the state of application. That means
	 * if application is running daemon mode, resume it if suspended otherwise
	 * suspend it. If application is running but not in daemon just quit the
	 * application. If application was not inited yet, skip toggling state and
	 * perform a normal start-up.
	 */
	if(optionToggle &&
		priv->initialized)
	{
		/* If application is running in daemon mode, toggle between suspend/resume ... */
		if(priv->isDaemon)
		{
			if(priv->isSuspended)
			{
				/* Switch to view if requested */
				_esdashboard_application_switch_to_view(self, optionSwitchToView);

				/* Show application again */
				_esdashboard_application_activate(G_APPLICATION(self));
			}
				else
				{
					/* Hide application */
					_esdashboard_application_quit(self, FALSE);
				}
		}
			/* ... otherwise if not running in daemon mode, just quit */
			else
			{
				/* Hide application */
				_esdashboard_application_quit(self, FALSE);
			}

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		/* Stop here because option was handled and application does not get initialized */
		return(ESDASHBOARD_APPLICATION_ERROR_NONE);
	}

	/* Handle options: daemonize
	 *
	 * Check if application shoud run in daemon mode. A daemonized instance runs in
	 * background and does not present the stage initially. The application must not
	 * be initialized yet as it can only be done on start-up.
	 *
	 * Does not work if a new instance was forced.
	 */
	if(optionDaemonize &&
		!priv->initialized)
	{
		if(!priv->forcedNewInstance)
		{
			priv->isDaemon=optionDaemonize;
			g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationProperties[PROP_DAEMONIZED]);

			if(priv->isDaemon)
			{
				priv->isSuspended=TRUE;
				g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationProperties[PROP_SUSPENDED]);
			}
		}
			else
			{
				g_warning("Cannot daemonized because a temporary new instance of application was forced.");
			}
	}

	/* Handle options: version
	 *
	 * Show the version of this (maybe daemonized) instance.
	 */
	if(optionVersion)
	{
		if(priv->isDaemon)
		{
			g_print("Daemon instance: %s-%s\n", PACKAGE_NAME, PACKAGE_VERSION);
		}
			else
			{
				g_print("Version: %s-%s\n", PACKAGE_NAME, PACKAGE_VERSION);
				return(ESDASHBOARD_APPLICATION_ERROR_QUIT);
			}
	}

	/* Check if this instance needs to be initialized fully */
	if(!priv->initialized)
	{
		/* Perform full initialization of this application instance */
		result=_esdashboard_application_initialize_full(self);
		if(result==FALSE) return(ESDASHBOARD_APPLICATION_ERROR_FAILED);

		/* Switch to view if requested */
		_esdashboard_application_switch_to_view(self, optionSwitchToView);

		/* Show application if not started daemonized */
		if(!priv->isDaemon) clutter_actor_show(CLUTTER_ACTOR(priv->stage));

		/* Take extra reference on the application to keep application in
		 * function g_application_run() alive when returning.
		 */
		g_application_hold(G_APPLICATION(self));
	}

	/* Check if this instance need to be activated. Is should only be done
	 * if instance is initialized
	 */
	if(priv->initialized)
	{
		/* Switch to view if requested */
		_esdashboard_application_switch_to_view(self, optionSwitchToView);

		/* Show application */
		_esdashboard_application_activate(G_APPLICATION(self));
	}

	/* Release allocated resources */
	if(optionSwitchToView) g_free(optionSwitchToView);
	if(context) g_option_context_free(context);

	/* All done successfully so return status code 0 for success */
	priv->initialized=TRUE;
	return(ESDASHBOARD_APPLICATION_ERROR_NONE);
}

/* IMPLEMENTATION: GApplication */

/* Received "activate" signal on primary instance */
static void _esdashboard_application_activate(GApplication *inApplication)
{
	EsdashboardApplication			*self;
	EsdashboardApplicationPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(inApplication));

	self=ESDASHBOARD_APPLICATION(inApplication);
	priv=self->priv;

	/* Emit "resume" signal */
	g_signal_emit(self, EsdashboardApplicationSignals[SIGNAL_RESUME], 0);

	/* Unset flag for suspension */
	if(priv->isSuspended)
	{
		priv->isSuspended=FALSE;
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationProperties[PROP_SUSPENDED]);
	}
}

/* Handle command-line on primary instance */
static int _esdashboard_application_command_line(GApplication *inApplication, GApplicationCommandLine *inCommandLine)
{
	EsdashboardApplication			*self;
	gint							argc;
	gchar							**argv;
	gint							exitStatus;

	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATION(inApplication), ESDASHBOARD_APPLICATION_ERROR_FAILED);

	self=ESDASHBOARD_APPLICATION(inApplication);

	/* Get number of command-line arguments and the arguments */
	argv=g_application_command_line_get_arguments(inCommandLine, &argc);

	/* Parse command-line and get exit status code */
	exitStatus=_esdashboard_application_handle_command_line_arguments(self, argc, argv);

	/* Release allocated resources */
	if(argv) g_strfreev(argv);

	/* Return exit status code */
	return(exitStatus);
}

/* Check and handle command-line on local instance regardless if this one
 * is the primary instance or a remote one. This functions checks for arguments
 * which can be handled locally, e.g. '--help'. Otherwise they will be send to
 * the primary instance.
 */
static gboolean _esdashboard_application_local_command_line(GApplication *inApplication,
															gchar ***ioArguments,
															int *outExitStatus)
{
	EsdashboardApplication			*self;
	GError							*error;

	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATION(inApplication), TRUE);

	self=ESDASHBOARD_APPLICATION(inApplication);
	error=NULL;

	/* Set exit status code initially to success result but can be changed later */
	if(outExitStatus) *outExitStatus=ESDASHBOARD_APPLICATION_ERROR_NONE;

	/* Try to register application to determine early if this instance will be
	 * the primary application instance or a remote one.
	 */
	if(!g_application_register(inApplication, NULL, &error))
	{
		g_critical("Unable to register application: %s",
					(error && error->message) ? error->message : "Unknown error");
		if(error)
		{
			g_error_free(error);
			error=NULL;
		}

		/* Set error status code */
		if(outExitStatus) *outExitStatus=ESDASHBOARD_APPLICATION_ERROR_FAILED;

		/* Return TRUE to indicate that command-line does not need further processing */
		return(TRUE);
	}

	/* If this is a remote instance we need to parse command-line now */
	if(g_application_get_is_remote(inApplication))
	{
		gint						argc;
		gchar						**argv;
		gchar						**originArgv;
		gint						exitStatus;
		gint						i;

		/* We have to make a extra copy of the command-line arguments, since
		 * g_option_context_parse() in command-line argument handling function
		 * might remove parameters from the arguments list and maybe we need
		 * them to sent the arguments to primary instance if not handled locally
		 * like '--help'.
		 */
		originArgv=*ioArguments;

		argc=g_strv_length(originArgv);
		argv=g_new0(gchar*, argc+1);
		for(i=0; i<=argc; i++) argv[i]=g_strdup(originArgv[i]);

		/* Parse command-line and store exit status code */
		exitStatus=_esdashboard_application_handle_command_line_arguments(self, argc, argv);
		if(outExitStatus) *outExitStatus=exitStatus;

		/* Release allocated resources */
		if(argv) g_strfreev(argv);

		/* If exit status code indicates an error then return TRUE here to indicate
		 * that command-line does not need further processing.
		 */
		if(exitStatus==ESDASHBOARD_APPLICATION_ERROR_FAILED) return(TRUE);
	}

	/* Return FALSE to indicate that command-line was not completely handled
	 * and needs further processing, e.g. this is the primary instance or a remote
	 * instance which could not handle the arguments locally.
	 */
	return(FALSE);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_application_dispose(GObject *inObject)
{
	EsdashboardApplication			*self=ESDASHBOARD_APPLICATION(inObject);
	EsdashboardApplicationPrivate	*priv=self->priv;

	/* Ensure "is-quitting" flag is set just in case someone asks */
	priv->isQuitting=TRUE;

	/* Signal "shutdown-final" of application */
	g_signal_emit(self, EsdashboardApplicationSignals[SIGNAL_SHUTDOWN_FINAL], 0);

	/* Release allocated resources */
	if(priv->windowTrackerBackend)
	{
		g_object_unref(priv->windowTrackerBackend);
		priv->windowTrackerBackend=NULL;
	}

	if(priv->pluginManager)
	{
		g_object_unref(priv->pluginManager);
		priv->pluginManager=NULL;
	}

	if(priv->esconfThemeChangedSignalID)
	{
		esconf_g_property_unbind(priv->esconfThemeChangedSignalID);
		priv->esconfThemeChangedSignalID=0L;
	}

	if(priv->viewManager)
	{
		/* Unregisters all remaining registered views - no need to unregister them here */
		g_object_unref(priv->viewManager);
		priv->viewManager=NULL;
	}

	if(priv->searchManager)
	{
		/* Unregisters all remaining registered providers - no need to unregister them here */
		g_object_unref(priv->searchManager);
		priv->searchManager=NULL;
	}

	if(priv->focusManager)
	{
		/* Unregisters all remaining registered focusable actors.
		 * There is no need to unregister them here.
		 */
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	if(priv->bindings)
	{
		g_object_unref(priv->bindings);
		priv->bindings=NULL;
	}

	if(priv->appDatabase)
	{
		g_object_unref(priv->appDatabase);
		priv->appDatabase=NULL;
	}

	if(priv->appTracker)
	{
		g_object_unref(priv->appTracker);
		priv->appTracker=NULL;
	}

	if(priv->theme)
	{
		g_object_unref(priv->theme);
		priv->theme=NULL;
	}

	if(priv->themeName)
	{
		g_free(priv->themeName);
		priv->themeName=NULL;
	}

	if(priv->stage)
	{
		g_object_unref(priv->stage);
		priv->stage=NULL;
	}

	/* Shutdown session management */
	if(priv->sessionManagementClient)
	{
		/* This instance looks like to be disposed normally and not like a crash
		 * so set the restart style at session management to something that it
		 * will not restart itself but shutting down.
		 */
		if(EXPIDUS_IS_SM_CLIENT(priv->sessionManagementClient))
		{
			expidus_sm_client_set_restart_style(priv->sessionManagementClient, EXPIDUS_SM_CLIENT_RESTART_NORMAL);
		}

		priv->sessionManagementClient=NULL;
	}

	/* Shutdown esconf */
	priv->esconfChannel=NULL;
	esconf_shutdown();

	/* Unset singleton */
	if(G_LIKELY(G_OBJECT(_esdashboard_application)==inObject)) _esdashboard_application=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_application_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_application_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardApplication	*self=ESDASHBOARD_APPLICATION(inObject);

	switch(inPropID)
	{
		case PROP_THEME_NAME:
			_esdashboard_application_set_theme_name(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_application_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardApplication	*self=ESDASHBOARD_APPLICATION(inObject);

	switch(inPropID)
	{
		case PROP_DAEMONIZED:
			g_value_set_boolean(outValue, self->priv->isDaemon);
			break;

		case PROP_SUSPENDED:
			g_value_set_boolean(outValue, self->priv->isSuspended);
			break;

		case PROP_STAGE:
			g_value_set_object(outValue, self->priv->stage);
			break;

		case PROP_THEME_NAME:
			g_value_set_string(outValue, self->priv->themeName);
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
static void esdashboard_application_class_init(EsdashboardApplicationClass *klass)
{
	GApplicationClass	*appClass=G_APPLICATION_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	klass->exit=_esdashboard_application_action_exit;

	appClass->activate=_esdashboard_application_activate;
	appClass->command_line=_esdashboard_application_command_line;
	appClass->local_command_line=_esdashboard_application_local_command_line;

	gobjectClass->dispose=_esdashboard_application_dispose;
	gobjectClass->set_property=_esdashboard_application_set_property;
	gobjectClass->get_property=_esdashboard_application_get_property;

	/* Define properties */
	/**
	 * EsdashboardApplication:is-daemonized:
	 *
	 * A flag indicating if application is running in daemon mode. It is set to
	 * %TRUE if it running in daemon mode otherwise %FALSE as it running in
	 * standalone application.
	 */
	EsdashboardApplicationProperties[PROP_DAEMONIZED]=
		g_param_spec_boolean("is-daemonized",
								"Is daemonized",
								"Flag indicating if application is daemonized",
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardApplication:is-suspended:
	 *
	 * A flag indicating if application running in daemon mode is suspended.
	 * It is set to %TRUE if application is suspended. If it is %FALSE then
	 * application is resumed, active and visible.
	 */
	EsdashboardApplicationProperties[PROP_SUSPENDED]=
		g_param_spec_boolean("is-suspended",
								"Is suspended",
								"Flag indicating if application is suspended currently",
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardApplication:stage:
	 *
	 * The #EsdashboardStage of application.
	 */
	EsdashboardApplicationProperties[PROP_STAGE]=
		g_param_spec_object("stage",
								"Stage",
								"The stage object of application",
								ESDASHBOARD_TYPE_STAGE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardApplication:theme-name:
	 *
	 * The name of current theme used in application.
	 */
	EsdashboardApplicationProperties[PROP_THEME_NAME]=
		g_param_spec_string("theme-name",
								"Theme name",
								"Name of current theme",
								DEFAULT_THEME_NAME,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardApplicationProperties);

	/* Define signals */
	/**
	 * EsdashboardApplication::initialized:
	 * @self: The application's primary instance was fully initialized
	 *
	 * The ::initialized signal is emitted when the primary instance of
	 * application was fully initialized.
	 *
	 * The main purpose of this signal is to give other objects and plugin
	 * a chance to initialize properly after application is fully initialized
	 * and all other components are really available and also initialized.
	 */
	EsdashboardApplicationSignals[SIGNAL_INITIALIZED]=
		g_signal_new("initialized",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, initialized),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * EsdashboardApplication::quit:
	 * @self: The application going to quit
	 *
	 * The ::quit signal is emitted when the application is going to quit. This
	 * signal is definitely emitted before the stage is destroyed.
	 *
	 * The main purpose of this signal is to give other objects and plugin
	 * a chance to clean-up properly.
	 */
	EsdashboardApplicationSignals[SIGNAL_QUIT]=
		g_signal_new("quit",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, quit),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * EsdashboardApplication::shutdown-final:
	 * @self: The application being destroyed
	 *
	 * The ::shutdown-final signal is emitted when the application object
	 * is currently destroyed and can be considered as quitted. The stage is
	 * detroyed when this signal is emitted and only the non-visible managers
	 * are still accessible at this time.
	 */
	EsdashboardApplicationSignals[SIGNAL_SHUTDOWN_FINAL]=
		g_signal_new("shutdown-final",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, shutdown_final),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * EsdashboardApplication::suspend:
	 * @self: The application suspended
	 *
	 * The ::suspend signal is emitted when the application is suspended and
	 * send to background.
	 *
	 * This signal is only emitted if application is running in daemon mode.
	 */
	EsdashboardApplicationSignals[SIGNAL_SUSPEND]=
		g_signal_new("suspend",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, suspend),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * EsdashboardApplication::resume:
	 * @self: The application resumed
	 *
	 * The ::resume signal is emitted when the application is resumed and
	 * brought to foreground.
	 *
	 * This signal is only emitted if application is running in daemon mode.
	 */
	EsdashboardApplicationSignals[SIGNAL_RESUME]=
		g_signal_new("resume",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, resume),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * EsdashboardApplication::theme-loading:
	 * @self: The application whose theme is going to change
	 * @inTheme: The new #EsdashboardTheme used
	 *
	 * The ::theme-loading signal is emitted when the theme of application is
	 * going to be loaded. When this signal is received no file was loaded so far
	 * but will be. For example, at this moment it is possible to load a CSS file
	 * by a plugin before the CSS files of the new theme will be loaded to give
	 * the theme a chance to override the default CSS of plugin.
	 */
	EsdashboardApplicationSignals[SIGNAL_THEME_LOADING]=
		g_signal_new("theme-loading",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, theme_loading),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						ESDASHBOARD_TYPE_THEME);

	/**
	 * EsdashboardApplication::theme-loaded:
	 * @self: The application whose theme is going to change
	 * @inTheme: The new #EsdashboardTheme used
	 *
	 * The ::theme-loaded signal is emitted when the new theme of application was
	 * loaded and will soon be applied. When this signal is received it is the
	 * last chance for other components and plugins to load additionally resources
	 * like CSS, e.g. to override CSS of theme.
	 */
	EsdashboardApplicationSignals[SIGNAL_THEME_LOADED]=
		g_signal_new("theme-loaded",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, theme_loaded),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						ESDASHBOARD_TYPE_THEME);

	/**
	 * EsdashboardApplication::theme-changed:
	 * @self: The application whose theme has changed
	 * @inTheme: The new #EsdashboardTheme used
	 *
	 * The ::theme-changed signal is emitted when a new theme of application
	 * has been loaded and applied.
	 */
	EsdashboardApplicationSignals[SIGNAL_THEME_CHANGED]=
		g_signal_new("theme-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, theme_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						ESDASHBOARD_TYPE_THEME);

	/**
	 * EsdashboardApplication::application-launched:
	 * @self: The application
	 * @inAppInfo: The #GAppInfo that was just launched
	 *
	 * The ::application-launched signal is emitted when a #GAppInfo has been
	 * launched successfully, e.g. via any #EsdashboardApplicationButton.
	 */
	EsdashboardApplicationSignals[SIGNAL_APPLICATION_LAUNCHED]=
		g_signal_new("application-launched",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, application_launched),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_APP_INFO);

	/**
	 * EsdashboardApplication::exit:
	 * @self: The application
	 * @inFocusable: The source #EsdashboardFocusable which send this signal
	 * @inAction: A string containing the action (that is this signal name)
	 * @inEvent: The #ClutterEvent associated to this signal and action
	 *
	 * The ::exit signal is an action and when emitted it causes the application
	 * to quit like calling esdashboard_application_quit_or_suspend().
	 *
	 * Note: This action can be used at key bindings.
	 */
	EsdashboardApplicationSignals[ACTION_EXIT]=
		g_signal_new("exit",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardApplicationClass, exit),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	/* Register GValue transformation function not provided by any other library */
	esdashboard_register_gvalue_transformation_funcs();
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_application_init(EsdashboardApplication *self)
{
	EsdashboardApplicationPrivate	*priv;
	GSimpleAction					*action;

	priv=self->priv=esdashboard_application_get_instance_private(self);

	/* Set default values */
	priv->isDaemon=FALSE;
	priv->isSuspended=FALSE;
	priv->themeName=NULL;
	priv->initialized=FALSE;
	priv->esconfChannel=NULL;
	priv->stage=NULL;
	priv->viewManager=NULL;
	priv->searchManager=NULL;
	priv->focusManager=NULL;
	priv->theme=NULL;
	priv->esconfThemeChangedSignalID=0L;
	priv->isQuitting=FALSE;
	priv->sessionManagementClient=NULL;
	priv->pluginManager=NULL;
	priv->forcedNewInstance=FALSE;
	priv->windowTrackerBackend=NULL;

	/* Add callable DBUS actions for this application */
	action=g_simple_action_new("Quit", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(esdashboard_application_quit_forced), NULL);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(action));
	g_object_unref(action);
}

/* IMPLEMENTATION: Public API */

/**
 * esdashboard_application_has_default:
 *
 * Determine if the singleton instance of #EsdashboardApplication was created.
 * If the singleton instance of #EsdashboardApplication was created, it can be
 * retrieved with esdashboard_application_get_default().
 *
 * This function is useful if only the availability of the singleton instance
 * wants to be checked as esdashboard_application_get_default() will create this
 * singleton instance if not available.
 *
 * Return value: %TRUE if singleton instance of #EsdashboardApplication was
 *   created or %FALSE if not.
 */
gboolean esdashboard_application_has_default(void)
{
	if(G_LIKELY(_esdashboard_application)) return(TRUE);

	return(FALSE);
}

/**
 * esdashboard_application_get_default:
 *
 * Retrieves the singleton instance of #EsdashboardApplication.
 *
 * Return value: (transfer none): The instance of #EsdashboardApplication.
 *   The returned object is owned by Esdashboard and it should not be
 *   unreferenced directly.
 */
EsdashboardApplication* esdashboard_application_get_default(void)
{
	if(G_UNLIKELY(!_esdashboard_application))
	{
		gchar			*appID;
		const gchar		*forceNewInstance=NULL;

#ifdef DEBUG
		/* If a new instance of esdashboard is forced, e.g. for debugging purposes,
		 * then create a unique application ID.
		 */
		forceNewInstance=g_getenv("ESDASHBOARD_FORCE_NEW_INSTANCE");
#endif

		if(forceNewInstance)
		{
			appID=g_strdup_printf("%s-%u", ESDASHBOARD_APP_ID, getpid());
			g_message("Forcing new application instance with ID '%s'", appID);
		}
			else appID=g_strdup(ESDASHBOARD_APP_ID);

		_esdashboard_application=g_object_new(ESDASHBOARD_TYPE_APPLICATION,
												"application-id", appID,
												"flags", G_APPLICATION_HANDLES_COMMAND_LINE,
												NULL);

		if(forceNewInstance)
		{
			_esdashboard_application->priv->forcedNewInstance=TRUE;
		}

		/* Release allocated resources */
		if(appID) g_free(appID);
	}

	return(_esdashboard_application);
}

/**
 * esdashboard_application_is_daemonized:
 * @self: A #EsdashboardApplication
 *
 * Checks if application is running in background (daemon mode).
 *
 * Return value: %TRUE if @self is running in background and
 *   %FALSE if it is running as standalone application.
 */
gboolean esdashboard_application_is_daemonized(EsdashboardApplication *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATION(self), FALSE);

	return(self->priv->isDaemon);
}

/**
 * esdashboard_application_is_suspended:
 * @self: A #EsdashboardApplication
 *
 * Checks if application is suspended, that means it is not visible and
 * not active.
 *
 * Note: This state can only be check when @self is running in daemon mode.
 *
 * Return value: %TRUE if @self is suspended and %FALSE if it is resumed.
 */
gboolean esdashboard_application_is_suspended(EsdashboardApplication *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATION(self), FALSE);

	return(self->priv->isSuspended);
}

/**
 * esdashboard_application_is_quitting:
 * @self: A #EsdashboardApplication
 *
 * Checks if application is in progress to quit.
 *
 * Return value: %TRUE if @self is going to quit, otherwise %FALSE.
 */
gboolean esdashboard_application_is_quitting(EsdashboardApplication *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATION(self), FALSE);

	return(self->priv->isQuitting);
}

/**
 * esdashboard_application_resume:
 * @self: A #EsdashboardApplication or %NULL
 *
 * Resumes @self from suspended state, brings it to foreground
 * and activates it.
 *
 * If @self is %NULL the default singleton is used if it was created.
 */
void esdashboard_application_resume(EsdashboardApplication *self)
{
	g_return_if_fail(self==NULL || ESDASHBOARD_IS_APPLICATION(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_esdashboard_application;

	/* Resume application */
	if(G_LIKELY(self))
	{
		_esdashboard_application_activate(G_APPLICATION(self));
	}
}

/**
 * esdashboard_application_suspend_or_quit:
 * @self: A #EsdashboardApplication or %NULL
 *
 * Quits @self if running as standalone application or suspends @self
 * if runnning if running in daemon mode. Suspending application means
 * to hide application window and send it to background.
 *
 * If @self is %NULL the default singleton is used if it was created.
 */
void esdashboard_application_suspend_or_quit(EsdashboardApplication *self)
{
	g_return_if_fail(self==NULL || ESDASHBOARD_IS_APPLICATION(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_esdashboard_application;

	/* Quit application */
	if(G_LIKELY(self))
	{
		_esdashboard_application_quit(self, FALSE);
	}
}

/**
 * esdashboard_application_quit_forced:
 * @self: A #EsdashboardApplication or %NULL
 *
 * Quits @self regardless if it is running as standalone application
 * or in daemon mode. The application is really quitted after calling
 * this function.
 *
 * If @self is %NULL the default singleton is used if it was created.
 */
void esdashboard_application_quit_forced(EsdashboardApplication *self)
{
	g_return_if_fail(self==NULL || ESDASHBOARD_IS_APPLICATION(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_esdashboard_application;

	/* Force application to quit */
	if(G_LIKELY(self))
	{
		/* Quit also any other running instance */
		if(g_application_get_is_remote(G_APPLICATION(self))==TRUE)
		{
			g_action_group_activate_action(G_ACTION_GROUP(self), "Quit", NULL);
		}

		/* Quit this instance */
		_esdashboard_application_quit(self, TRUE);
	}
}

/**
 * esdashboard_application_get_stage:
 * @self: A #EsdashboardApplication or %NULL
 *
 * Retrieve #EsdashboardStage of @self.
 *
 * If @self is %NULL the default singleton is used if it was created.
 *
 * Return value: (transfer none): The #EsdashboardStage of application at @self.
 */
EsdashboardStage* esdashboard_application_get_stage(EsdashboardApplication *self)
{
	EsdashboardStage		*stage;

	g_return_val_if_fail(self==NULL || ESDASHBOARD_IS_APPLICATION(self), NULL);

	stage=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_esdashboard_application;

	/* Get esconf channel */
	if(G_LIKELY(self)) stage=self->priv->stage;

	return(stage);
}

/**
 * esdashboard_application_get_theme:
 * @self: A #EsdashboardApplication or %NULL
 *
 * Retrieve the current #EsdashboardTheme of @self.
 *
 * If @self is %NULL the default singleton is used if it was created.
 *
 * Return value: (transfer none): The current #EsdashboardTheme of application at @self.
 */
EsdashboardTheme* esdashboard_application_get_theme(EsdashboardApplication *self)
{
	EsdashboardTheme		*theme;

	g_return_val_if_fail(self==NULL || ESDASHBOARD_IS_APPLICATION(self), NULL);

	theme=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_esdashboard_application;

	/* Get theme */
	if(G_LIKELY(self)) theme=self->priv->theme;

	return(theme);
}

/**
 * esdashboard_application_get_esconf_channel:
 * @self: A #EsdashboardApplication or %NULL
 *
 * Retrieve the #EsconfChannel of @self used to query or modify settings stored
 * in Esconf.
 *
 * If @self is %NULL the default singleton is used if it was created.
 *
 * Return value: (transfer none): The current #EsconfChannel of application at @self.
 */
EsconfChannel* esdashboard_application_get_esconf_channel(EsdashboardApplication *self)
{
	EsconfChannel			*channel;

	g_return_val_if_fail(self==NULL || ESDASHBOARD_IS_APPLICATION(self), NULL);

	channel=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_esdashboard_application;

	/* Get esconf channel */
	if(G_LIKELY(self)) channel=self->priv->esconfChannel;

	return(channel);
}
