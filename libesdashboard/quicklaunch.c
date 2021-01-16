/*
 * quicklaunch: Quicklaunch box
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


#include <libesdashboard/quicklaunch.h>

#include <glib/gi18n-lib.h>
#include <math.h>
#include <gtk/gtk.h>

#include <libesdashboard/enums.h>
#include <libesdashboard/application.h>
#include <libesdashboard/application-button.h>
#include <libesdashboard/toggle-button.h>
#include <libesdashboard/drag-action.h>
#include <libesdashboard/drop-action.h>
#include <libesdashboard/applications-view.h>
#include <libesdashboard/tooltip-action.h>
#include <libesdashboard/focusable.h>
#include <libesdashboard/marshal.h>
#include <libesdashboard/desktop-app-info.h>
#include <libesdashboard/desktop-app-info-action.h>
#include <libesdashboard/application-database.h>
#include <libesdashboard/application-tracker.h>
#include <libesdashboard/window-tracker.h>
#include <libesdashboard/window-tracker-window.h>
#include <libesdashboard/click-action.h>
#include <libesdashboard/popup-menu.h>
#include <libesdashboard/popup-menu-item-button.h>
#include <libesdashboard/popup-menu-item-separator.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
static void _esdashboard_quicklaunch_focusable_iface_init(EsdashboardFocusableInterface *iface);

struct _EsdashboardQuicklaunchPrivate
{
	/* Properties related */
	GPtrArray						*favourites;

	gfloat							normalIconSize;
	gfloat							scaleMin;
	gfloat							scaleMax;
	gfloat							scaleStep;

	gfloat							spacing;

	ClutterOrientation				orientation;

	/* Instance related */
	EsconfChannel					*esconfChannel;
	guint							esconfFavouritesBindingID;

	gfloat							scaleCurrent;

	ClutterActor					*appsButton;
	ClutterActor					*trashButton;

	guint							dragMode;
	ClutterActor					*dragPreviewIcon;

	ClutterActor					*selectedItem;

	ClutterActor					*separatorFavouritesToDynamic;

	EsdashboardApplicationDatabase	*appDB;
	EsdashboardApplicationTracker	*appTracker;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardQuicklaunch,
						esdashboard_quicklaunch,
						ESDASHBOARD_TYPE_BACKGROUND,
						G_ADD_PRIVATE(EsdashboardQuicklaunch)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_FOCUSABLE, _esdashboard_quicklaunch_focusable_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_FAVOURITES,
	PROP_NORMAL_ICON_SIZE,
	PROP_SPACING,
	PROP_ORIENTATION,

	PROP_LAST
};

static GParamSpec* EsdashboardQuicklaunchProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_FAVOURITE_ADDED,
	SIGNAL_FAVOURITE_REMOVED,

	ACTION_SELECTION_ADD_FAVOURITE,
	ACTION_SELECTION_REMOVE_FAVOURITE,
	ACTION_FAVOURITE_REORDER_LEFT,
	ACTION_FAVOURITE_REORDER_RIGHT,
	ACTION_FAVOURITE_REORDER_UP,
	ACTION_FAVOURITE_REORDER_DOWN,

	SIGNAL_LAST
};

static guint EsdashboardQuicklaunchSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define FAVOURITES_ESCONF_PROP				"/favourites"

#define LAUNCH_NEW_INSTANCE_ESCONF_PROP		"/always-launch-new-instance"
#define DEFAULT_LAUNCH_NEW_INSTANCE			TRUE

#define DEFAULT_SCALE_MIN			0.1
#define DEFAULT_SCALE_MAX			1.0
#define DEFAULT_SCALE_STEP			0.1

enum
{
	DRAG_MODE_NONE,
	DRAG_MODE_CREATE,
	DRAG_MODE_MOVE_EXISTING
};

/* Forward declarations */
static void _esdashboard_quicklaunch_update_property_from_icons(EsdashboardQuicklaunch *self);
static ClutterActor* _esdashboard_quicklaunch_create_dynamic_actor(EsdashboardQuicklaunch *self, GAppInfo *inAppInfo);
static ClutterActor* _esdashboard_quicklaunch_create_favourite_actor(EsdashboardQuicklaunch *self, GAppInfo *inAppInfo);

/* Get actor for desktop application information */
static ClutterActor* _esdashboard_quicklaunch_get_actor_for_appinfo(EsdashboardQuicklaunch *self,
																	GAppInfo *inAppInfo)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActorIter				iter;
	ClutterActor					*child;
	GAppInfo						*desktopAppInfo;
	GFile							*desktopFile;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), NULL);
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), NULL);

	priv=self->priv;

	/* If requested application information does not contain a desktop file
	 * (means it must derive from EsdashboardDesktopAppInfo) then assume
	 * no actor exists for it.
	 */
	if(!ESDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo))
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"%s is derived from %s but not derived %s",
							G_OBJECT_TYPE_NAME(inAppInfo),
							g_type_name(G_TYPE_APP_INFO),
							g_type_name(ESDASHBOARD_TYPE_DESKTOP_APP_INFO));
		return(NULL);
	}

	/* Check if application information is valid and provides a desktop file */
	desktopFile=esdashboard_desktop_app_info_get_file(ESDASHBOARD_DESKTOP_APP_INFO(inAppInfo));
	if(!desktopFile)
	{
		g_critical("Could not check for duplicates for invalid %s object so assume no actor exists",
					G_OBJECT_TYPE_NAME(inAppInfo));
		return(NULL);
	}

	/* Iterate through actors and check each application button if it
	 * provides the requested desktop file of application information.
	 */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check application buttons */
 		if(!ESDASHBOARD_IS_APPLICATION_BUTTON(child)) continue;

		/* Do not check preview icon for drag if available */
		if(priv->dragPreviewIcon && child==priv->dragPreviewIcon) continue;

		/* Check if application button provides requested desktop file */
		desktopAppInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(child));
		if(desktopAppInfo &&
			g_app_info_equal(desktopAppInfo, inAppInfo))
		{
			/* Found actor so return it */
			return(child);
		}
	}

	/* If we get here then this quicklaunch does not have any item
	 * which matches the requested application information so return NULL.
	 */
	return(NULL);
}

/* Check for duplicate application buttons */
static gboolean _esdashboard_quicklaunch_has_favourite_appinfo(EsdashboardQuicklaunch *self, GAppInfo *inAppInfo)
{
	EsdashboardQuicklaunchPrivate	*priv;
	guint							i;
	GFile							*desktopFile;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), TRUE);
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), TRUE);

	priv=self->priv;

	/* If requested application information does not contain a desktop file
	 * (means it must derive from EsdashboardDesktopAppInfo) then assume
	 * it exists already.
	 */
	if(!ESDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo))
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"%s is derived from %s but not derived %s",
							G_OBJECT_TYPE_NAME(inAppInfo),
							g_type_name(G_TYPE_APP_INFO),
							g_type_name(ESDASHBOARD_TYPE_DESKTOP_APP_INFO));
		return(TRUE);
	}

	/* Check if application information is valid and provides a desktop file */
	desktopFile=esdashboard_desktop_app_info_get_file(ESDASHBOARD_DESKTOP_APP_INFO(inAppInfo));
	if(!desktopFile)
	{
		g_critical("Could not check for duplicates for invalid %s object so assume it exists",
					G_OBJECT_TYPE_NAME(inAppInfo));
		return(TRUE);
	}

	/* Iterate through favourites and check if already a favourite for
	 * requested desktop file.
	 */
	for(i=0; i<priv->favourites->len; i++)
	{
		GValue						*value;
		GAppInfo					*valueAppInfo;
		const gchar					*valueAppInfoFilename;

		valueAppInfo=NULL;

		/* Get favourite value and the string it contains */
		value=(GValue*)g_ptr_array_index(priv->favourites, i);
		if(value)
		{
#if DEBUG
			if(!G_VALUE_HOLDS_STRING(value))
			{
				g_critical("Value at %p of type %s is not a %s so assume this desktop application item exists",
							value,
							G_VALUE_TYPE_NAME(value),
							g_type_name(G_TYPE_STRING));
				return(TRUE);
			}
#endif

			/* Get application information for string */
			valueAppInfoFilename=g_value_get_string(value);
			if(g_path_is_absolute(valueAppInfoFilename))
			{
				valueAppInfo=esdashboard_desktop_app_info_new_from_path(valueAppInfoFilename);
			}
				else
				{
					valueAppInfo=esdashboard_application_database_lookup_desktop_id(priv->appDB, valueAppInfoFilename);
				}
		}

		/* Check if favourite value matches application information */
		if(valueAppInfo &&
			g_app_info_equal(valueAppInfo, inAppInfo))
		{

			/* Release allocated resources */
			if(valueAppInfo) g_object_unref(valueAppInfo);

			return(TRUE);
		}

		/* Release allocated resources */
		if(valueAppInfo) g_object_unref(valueAppInfo);
	}

	/* If we get here then this quicklaunch does not have any item
	 * which matches the requested application information so return FALSE.
	 */
	return(FALSE);
}

/* An application icon (favourite) in quicklaunch was clicked */
static void _esdashboard_quicklaunch_on_favourite_clicked(EsdashboardQuicklaunch *self, gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate			*priv;
	EsdashboardApplicationButton			*button;
	gboolean								launchNewInstance;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	priv=self->priv;
	button=ESDASHBOARD_APPLICATION_BUTTON(inUserData);

	/* If user wants to activate the last active windows for a running instance
	 * of application whose button was clicked, then check if a window exists
	 * and activate it. Otherwise launch a new instance.
	 */
	launchNewInstance=esconf_channel_get_bool(esdashboard_application_get_esconf_channel(NULL),
												LAUNCH_NEW_INSTANCE_ESCONF_PROP,
												DEFAULT_LAUNCH_NEW_INSTANCE);
	if(!launchNewInstance)
	{
		GAppInfo							*appInfo;
		const GList							*windows;
		EsdashboardWindowTrackerWindow		*lastActiveWindow;
		EsdashboardWindowTrackerWorkspace	*lastActiveWorkspace;

		/* Get application information of application button */
		appInfo=esdashboard_application_button_get_app_info(button);
		if(!appInfo)
		{
			esdashboard_notify(CLUTTER_ACTOR(self),
								"dialog-error",
								_("Launching application '%s' failed: %s"),
								esdashboard_application_button_get_display_name(button),
								_("No information available for application"));

			g_warning("Launching application '%s' failed: %s",
						esdashboard_application_button_get_display_name(button),
						"No information available for application");

			return;
		}

		/* Get list of windows for application */
		windows=esdashboard_application_tracker_get_window_list_by_app_info(priv->appTracker, appInfo);
		if(windows)
		{
			/* Get last active window for application which is the first one in list */
			lastActiveWindow=ESDASHBOARD_WINDOW_TRACKER_WINDOW(windows->data);

			/* If last active window for application if available, wwitch to workspace
			 * where it is placed at and activate it.
			 */
			if(lastActiveWindow)
			{
				/* Switch to workspace where window is placed at */
				lastActiveWorkspace=esdashboard_window_tracker_window_get_workspace(lastActiveWindow);
				esdashboard_window_tracker_workspace_activate(lastActiveWorkspace);

				/* Activate window */
				esdashboard_window_tracker_window_activate(lastActiveWindow);

				/* Activating last active window of application seems to be successfully
				 * so quit application.
				 */
				esdashboard_application_suspend_or_quit(NULL);

				return;
			}
		}

		/* If we get here we found the application but no active window,
		 * so check if application is running. If it is display a warning
		 * message as notification and return from this function. If it
		 * is not running, continue to start a new instance.
		 */
		if(esdashboard_application_tracker_is_running_by_app_info(priv->appTracker, appInfo))
		{
			esdashboard_notify(CLUTTER_ACTOR(self),
								"dialog-error",
								_("Launching application '%s' failed: %s"),
								esdashboard_application_button_get_display_name(button),
								_("No windows to activate for application"));

			g_warning("Launching application '%s' failed: %s",
						esdashboard_application_button_get_display_name(button),
						"No windows to activate for application");

			return;
		}
	}

	/* Launch a new instance of application whose button was clicked */
	if(esdashboard_application_button_execute(button, NULL))
	{
		/* Launching application seems to be successfully so quit application */
		esdashboard_application_suspend_or_quit(NULL);

		return;
	}
}

/* User selected to open a new window or to launch that application at pop-up menu */
static void _esdashboard_quicklaunch_on_favourite_popup_menu_item_launch(EsdashboardPopupMenuItem *inMenuItem,
																			gpointer inUserData)
{
	GAppInfo							*appInfo;
	EsdashboardApplicationTracker		*appTracker;
	GIcon								*gicon;
	const gchar							*iconName;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem));
	g_return_if_fail(G_IS_APP_INFO(inUserData));

	appInfo=G_APP_INFO(inUserData);
	iconName=NULL;

	/* Get icon of application */
	gicon=g_app_info_get_icon(appInfo);
	if(gicon) iconName=g_icon_to_string(gicon);

	/* Check if we should launch that application or to open a new window */
	appTracker=esdashboard_application_tracker_get_default();
	if(!esdashboard_application_tracker_is_running_by_app_info(appTracker, appInfo))
	{
		GAppLaunchContext			*context;
		GError						*error;

		/* Create context to start application at */
		context=esdashboard_create_app_context(NULL);

		/* Try to launch application */
		error=NULL;
		if(!g_app_info_launch(appInfo, NULL, context, &error))
		{
			/* Show notification about failed application launch */
			esdashboard_notify(CLUTTER_ACTOR(inMenuItem),
								iconName,
								_("Launching application '%s' failed: %s"),
								g_app_info_get_display_name(appInfo),
								(error && error->message) ? error->message : _("unknown error"));
			g_warning("Launching application '%s' failed: %s",
						g_app_info_get_display_name(appInfo),
						(error && error->message) ? error->message : "unknown error");
			if(error) g_error_free(error);
		}
			else
			{
				/* Show notification about successful application launch */
				esdashboard_notify(CLUTTER_ACTOR(inMenuItem),
									iconName,
									_("Application '%s' launched"),
									g_app_info_get_display_name(appInfo));

				/* Emit signal for successful application launch */
				g_signal_emit_by_name(esdashboard_application_get_default(), "application-launched", appInfo);

				/* Quit application */
				esdashboard_application_suspend_or_quit(NULL);
			}

		/* Release allocated resources */
		g_object_unref(context);
	}

	/* Release allocated resources */
	g_object_unref(appTracker);
	g_object_unref(gicon);
}

/* User selected to remove application from favourites via pop-up menu */
static void _esdashboard_quicklaunch_on_favourite_popup_menu_item_remove_from_favourite(EsdashboardPopupMenuItem *inMenuItem,
																						gpointer inUserData)
{
	EsdashboardApplicationButton		*appButton;
	GAppInfo							*appInfo;
	ClutterActor						*actor;
	EsdashboardQuicklaunch				*self;
	EsdashboardQuicklaunchPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	appButton=ESDASHBOARD_APPLICATION_BUTTON(inUserData);

	/* Find quicklaunch for application button */
	actor=CLUTTER_ACTOR(appButton);
	do
	{
		actor=clutter_actor_get_parent(actor);
	}
	while(actor && !ESDASHBOARD_IS_QUICKLAUNCH(actor));

	if(!actor)
	{
		g_critical("Cannot find quicklaunch for application button.");
		return;
	}

	self=ESDASHBOARD_QUICKLAUNCH(actor);
	priv=self->priv;

	/* Notify about removal of favourite icon */
	esdashboard_notify(CLUTTER_ACTOR(self),
						esdashboard_application_button_get_icon_name(appButton),
						_("Favourite '%s' removed"),
						esdashboard_application_button_get_display_name(appButton));

	/* Emit signal and re-add removed favourite as dynamically added
	 * application button for non-favourites apps when it is still running.
	 */
	appInfo=esdashboard_application_button_get_app_info(appButton);
	if(appInfo)
	{
		/* Emit signal */
		g_signal_emit(self, EsdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_REMOVED], 0, appInfo);

		/* Re-add removed favourite as dynamically added application button
		 * for non-favourites apps when it is still running.
		 */
		if(esdashboard_application_tracker_is_running_by_app_info(priv->appTracker, appInfo))
		{
			ClutterActor				*newAppButton;

			newAppButton=_esdashboard_quicklaunch_create_dynamic_actor(self, appInfo);
			clutter_actor_show(newAppButton);
			clutter_actor_add_child(CLUTTER_ACTOR(self), newAppButton);
		}
	}

	/* Destroy favourite icon before updating property */
	esdashboard_actor_destroy(CLUTTER_ACTOR(appButton));

	/* Update favourites from icon order */
	_esdashboard_quicklaunch_update_property_from_icons(self);
}

/* User selected to add application to favourites via pop-up menu */
static void _esdashboard_quicklaunch_on_favourite_popup_menu_item_add_to_favourite(EsdashboardPopupMenuItem *inMenuItem,
																					gpointer inUserData)
{
	EsdashboardApplicationButton		*appButton;
	GAppInfo							*appInfo;
	ClutterActor						*actor;
	EsdashboardQuicklaunch				*self;
	EsdashboardQuicklaunchPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	appButton=ESDASHBOARD_APPLICATION_BUTTON(inUserData);

	/* Find quicklaunch for application button */
	actor=CLUTTER_ACTOR(appButton);
	do
	{
		actor=clutter_actor_get_parent(actor);
	}
	while(actor && !ESDASHBOARD_IS_QUICKLAUNCH(actor));

	if(!actor)
	{
		g_critical("Cannot find quicklaunch for application button.");
		return;
	}

	self=ESDASHBOARD_QUICKLAUNCH(actor);
	priv=self->priv;

	/* Check if application button provides all information needed to add favourite
	 * and also check for duplicates.
	 */
	appInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(appButton));
	if(appInfo &&
		!_esdashboard_quicklaunch_has_favourite_appinfo(self, appInfo))
	{
		ClutterActor					*favouriteActor;

		/* If an actor for current selection to add to favourites already exists,
		 * destroy and remove it regardless if it an actor or a favourite app or
		 * dynamic non-favourite app. It will be re-added later.
		 */
		favouriteActor=_esdashboard_quicklaunch_get_actor_for_appinfo(self, appInfo);
		if(favouriteActor)
		{
			/* Remove existing actor from this quicklaunch */
			esdashboard_actor_destroy(favouriteActor);
		}

		/* Now (re-)add current selection to favourites but hidden as
		 * it will become visible and properly set up when function
		 * _esdashboard_quicklaunch_update_property_from_icons is called.
		 */
		favouriteActor=esdashboard_application_button_new_from_app_info(appInfo);
		clutter_actor_hide(favouriteActor);
		esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(favouriteActor), "favourite-app");
		clutter_actor_insert_child_below(CLUTTER_ACTOR(self), favouriteActor, priv->separatorFavouritesToDynamic);

		/* Update favourites from icon order */
		_esdashboard_quicklaunch_update_property_from_icons(self);

		/* Notify about new favourite */
		esdashboard_notify(CLUTTER_ACTOR(self),
							esdashboard_application_button_get_icon_name(ESDASHBOARD_APPLICATION_BUTTON(favouriteActor)),
							_("Favourite '%s' added"),
							esdashboard_application_button_get_display_name(ESDASHBOARD_APPLICATION_BUTTON(favouriteActor)));

		g_signal_emit(self, EsdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_ADDED], 0, appInfo);
	}
}

/* A right-click might have happened on an application icon (favourite) in quicklaunch */
static void _esdashboard_quicklaunch_on_favourite_popup_menu(EsdashboardQuicklaunch *self,
																ClutterActor *inActor,
																gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate				*priv;
	EsdashboardApplicationButton				*appButton;
	EsdashboardClickAction						*action;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(inUserData));

	priv=self->priv;
	appButton=ESDASHBOARD_APPLICATION_BUTTON(inActor);
	action=ESDASHBOARD_CLICK_ACTION(inUserData);

	/* Check if right button was used when the application button was clicked */
	if(esdashboard_click_action_get_button(action)==ESDASHBOARD_CLICK_ACTION_RIGHT_BUTTON)
	{
		ClutterActor							*popup;
		ClutterActor							*menuItem;
		GAppInfo								*appInfo;

		/* Get app info for application button as it is needed most the time */
		appInfo=esdashboard_application_button_get_app_info(appButton);
		if(!appInfo)
		{
			g_critical("No application information available for clicked application button.");
			return;
		}

		/* Create pop-up menu */
		popup=esdashboard_popup_menu_new_for_source(CLUTTER_ACTOR(self));
		esdashboard_popup_menu_set_destroy_on_cancel(ESDASHBOARD_POPUP_MENU(popup), TRUE);
		esdashboard_popup_menu_set_title(ESDASHBOARD_POPUP_MENU(popup), g_app_info_get_display_name(appInfo));
		esdashboard_popup_menu_set_title_gicon(ESDASHBOARD_POPUP_MENU(popup), g_app_info_get_icon(appInfo));

		/* Add each open window to pop-up of application */
		if(esdashboard_application_button_add_popup_menu_items_for_windows(appButton, ESDASHBOARD_POPUP_MENU(popup))>0)
		{
			/* Add a separator to split windows from other actions in pop-up menu */
			menuItem=esdashboard_popup_menu_item_separator_new();
			clutter_actor_set_x_expand(menuItem, TRUE);
			esdashboard_popup_menu_add_item(ESDASHBOARD_POPUP_MENU(popup), ESDASHBOARD_POPUP_MENU_ITEM(menuItem));
		}

		/* Add menu item to launch application if it is not running */
		if(!esdashboard_application_tracker_is_running_by_app_info(priv->appTracker, appInfo))
		{
			menuItem=esdashboard_popup_menu_item_button_new();
			esdashboard_label_set_text(ESDASHBOARD_LABEL(menuItem), _("Launch"));
			clutter_actor_set_x_expand(menuItem, TRUE);
			esdashboard_popup_menu_add_item(ESDASHBOARD_POPUP_MENU(popup), ESDASHBOARD_POPUP_MENU_ITEM(menuItem));

			g_signal_connect(menuItem,
								"activated",
								G_CALLBACK(_esdashboard_quicklaunch_on_favourite_popup_menu_item_launch),
								appInfo);
		}

		/* Add application actions */
		if(esdashboard_application_button_add_popup_menu_items_for_actions(appButton, ESDASHBOARD_POPUP_MENU(popup))>0)
		{
			/* Add a separator to split windows from other actions in pop-up menu */
			menuItem=esdashboard_popup_menu_item_separator_new();
			clutter_actor_set_x_expand(menuItem, TRUE);
			esdashboard_popup_menu_add_item(ESDASHBOARD_POPUP_MENU(popup), ESDASHBOARD_POPUP_MENU_ITEM(menuItem));
		}

		/* Add "Remove from favourites" if application button is for a favourite application */
		if(esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(appButton), "favourite-app"))
		{
			menuItem=esdashboard_popup_menu_item_button_new();
			esdashboard_label_set_text(ESDASHBOARD_LABEL(menuItem), _("Remove from favourites"));
			clutter_actor_set_x_expand(menuItem, TRUE);
			esdashboard_popup_menu_add_item(ESDASHBOARD_POPUP_MENU(popup), ESDASHBOARD_POPUP_MENU_ITEM(menuItem));

			g_signal_connect(menuItem,
								"activated",
								G_CALLBACK(_esdashboard_quicklaunch_on_favourite_popup_menu_item_remove_from_favourite),
								appButton);
		}

		/* Add "Add to favourites" if application button is for a non-favourite application */
		if(esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(appButton), "dynamic-app"))
		{
			menuItem=esdashboard_popup_menu_item_button_new();
			esdashboard_label_set_text(ESDASHBOARD_LABEL(menuItem), _("Add to favourites"));
			clutter_actor_set_x_expand(menuItem, TRUE);
			esdashboard_popup_menu_add_item(ESDASHBOARD_POPUP_MENU(popup), ESDASHBOARD_POPUP_MENU_ITEM(menuItem));

			g_signal_connect(menuItem,
								"activated",
								G_CALLBACK(_esdashboard_quicklaunch_on_favourite_popup_menu_item_add_to_favourite),
								appButton);
		}

		/* Activate pop-up menu */
		esdashboard_popup_menu_activate(ESDASHBOARD_POPUP_MENU(popup));
	}
}

/* Drag of a quicklaunch icon begins */
static void _esdashboard_quicklaunch_on_favourite_drag_begin(ClutterDragAction *inAction,
																ClutterActor *inActor,
																gfloat inStageX,
																gfloat inStageY,
																ClutterModifierType inModifiers,
																gpointer inUserData)
{
	EsdashboardQuicklaunch			*self;
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*dragHandle;
	ClutterStage					*stage;
	GAppInfo						*appInfo;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(inUserData));

	self=ESDASHBOARD_QUICKLAUNCH(inUserData);
	priv=self->priv;

	/* Prevent signal "clicked" from being emitted on dragged icon */
	g_signal_handlers_block_by_func(inActor, _esdashboard_quicklaunch_on_favourite_clicked, inUserData);

	/* Get stage */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));

	/* Create a clone of application icon for drag handle and hide it
	 * initially. It is only shown if pointer is outside of quicklaunch.
	 */
	appInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(inActor));

	dragHandle=esdashboard_application_button_new_from_app_info(appInfo);
	clutter_actor_set_position(dragHandle, inStageX, inStageY);
	esdashboard_label_set_icon_size(ESDASHBOARD_LABEL(dragHandle), priv->normalIconSize);
	esdashboard_label_set_sync_icon_size(ESDASHBOARD_LABEL(dragHandle), FALSE);
	esdashboard_label_set_style(ESDASHBOARD_LABEL(dragHandle), ESDASHBOARD_LABEL_STYLE_ICON);
	clutter_actor_add_child(CLUTTER_ACTOR(stage), dragHandle);

	clutter_drag_action_set_drag_handle(inAction, dragHandle);
}

/* Drag of a quicklaunch icon ends */
static void _esdashboard_quicklaunch_on_favourite_drag_end(ClutterDragAction *inAction,
															ClutterActor *inActor,
															gfloat inStageX,
															gfloat inStageY,
															ClutterModifierType inModifiers,
															gpointer inUserData)
{
	ClutterActor					*dragHandle;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(inUserData));

	/* Destroy clone of application icon used as drag handle */
	dragHandle=clutter_drag_action_get_drag_handle(inAction);
	if(dragHandle)
	{
#if CLUTTER_CHECK_VERSION(1, 14, 0)
		/* Only unset drag handle if not running Clutter in version
		 * 1.12. This prevents a critical warning message in 1.12.
		 * Later versions of Clutter are fixed already.
		 */
		clutter_drag_action_set_drag_handle(inAction, NULL);
#endif
		esdashboard_actor_destroy(dragHandle);
	}

	/* Allow signal "clicked" from being emitted again */
	g_signal_handlers_unblock_by_func(inActor, _esdashboard_quicklaunch_on_favourite_clicked, inUserData);
}

/* Drag of an actor to quicklaunch as drop target begins */
static gboolean _esdashboard_quicklaunch_on_drop_begin(EsdashboardQuicklaunch *self,
														EsdashboardDragAction *inDragAction,
														gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*dragSource;
	ClutterActor					*draggedActor;
	GAppInfo						*appInfo;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData), FALSE);

	priv=self->priv;

	/* Get source where dragging started and actor being dragged */
	dragSource=esdashboard_drag_action_get_source(inDragAction);
	draggedActor=esdashboard_drag_action_get_actor(inDragAction);

	/* Check if we can handle dragged actor from given source */
	priv->dragMode=DRAG_MODE_NONE;

	if(ESDASHBOARD_IS_QUICKLAUNCH(dragSource) &&
		ESDASHBOARD_IS_APPLICATION_BUTTON(draggedActor) &&
		esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(draggedActor)))
	{
		priv->dragMode=DRAG_MODE_MOVE_EXISTING;
	}

	if(!ESDASHBOARD_IS_QUICKLAUNCH(dragSource) &&
		ESDASHBOARD_IS_APPLICATION_BUTTON(draggedActor) &&
		esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(draggedActor)))
	{
		gboolean					isExistingItem;

		isExistingItem=FALSE;

		/* Get application information of item which should be added */
		appInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(draggedActor));
		if(appInfo)
		{
			isExistingItem=_esdashboard_quicklaunch_has_favourite_appinfo(self, appInfo);
			if(!isExistingItem) priv->dragMode=DRAG_MODE_CREATE;
		}
	}

	/* Create a visible copy of dragged application button and insert it
	 * after dragged icon in quicklaunch. This one is the one which is
	 * moved within quicklaunch. It is used as preview how quicklaunch
	 * will look like if drop will be successful. It is also needed to
	 * restore original order of all favourite icons if drag was
	 * cancelled by just destroying this preview icon.
	 * If dragging an favourite icon we will hide the it and leave it
	 * untouched when drag is cancelled.
	 */
	if(priv->dragMode!=DRAG_MODE_NONE)
	{
		appInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(draggedActor));

		priv->dragPreviewIcon=esdashboard_application_button_new_from_app_info(appInfo);
		esdashboard_label_set_icon_size(ESDASHBOARD_LABEL(priv->dragPreviewIcon), priv->normalIconSize);
		esdashboard_label_set_sync_icon_size(ESDASHBOARD_LABEL(priv->dragPreviewIcon), FALSE);
		esdashboard_label_set_style(ESDASHBOARD_LABEL(priv->dragPreviewIcon), ESDASHBOARD_LABEL_STYLE_ICON);
		if(priv->dragMode==DRAG_MODE_CREATE) clutter_actor_hide(priv->dragPreviewIcon);
		clutter_actor_add_child(CLUTTER_ACTOR(self), priv->dragPreviewIcon);

		if(priv->dragMode==DRAG_MODE_MOVE_EXISTING)
		{
			clutter_actor_set_child_below_sibling(CLUTTER_ACTOR(self),
													priv->dragPreviewIcon,
													draggedActor);
			clutter_actor_hide(draggedActor);
		}
	}

	/* Hide all dynamically added application button for non-favourite apps */
	if(priv->dragMode!=DRAG_MODE_NONE)
	{
		ClutterActorIter			iter;
		ClutterActor				*child;

		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only check application buttons */
			if(!ESDASHBOARD_IS_APPLICATION_BUTTON(child)) continue;

			/* If actor is an application button for non-favourite apps,
			 * hide it now.
			 */
			if(esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(child), "dynamic-app"))
			{
				clutter_actor_hide(child);
			}
		}
	}

	/* If drag mode is set return TRUE because we can handle dragged actor
	 * otherwise return FALSE
	 */
	return(priv->dragMode!=DRAG_MODE_NONE ? TRUE : FALSE);
}

/* Dragged actor was dropped on this drop target */
static void _esdashboard_quicklaunch_on_drop_drop(EsdashboardQuicklaunch *self,
													EsdashboardDragAction *inDragAction,
													gfloat inX,
													gfloat inY,
													gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate		*priv;
	ClutterActor						*draggedActor;
	ClutterActorIter					iter;
	ClutterActor						*child;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get dragged actor */
	draggedActor=esdashboard_drag_action_get_actor(inDragAction);

	/* Remove dynamically added non-favourite application buttons and
	 * emit signal when a favourite icon was added.
	 */
	if(priv->dragMode==DRAG_MODE_CREATE)
	{
		GAppInfo						*appInfo;
		ClutterActor					*actor;

		esdashboard_notify(CLUTTER_ACTOR(self),
							esdashboard_application_button_get_icon_name(ESDASHBOARD_APPLICATION_BUTTON(draggedActor)),
							_("Favourite '%s' added"),
							esdashboard_application_button_get_display_name(ESDASHBOARD_APPLICATION_BUTTON(draggedActor)));

		appInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(draggedActor));
		if(appInfo)
		{
			/* Remove any application button marked as dynamically added for non-favourite
			 * apps for the newly added favourite if available.
			 */
			actor=_esdashboard_quicklaunch_get_actor_for_appinfo(self, appInfo);
			if(actor) esdashboard_actor_destroy(actor);

			/* Emit signal for newly added favourite */
			g_signal_emit(self, EsdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_ADDED], 0, appInfo);
		}

		/* Set CSS class for favourite to get it included when property is updated
		 * in function _esdashboard_quicklaunch_update_property_from_icons.
		 */
		esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(priv->dragPreviewIcon), "favourite-app");
	}

	/* If drag mode is reorder move originally dragged application icon
	 * to its final position and destroy preview application icon.
	 */
	if(priv->dragMode==DRAG_MODE_MOVE_EXISTING)
	{
		clutter_actor_set_child_below_sibling(CLUTTER_ACTOR(self),
												draggedActor,
												priv->dragPreviewIcon);
		clutter_actor_show(draggedActor);

		if(priv->dragPreviewIcon)
		{
			esdashboard_actor_destroy(priv->dragPreviewIcon);
			priv->dragPreviewIcon=NULL;
		}
	}

	/* Show (remaining) hidden application buttons for non-favourite apps again */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check application buttons */
		if(!ESDASHBOARD_IS_APPLICATION_BUTTON(child)) continue;

		/* If actor is an application button for non-favourite apps,
		 * show it now.
		 */
		if(esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(child), "dynamic-app"))
		{
			clutter_actor_show(child);
		}
	}

	/* Update favourites from icon order */
	_esdashboard_quicklaunch_update_property_from_icons(self);

	/* Reset drag mode */
	priv->dragMode=DRAG_MODE_NONE;
}

/* Drag of an actor to this drop target ended without actor being dropped here */
static void _esdashboard_quicklaunch_on_drop_end(EsdashboardQuicklaunch *self,
													EsdashboardDragAction *inDragAction,
													gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate		*priv;
	ClutterActor						*draggedActor;
	ClutterActorIter					iter;
	ClutterActor						*child;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get dragged actor */
	draggedActor=esdashboard_drag_action_get_actor(inDragAction);

	/* Show hidden application buttons for non-favourite apps again */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check application buttons */
		if(!ESDASHBOARD_IS_APPLICATION_BUTTON(child)) continue;

		/* If actor is an application button for non-favourite apps,
		 * show it now.
		 */
		if(esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(child), "dynamic-app"))
		{
			clutter_actor_show(child);
		}
	}

	/* Destroy preview icon and show originally dragged favourite icon.
	 * Doing it this way will restore the order of favourite icons.
	 */
	if(priv->dragPreviewIcon)
	{
		esdashboard_actor_destroy(priv->dragPreviewIcon);
		priv->dragPreviewIcon=NULL;
	}

	if(priv->dragMode==DRAG_MODE_MOVE_EXISTING) clutter_actor_show(draggedActor);
	priv->dragMode=DRAG_MODE_NONE;
}

/* Drag of an actor entered this drop target */
static void _esdashboard_quicklaunch_on_drop_enter(EsdashboardQuicklaunch *self,
													EsdashboardDragAction *inDragAction,
													gpointer inUserData)
{
	ClutterActor					*dragHandle;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	/* Get source where dragging started and actor being dragged */
	dragHandle=clutter_drag_action_get_drag_handle(CLUTTER_DRAG_ACTION(inDragAction));

	/* Hide drag handle in this quicklaunch */
	clutter_actor_hide(dragHandle);
}

/* Drag of an actor moves over this drop target */
static void _esdashboard_quicklaunch_on_drop_motion(EsdashboardQuicklaunch *self,
													EsdashboardDragAction *inDragAction,
													gfloat inX,
													gfloat inY,
													gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*draggedActor;
	ClutterActor					*dragHandle;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get actor being dragged and the drag handle */
	draggedActor=esdashboard_drag_action_get_actor(inDragAction);
	dragHandle=clutter_drag_action_get_drag_handle(CLUTTER_DRAG_ACTION(inDragAction));

	/* Get new position of preview application icon in quicklaunch
	 * if dragged actor is an application icon
	 */
	if(ESDASHBOARD_IS_APPLICATION_BUTTON(draggedActor))
	{
		ClutterStage				*stage;
		gboolean					oldPreviewReactive;
		gboolean					oldHandleReactive;
		gfloat						stageX, stageY;
		gfloat						deltaX, deltaY;
		ClutterActor				*actorUnderMouse;
		ClutterActorIter			iter;
		ClutterActor				*iterChild;

		/* Preview icon and drag handle should not be reactive to prevent
		 * clutter_stage_get_actor_at_pos() choosing one of both as the
		 * actor under mouse. But remember their state to reset it later.
		 */
		oldPreviewReactive=clutter_actor_get_reactive(priv->dragPreviewIcon);
		clutter_actor_set_reactive(priv->dragPreviewIcon, FALSE);

		oldHandleReactive=clutter_actor_get_reactive(dragHandle);
		clutter_actor_set_reactive(dragHandle, FALSE);

		/* Get new position and move preview icon */
		clutter_drag_action_get_motion_coords(CLUTTER_DRAG_ACTION(inDragAction), &stageX, &stageY);
		esdashboard_drag_action_get_motion_delta(inDragAction, &deltaX, &deltaY);

		stage=CLUTTER_STAGE(clutter_actor_get_stage(dragHandle));
		actorUnderMouse=clutter_stage_get_actor_at_pos(stage, CLUTTER_PICK_REACTIVE, stageX, stageY);
		if(ESDASHBOARD_IS_APPLICATION_BUTTON(actorUnderMouse) &&
			actorUnderMouse!=priv->dragPreviewIcon)
		{
			if((priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL && deltaX<0) ||
				(priv->orientation==CLUTTER_ORIENTATION_VERTICAL && deltaY<0))
			{
				clutter_actor_set_child_below_sibling(CLUTTER_ACTOR(self),
														priv->dragPreviewIcon,
														actorUnderMouse);
			}
				else
				{
					clutter_actor_set_child_above_sibling(CLUTTER_ACTOR(self),
															priv->dragPreviewIcon,
															actorUnderMouse);
				}

			/* Show preview icon now if drag mode is "new". Doing it earlier will
			 * show preview icon at wrong position when entering quicklaunch
			 */
			if(priv->dragMode==DRAG_MODE_CREATE) clutter_actor_show(priv->dragPreviewIcon);

			/* Iterate through list of current actors and enable allocation animation */
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
			if((priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL && deltaX<0) ||
				(priv->orientation==CLUTTER_ORIENTATION_VERTICAL && deltaY<0))
			{
				while(clutter_actor_iter_next(&iter, &iterChild) && iterChild!=priv->dragPreviewIcon)
				{
					if(ESDASHBOARD_IS_ACTOR(iterChild))
					{
						esdashboard_actor_enable_allocation_animation_once(ESDASHBOARD_ACTOR(iterChild));
					}
				}
			}
				else
				{
					while(clutter_actor_iter_next(&iter, &iterChild) && iterChild!=priv->dragPreviewIcon);
					while(clutter_actor_iter_next(&iter, &iterChild))
					{
						if(ESDASHBOARD_IS_ACTOR(iterChild))
						{
							esdashboard_actor_enable_allocation_animation_once(ESDASHBOARD_ACTOR(iterChild));
						}
					}
				}
		}

		/* Reset reactive state of preview icon and drag handle */
		clutter_actor_set_reactive(priv->dragPreviewIcon, oldPreviewReactive);
		clutter_actor_set_reactive(dragHandle, oldHandleReactive);
	}
}

/* Drag of an actor entered this drop target */
static void _esdashboard_quicklaunch_on_drop_leave(EsdashboardQuicklaunch *self,
													EsdashboardDragAction *inDragAction,
													gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*dragHandle;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get source where dragging started and drag handle */
	dragHandle=clutter_drag_action_get_drag_handle(CLUTTER_DRAG_ACTION(inDragAction));

	/* Show drag handle outside this quicklaunch */
	clutter_actor_show(dragHandle);

	/* Hide preview icon if drag mode is "create new" favourite */
	if(priv->dragMode==DRAG_MODE_CREATE) clutter_actor_hide(priv->dragPreviewIcon);
}

/* Drag of an actor to trash drop target begins */
static gboolean _esdashboard_quicklaunch_on_trash_drop_begin(EsdashboardQuicklaunch *self,
																EsdashboardDragAction *inDragAction,
																gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*dragSource;
	ClutterActor					*draggedActor;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData), FALSE);

	priv=self->priv;

	/* Get source where dragging started and actor being dragged */
	dragSource=esdashboard_drag_action_get_source(inDragAction);
	draggedActor=esdashboard_drag_action_get_actor(inDragAction);

	/* Check if we can handle dragged actor from given source */
	if(ESDASHBOARD_IS_QUICKLAUNCH(dragSource) &&
		ESDASHBOARD_IS_APPLICATION_BUTTON(draggedActor))
	{
		/* Dragged actor is a favourite icon from quicklaunch. So hide
		 * "applications" button and show an unhighlited trash button instead.
		 * Trash button will be hidden and "applications" button shown again
		 * when drag ends.
		 */
		clutter_actor_hide(priv->appsButton);
		clutter_actor_show(priv->trashButton);

		return(TRUE);
	}

	/* If we get here we cannot handle dragged actor at trash drop target */
	return(FALSE);
}

/* Dragged actor was dropped on trash drop target */
static void _esdashboard_quicklaunch_on_trash_drop_drop(EsdashboardQuicklaunch *self,
															EsdashboardDragAction *inDragAction,
															gfloat inX,
															gfloat inY,
															gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*draggedActor;
	GAppInfo						*appInfo;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get dragged favourite icon */
	draggedActor=esdashboard_drag_action_get_actor(inDragAction);

	/* Notify about removal of favourite icon */
	esdashboard_notify(CLUTTER_ACTOR(self),
						esdashboard_application_button_get_icon_name(ESDASHBOARD_APPLICATION_BUTTON(draggedActor)),
						_("Favourite '%s' removed"),
						esdashboard_application_button_get_display_name(ESDASHBOARD_APPLICATION_BUTTON(draggedActor)));

	/* Emit signal and re-add removed favourite as dynamically added
	 * application button for non-favourites apps when it is still running.
	 */
	appInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(draggedActor));
	if(appInfo)
	{
		/* Emit signal */
		g_signal_emit(self, EsdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_REMOVED], 0, appInfo);

		/* Re-add removed favourite as dynamically added application button
		 * for non-favourites apps when it is still running.
		 */
		if(esdashboard_application_tracker_is_running_by_app_info(priv->appTracker, appInfo))
		{
			ClutterActor			*actor;

			actor=_esdashboard_quicklaunch_create_dynamic_actor(self, appInfo);
			clutter_actor_show(actor);
			clutter_actor_add_child(CLUTTER_ACTOR(self), actor);
		}
	}

	/* Destroy dragged favourite icon before updating property */
	esdashboard_actor_destroy(draggedActor);

	/* Destroy preview icon before updating property */
	if(priv->dragPreviewIcon)
	{
		esdashboard_actor_destroy(priv->dragPreviewIcon);
		priv->dragPreviewIcon=NULL;
	}

	/* Show "applications" button again and hide trash button instead */
	clutter_actor_hide(priv->trashButton);
	clutter_actor_show(priv->appsButton);

	/* Update favourites from icon order */
	_esdashboard_quicklaunch_update_property_from_icons(self);

	/* Reset drag mode */
	priv->dragMode=DRAG_MODE_NONE;
}

/* Drag of an actor to trash drop target ended without actor being dropped here */
static void _esdashboard_quicklaunch_on_trash_drop_end(EsdashboardQuicklaunch *self,
														EsdashboardDragAction *inDragAction,
														gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Show "applications" button again and hide trash button instead */
	clutter_actor_hide(priv->trashButton);
	clutter_actor_show(priv->appsButton);
}

/* Drag of an actor entered trash drop target */
static void _esdashboard_quicklaunch_on_trash_drop_enter(EsdashboardQuicklaunch *self,
															EsdashboardDragAction *inDragAction,
															gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Set toggle state on trash drop target as dragged actor is over it */
	esdashboard_toggle_button_set_toggle_state(ESDASHBOARD_TOGGLE_BUTTON(priv->trashButton), TRUE);
}

/* Drag of an actor leaves trash drop target */
static void _esdashboard_quicklaunch_on_trash_drop_leave(EsdashboardQuicklaunch *self,
															EsdashboardDragAction *inDragAction,
															gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(ESDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Set toggle state on trash drop target as dragged actor is over it */
	esdashboard_toggle_button_set_toggle_state(ESDASHBOARD_TOGGLE_BUTTON(priv->trashButton), FALSE);
}

/* A tooltip for a favourite will be activated */
static void _esdashboard_quicklaunch_on_tooltip_activating(ClutterAction *inAction, gpointer inUserData)
{
	EsdashboardTooltipAction		*action;
	EsdashboardApplicationButton	*button;

	g_return_if_fail(ESDASHBOARD_IS_TOOLTIP_ACTION(inAction));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	action=ESDASHBOARD_TOOLTIP_ACTION(inAction);
	button=ESDASHBOARD_APPLICATION_BUTTON(inUserData);

	/* Update tooltip text to reflect favourites current display name */
	esdashboard_tooltip_action_set_text(action, esdashboard_application_button_get_display_name(button));
}

/* Create actor for a dynamically added non-favourite application */
static ClutterActor* _esdashboard_quicklaunch_create_dynamic_actor(EsdashboardQuicklaunch *self,
																	GAppInfo *inAppInfo)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*actor;
	ClutterAction					*action;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), NULL);
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), NULL);

	priv=self->priv;

	/* Create and set up actor */
	actor=esdashboard_application_button_new_from_app_info(inAppInfo);
	esdashboard_label_set_icon_size(ESDASHBOARD_LABEL(actor), priv->normalIconSize);
	esdashboard_label_set_sync_icon_size(ESDASHBOARD_LABEL(actor), FALSE);
	esdashboard_label_set_style(ESDASHBOARD_LABEL(actor), ESDASHBOARD_LABEL_STYLE_ICON);
	esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(actor), "dynamic-app");

	/* Set up and add click action */
	g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_esdashboard_quicklaunch_on_favourite_clicked), self);

	/* Set up and add pop-up menu click action */
	action=esdashboard_click_action_new();
	g_signal_connect_swapped(action, "clicked", G_CALLBACK(_esdashboard_quicklaunch_on_favourite_popup_menu), self);
	clutter_actor_add_action(actor, action);

	/* Set up and add tooltip action */
	action=esdashboard_tooltip_action_new();
	g_signal_connect(action, "activating", G_CALLBACK(_esdashboard_quicklaunch_on_tooltip_activating), actor);
	clutter_actor_add_action(actor, action);

	/* Return newly created actor */
	return(actor);
}

/* Create actor for a favourite application */
static ClutterActor* _esdashboard_quicklaunch_create_favourite_actor(EsdashboardQuicklaunch *self,
																		GAppInfo *inAppInfo)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*actor;
	ClutterAction					*action;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), NULL);
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), NULL);

	priv=self->priv;

	/* Create and set up actor */
	actor=esdashboard_application_button_new_from_app_info(inAppInfo);
	esdashboard_label_set_icon_size(ESDASHBOARD_LABEL(actor), priv->normalIconSize);
	esdashboard_label_set_sync_icon_size(ESDASHBOARD_LABEL(actor), FALSE);
	esdashboard_label_set_style(ESDASHBOARD_LABEL(actor), ESDASHBOARD_LABEL_STYLE_ICON);
	esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(actor), "favourite-app");

	/* Set up and add click action */
	g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_esdashboard_quicklaunch_on_favourite_clicked), self);

	/* Set up and add pop-up menu click action */
	action=esdashboard_click_action_new();
	g_signal_connect_swapped(action, "clicked", G_CALLBACK(_esdashboard_quicklaunch_on_favourite_popup_menu), self);
	clutter_actor_add_action(actor, action);

	/* Set up and add drag'n'drop action */
	action=esdashboard_drag_action_new_with_source(CLUTTER_ACTOR(self));
	clutter_drag_action_set_drag_threshold(CLUTTER_DRAG_ACTION(action), -1, -1);
	clutter_actor_add_action(actor, action);
	g_signal_connect(action, "drag-begin", G_CALLBACK(_esdashboard_quicklaunch_on_favourite_drag_begin), self);
	g_signal_connect(action, "drag-end", G_CALLBACK(_esdashboard_quicklaunch_on_favourite_drag_end), self);

	/* Set up and add tooltip action */
	action=esdashboard_tooltip_action_new();
	g_signal_connect(action, "activating", G_CALLBACK(_esdashboard_quicklaunch_on_tooltip_activating), actor);
	clutter_actor_add_action(actor, action);

	/* Return newly created actor */
	return(actor);
}

/* Update property from icons in quicklaunch */
static void _esdashboard_quicklaunch_update_property_from_icons(EsdashboardQuicklaunch *self)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	GAppInfo						*desktopAppInfo;
	gchar							*desktopFile;
	GValue							*desktopValue;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));

	priv=self->priv;

	/* Free current list of desktop files */
	if(priv->favourites)
	{
		esconf_array_free(priv->favourites);
		priv->favourites=NULL;
	}

	/* Create array of strings pointing to desktop files for new order,
	 * update quicklaunch and store settings
	 */
	priv->favourites=g_ptr_array_new();

	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		desktopFile=NULL;

		/* Only add desktop file if it is an application button for
		 * a favourite and provides a desktop ID or desktop file name
		 * and is not going to be destroyed
		 */
 		if(!ESDASHBOARD_IS_APPLICATION_BUTTON(child)) continue;
		if(esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(child), "destroying")) continue;
		if(!esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(child), "favourite-app")) continue;

		desktopAppInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(child));
		if(desktopAppInfo &&
			ESDASHBOARD_IS_DESKTOP_APP_INFO(desktopAppInfo))
		{
			desktopFile=g_strdup(g_app_info_get_id(desktopAppInfo));
			if(!desktopFile)
			{
				GFile				*file;

				file=esdashboard_desktop_app_info_get_file(ESDASHBOARD_DESKTOP_APP_INFO(desktopAppInfo));
				if(file) desktopFile=g_file_get_path(file);
			}
		}
		if(!desktopFile) continue;

		/* Add desktop file name to array */
		desktopValue=g_value_init(g_new0(GValue, 1), G_TYPE_STRING);
		g_value_set_string(desktopValue, desktopFile);
		g_ptr_array_add(priv->favourites, desktopValue);

		/* Release allocated resources */
		g_free(desktopFile);
	}

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardQuicklaunchProperties[PROP_FAVOURITES]);
}

/* Update icons in quicklaunch from property */
static void _esdashboard_quicklaunch_update_icons_from_property(EsdashboardQuicklaunch *self)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	guint							i;
	ClutterActor					*actor;
	GValue							*desktopFile;
	GAppInfo						*currentSelectionAppInfo;
	const gchar						*desktopFilename;
	GAppInfo						*appInfo;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));

	priv=self->priv;
	currentSelectionAppInfo=NULL;

	/* If current selection is an application button then remember it
	 * to reselect it after favourites are re-setup.
	 */
	if(priv->selectedItem &&
		ESDASHBOARD_IS_APPLICATION_BUTTON(priv->selectedItem))
	{
		currentSelectionAppInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(priv->selectedItem));

		ESDASHBOARD_DEBUG(self, ACTOR,
							"Going to destroy current selection %p (%s) for desktop ID '%s'",
							priv->selectedItem, G_OBJECT_TYPE_NAME(priv->selectedItem),
							g_app_info_get_id(esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(priv->selectedItem))));
	}

	/* Remove all application buttons */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(ESDASHBOARD_IS_APPLICATION_BUTTON(child) &&
			esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(child), "favourite-app"))
		{
			clutter_actor_iter_destroy(&iter);
		}
	}

	/* Now re-add all application icons for current favourites */
	for(i=0; i<priv->favourites->len; i++)
	{
		/* Get desktop file to create application button for in quicklaunch */
		desktopFile=(GValue*)g_ptr_array_index(priv->favourites, i);

		desktopFilename=g_value_get_string(desktopFile);
		if(g_path_is_absolute(desktopFilename)) appInfo=esdashboard_desktop_app_info_new_from_path(desktopFilename);
			else
			{
				appInfo=esdashboard_application_database_lookup_desktop_id(priv->appDB, desktopFilename);
				if(!appInfo) appInfo=esdashboard_desktop_app_info_new_from_desktop_id(desktopFilename);
			}

		/* If we could not get application information for desktop file, do not
		 * create the application button.
		 */
		if(!appInfo) continue;

		/* Create application button from desktop file */
		actor=_esdashboard_quicklaunch_create_favourite_actor(self, appInfo);
		clutter_actor_show(actor);
		clutter_actor_insert_child_below(CLUTTER_ACTOR(self), actor, priv->separatorFavouritesToDynamic);

		/* Select this item if it matches the previously selected item
		 * which was destroyed in the meantime.
		 */
		if(currentSelectionAppInfo)
		{
			/* Check if newly created application button matches current selection
			 * then reselect newly create actor as current selection.
			 */
			if(g_app_info_equal(G_APP_INFO(appInfo), currentSelectionAppInfo))
			{
				esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), actor);

				ESDASHBOARD_DEBUG(self, ACTOR,
									"Select newly created actor %p (%s) because it matches desktop ID '%s'",
									actor, G_OBJECT_TYPE_NAME(actor),
									g_app_info_get_id(esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(actor))));
			}
		}

		/* Release allocated resources */
		g_object_unref(appInfo);
	}
}

/* Set up favourites array from string array value */
static void _esdashboard_quicklaunch_set_favourites(EsdashboardQuicklaunch *self, const GValue *inValue)
{
	EsdashboardQuicklaunchPrivate	*priv;
	GPtrArray						*desktopFiles;
	guint							i;
	GValue							*element;
	GValue							*desktopFile;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(G_IS_VALUE(inValue));

	priv=self->priv;

	/* Free current list of favourites */
	if(priv->favourites)
	{
		esconf_array_free(priv->favourites);
		priv->favourites=NULL;
	}

	/* Copy array of string pointing to desktop files */
	desktopFiles=g_value_get_boxed(inValue);
	if(desktopFiles)
	{
		priv->favourites=g_ptr_array_sized_new(desktopFiles->len);
		for(i=0; i<desktopFiles->len; ++i)
		{
			element=(GValue*)g_ptr_array_index(desktopFiles, i);

			/* Filter string value types */
			if(G_VALUE_HOLDS_STRING(element))
			{
				desktopFile=g_value_init(g_new0(GValue, 1), G_TYPE_STRING);
				g_value_copy(element, desktopFile);
				g_ptr_array_add(priv->favourites, desktopFile);
			}
		}
	}
		else priv->favourites=g_ptr_array_new();

	/* Update list of icons for desktop files */
	_esdashboard_quicklaunch_update_icons_from_property(self);
}

/* Set up default favourites (e.g. used when started for the very first time) */
static void _esdashboard_quicklaunch_setup_default_favourites(EsdashboardQuicklaunch *self)
{
	EsdashboardQuicklaunchPrivate	*priv;
	guint							i;
	const gchar						*defaultApplications[]=	{
																"exo-web-browser.desktop",
																"exo-mail-reader.desktop",
																"exo-file-manager.desktop",
																"exo-terminal-emulator.desktop",
															};

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));

	priv=self->priv;

	/* Free current list of favourites */
	if(priv->favourites)
	{
		esconf_array_free(priv->favourites);
		priv->favourites=NULL;
	}

	/* Build array with each available default application */
	priv->favourites=g_ptr_array_new();
	for(i=0; i<(sizeof(defaultApplications)/sizeof(defaultApplications[0])); i++)
	{
		GAppInfo					*appInfo;
		GValue						*desktopFile;

		appInfo=esdashboard_application_database_lookup_desktop_id(priv->appDB, defaultApplications[i]);
		if(!appInfo) appInfo=esdashboard_desktop_app_info_new_from_desktop_id(defaultApplications[i]);

		if(appInfo)
		{
			/* Add desktop file to array */
			desktopFile=g_value_init(g_new0(GValue, 1), G_TYPE_STRING);
			g_value_set_string(desktopFile, defaultApplications[i]);
			g_ptr_array_add(priv->favourites, desktopFile);

			/* Release allocated resources */
			g_object_unref(appInfo);
		}
	}

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), EsdashboardQuicklaunchProperties[PROP_FAVOURITES]);
}

/* Get scale factor to fit all children into given width */
static gfloat _esdashboard_quicklaunch_get_scale_for_width(EsdashboardQuicklaunch *self,
															gfloat inForWidth,
															gboolean inDoMinimumSize)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gint							numberChildren;
	gfloat							totalWidth, scalableWidth;
	gfloat							childWidth;
	gfloat							childMinWidth, childNaturalWidth;
	gfloat							scale;
	gboolean						recheckWidth;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);
	g_return_val_if_fail(inForWidth>=0.0f, 0.0f);

	priv=self->priv;

	/* Count visible children and determine their total width */
	numberChildren=0;
	totalWidth=0.0f;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!clutter_actor_is_visible(child)) continue;

		/* Get width of child */
		clutter_actor_get_preferred_width(child, -1, &childMinWidth, &childNaturalWidth);
		if(inDoMinimumSize==TRUE) childWidth=childMinWidth;
			else childWidth=childNaturalWidth;

		/* Determine total size so far */
		totalWidth+=ceil(childWidth);

		/* Count visible children */
		numberChildren++;
	}
	if(numberChildren==0) return(priv->scaleMax);

	/* Determine scalable width. That is the width without spacing
	 * between children and the spacing used as padding.
	 */
	scalableWidth=inForWidth-((numberChildren+1)*priv->spacing);

	/* Get scale factor */
	scale=priv->scaleMax;
	if(totalWidth>0.0f)
	{
		scale=floor((scalableWidth/totalWidth)/priv->scaleStep)*priv->scaleStep;
		scale=MIN(scale, priv->scaleMax);
		scale=MAX(scale, priv->scaleMin);
	}

	/* Check if all visible children would really fit into width
	 * otherwise we need to decrease scale factor one step down
	 */
	if(scale>priv->scaleMin)
	{
		do
		{
			recheckWidth=FALSE;
			totalWidth=priv->spacing;

			/* Iterate through visible children and sum their scaled
			 * widths. The total width will be initialized with unscaled
			 * spacing and all visible children's scaled width will also
			 * be added with unscaled spacing to have the padding added.
			 */
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!clutter_actor_is_visible(child)) continue;

				/* Get scaled size of child and add to total width */
				clutter_actor_get_preferred_width(child, -1, &childMinWidth, &childNaturalWidth);
				if(inDoMinimumSize==TRUE) childWidth=childMinWidth;
					else childWidth=childNaturalWidth;

				childWidth*=scale;
				totalWidth+=ceil(childWidth)+priv->spacing;
			}

			/* If total width is greater than given width
			 * decrease scale factor by one step and recheck
			 */
			if(totalWidth>inForWidth && scale>priv->scaleMin)
			{
				scale-=priv->scaleStep;
				recheckWidth=TRUE;
			}
		}
		while(recheckWidth==TRUE);
	}

	/* Return found scale factor */
	return(scale);
}

/* Get scale factor to fit all children into given height */
static gfloat _esdashboard_quicklaunch_get_scale_for_height(EsdashboardQuicklaunch *self,
															gfloat inForHeight,
															gboolean inDoMinimumSize)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gint							numberChildren;
	gfloat							totalHeight, scalableHeight;
	gfloat							childHeight;
	gfloat							childMinHeight, childNaturalHeight;
	gfloat							scale;
	gboolean						recheckHeight;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);
	g_return_val_if_fail(inForHeight>=0.0f, 0.0f);

	priv=self->priv;

	/* Count visible children and determine their total height */
	numberChildren=0;
	totalHeight=0.0f;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!clutter_actor_is_visible(child)) continue;

		/* Get height of child */
		clutter_actor_get_preferred_height(child, -1, &childMinHeight, &childNaturalHeight);
		if(inDoMinimumSize==TRUE) childHeight=childMinHeight;
			else childHeight=childNaturalHeight;

		/* Determine total size so far */
		totalHeight+=ceil(childHeight);

		/* Count visible children */
		numberChildren++;
	}
	if(numberChildren==0) return(priv->scaleMax);

	/* Determine scalable height. That is the height without spacing
	 * between children and the spacing used as padding.
	 */
	scalableHeight=inForHeight-((numberChildren+1)*priv->spacing);

	/* Get scale factor */
	scale=priv->scaleMax;
	if(totalHeight>0.0f)
	{
		scale=floor((scalableHeight/totalHeight)/priv->scaleStep)*priv->scaleStep;
		scale=MIN(scale, priv->scaleMax);
		scale=MAX(scale, priv->scaleMin);
	}

	/* Check if all visible children would really fit into height
	 * otherwise we need to decrease scale factor one step down
	 */
	if(scale>priv->scaleMin)
	{
		do
		{
			recheckHeight=FALSE;
			totalHeight=priv->spacing;

			/* Iterate through visible children and sum their scaled
			 * heights. The total height will be initialized with unscaled
			 * spacing and all visible children's scaled height will also
			 * be added with unscaled spacing to have the padding added.
			 */
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!clutter_actor_is_visible(child)) continue;

				/* Get scaled size of child and add to total height */
				clutter_actor_get_preferred_height(child, -1, &childMinHeight, &childNaturalHeight);
				if(inDoMinimumSize==TRUE) childHeight=childMinHeight;
					else childHeight=childNaturalHeight;

				childHeight*=scale;
				totalHeight+=ceil(childHeight)+priv->spacing;
			}

			/* If total height is greater than given height
			 * decrease scale factor by one step and recheck
			 */
			if(totalHeight>inForHeight && scale>priv->scaleMin)
			{
				scale-=priv->scaleStep;
				recheckHeight=TRUE;
			}
		}
		while(recheckHeight==TRUE);
	}

	/* Return found scale factor */
	return(scale);
}

/* Get previous selectable actor in quicklaunch */
static ClutterActor* esdashboard_quicklaunch_get_previous_selectable(EsdashboardQuicklaunch *self,
																		ClutterActor *inSelected)
{
	ClutterActorIter	iter;
	ClutterActor		*child;
	ClutterActor		*prevItem;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), NULL);

	prevItem=NULL;

	/* Iterate through children and return previous selectable item after given one */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* If this child is the lookup one return previous visible item seen */
		if(child==inSelected && prevItem) return(prevItem);

		/* If this child is visible but not the one we lookup remember it as previous one */
		if(clutter_actor_is_visible(child)) prevItem=child;
	}

	/* If we get here there is no selectable item after given one, so return last
	 * selectable item we have seen.
	 */
	return(prevItem);
}

/* Get next selectable actor in quicklaunch */
static ClutterActor* esdashboard_quicklaunch_get_next_selectable(EsdashboardQuicklaunch *self,
																	ClutterActor *inSelected)
{
	ClutterActorIter	iter;
	ClutterActor		*child;
	gboolean			doLookup;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), NULL);

	/* Iterate through children beginnig at current selected one and return next
	 * selectable item after that one.
	 */
	doLookup=FALSE;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* If this child is the lookup one, lookup next visible item */
		if(child!=inSelected && doLookup==FALSE) continue;

		/* Return child if visible */
		if(doLookup && clutter_actor_is_visible(child)) return(child);

		/* If we get here we either found current selected one and we should
		 * look for next selectable item or we looked for next selectable item
		 * but it was not selectable. In both cases set flag to lookup for
		 * selectable items.
		 */
		doLookup=TRUE;
	}

	/* If we get here we are at the end of list of children and no one was selectable.
	 * Start over at the beginning of list of children up to the current selected one.
	 * Return the first visible item.
	 */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Stop at current selected one */
		if(child==inSelected) break;

		/* Return this child if visible */
		if(clutter_actor_is_visible(child)) return(child);
	}

	/* If we get here there is no selectable item was found, so return NULL */
	return(NULL);
}

/* Action signal to add current selected item as favourite was emitted */
static gboolean _esdashboard_quicklaunch_selection_add_favourite(EsdashboardQuicklaunch *self,
																	EsdashboardFocusable *inSource,
																	const gchar *inAction,
																	ClutterEvent *inEvent)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*currentSelection;
	GAppInfo						*appInfo;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inSource), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inEvent, CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Get current selection of focusable actor and check if it is an actor
	 * derived from EsdashboardApplicationButton.
	 */
	currentSelection=esdashboard_focusable_get_selection(inSource);
	if(!currentSelection)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Source actor %s has no selection and no favourite can be added.",
							G_OBJECT_TYPE_NAME(inSource));
		return(CLUTTER_EVENT_STOP);
	}

	if(!ESDASHBOARD_IS_APPLICATION_BUTTON(currentSelection))
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Current selection at source actor %s has type %s but only selections of type %s can be added.",
							G_OBJECT_TYPE_NAME(inSource),
							G_OBJECT_TYPE_NAME(currentSelection),
							g_type_name(ESDASHBOARD_TYPE_APPLICATION_BUTTON));
		return(CLUTTER_EVENT_STOP);
	}

	/* Check if application button provides all information needed to add favourite
	 * and also check for duplicates.
	 */
	appInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(currentSelection));
	if(appInfo &&
		!_esdashboard_quicklaunch_has_favourite_appinfo(self, appInfo))
	{
		ClutterActor		*favouriteActor;

		/* If an actor for current selection to add to favourites already exists,
		 * destroy and remove it regardless if it an actor or a favourite app or
		 * dynamic non-favourite app. It will be re-added later.
		 */
		favouriteActor=_esdashboard_quicklaunch_get_actor_for_appinfo(self, appInfo);
		if(favouriteActor)
		{
			/* Remove existing actor from this quicklaunch */
			esdashboard_actor_destroy(favouriteActor);
		}

		/* Now (re-)add current selection to favourites but hidden as
		 * it will become visible and properly set up when function
		 * _esdashboard_quicklaunch_update_property_from_icons is called.
		 */
		favouriteActor=esdashboard_application_button_new_from_app_info(appInfo);
		clutter_actor_hide(favouriteActor);
		esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(favouriteActor), "favourite-app");
		clutter_actor_insert_child_below(CLUTTER_ACTOR(self), favouriteActor, priv->separatorFavouritesToDynamic);

		/* Update favourites from icon order */
		_esdashboard_quicklaunch_update_property_from_icons(self);

		/* Notify about new favourite */
		esdashboard_notify(CLUTTER_ACTOR(self),
							esdashboard_application_button_get_icon_name(ESDASHBOARD_APPLICATION_BUTTON(favouriteActor)),
							_("Favourite '%s' added"),
							esdashboard_application_button_get_display_name(ESDASHBOARD_APPLICATION_BUTTON(favouriteActor)));

		g_signal_emit(self, EsdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_ADDED], 0, appInfo);
	}

	return(CLUTTER_EVENT_STOP);
}

/* Action signal to remove current selected item as favourite was emitted */
static gboolean _esdashboard_quicklaunch_selection_remove_favourite(EsdashboardQuicklaunch *self,
																	EsdashboardFocusable *inSource,
																	const gchar *inAction,
																	ClutterEvent *inEvent)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*currentSelection;
	ClutterActor					*newSelection;
	GAppInfo						*appInfo;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(inSource), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inEvent, CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* If this binding action was not emitted on this quicklaunch
	 * then propagate event because there might be another quicklaunch
	 * which will handle it.
	 */
	if(ESDASHBOARD_QUICKLAUNCH(inSource)!=self) return(CLUTTER_EVENT_PROPAGATE);

	/* Check if current selection in this quicklaunch is a favourite */
	currentSelection=esdashboard_focusable_get_selection(inSource);
	if(!currentSelection)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Source actor %s has no selection so no favourite can be removed.",
							G_OBJECT_TYPE_NAME(inSource));
		return(CLUTTER_EVENT_STOP);
	}

	if(!ESDASHBOARD_IS_APPLICATION_BUTTON(currentSelection))
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Current selection at source actor %s has type %s but only selections of type %s can be removed.",
							G_OBJECT_TYPE_NAME(inSource),
							G_OBJECT_TYPE_NAME(currentSelection),
							g_type_name(ESDASHBOARD_TYPE_APPLICATION_BUTTON));
		return(CLUTTER_EVENT_STOP);
	}

	if(priv->dragPreviewIcon && currentSelection==priv->dragPreviewIcon)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Current selection at source actor %s is %s which is the drag preview icon which cannot be removed.",
							G_OBJECT_TYPE_NAME(inSource),
							G_OBJECT_TYPE_NAME(currentSelection));
		return(CLUTTER_EVENT_STOP);
	}

	/* Get application information */
	appInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(currentSelection));

	/* Notify about removed favourite */
	esdashboard_notify(CLUTTER_ACTOR(self),
						esdashboard_application_button_get_icon_name(ESDASHBOARD_APPLICATION_BUTTON(currentSelection)),
						_("Favourite '%s' removed"),
						esdashboard_application_button_get_display_name(ESDASHBOARD_APPLICATION_BUTTON(currentSelection)));

	if(appInfo) g_signal_emit(self, EsdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_REMOVED], 0, appInfo);

	/* Select previous or next actor in quicklaunch if the favourite
	 * going to be removed is the currently selected one.
	 */
	newSelection=clutter_actor_get_next_sibling(currentSelection);
	if(!newSelection) newSelection=clutter_actor_get_previous_sibling(currentSelection);
	if(!newSelection) newSelection=clutter_actor_get_last_child(CLUTTER_ACTOR(self));

	if(newSelection)
	{
		esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), newSelection);
	}

	/* Remove actor from this quicklaunch */
	esdashboard_actor_destroy(currentSelection);

	/* Re-add removed favourite as dynamically added application button
	 * for non-favourites apps when it is still running.
	 */
	if(appInfo &&
		esdashboard_application_tracker_is_running_by_app_info(priv->appTracker, appInfo))
	{
		ClutterActor			*actor;

		actor=_esdashboard_quicklaunch_create_dynamic_actor(self, appInfo);
		clutter_actor_show(actor);
		clutter_actor_add_child(CLUTTER_ACTOR(self), actor);
	}

	/* Update favourites from icon order */
	_esdashboard_quicklaunch_update_property_from_icons(self);

	/* Action handled */
	return(CLUTTER_EVENT_STOP);
}

/* Action signal to move current selected item to reorder items was emitted */
static gboolean _esdashboard_quicklaunch_favourite_reorder_selection(EsdashboardQuicklaunch *self,
																		EsdashboardOrientation inDirection)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*currentSelection;
	ClutterActor					*newSelection;
	ClutterOrientation				orientation;

	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inDirection<=ESDASHBOARD_ORIENTATION_BOTTOM, CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Determine expected orientation of quicklaunch */
	if(inDirection==ESDASHBOARD_ORIENTATION_LEFT ||
		inDirection==ESDASHBOARD_ORIENTATION_RIGHT)
	{
		orientation=CLUTTER_ORIENTATION_HORIZONTAL;
	}
		else orientation=CLUTTER_ORIENTATION_VERTICAL;

	/* Orientation of quicklaunch must match orientation of direction
	 * to which we should reorder item.
	 */
	if(priv->orientation!=orientation)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Source actor %s does not have expected orientation.",
							G_OBJECT_TYPE_NAME(self));
		return(CLUTTER_EVENT_STOP);
	}

	/* Check if current selection in this quicklaunch is a favourite */
	currentSelection=esdashboard_focusable_get_selection(ESDASHBOARD_FOCUSABLE(self));
	if(!currentSelection)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Source actor %s has no selection so no favourite can be reordered.",
							G_OBJECT_TYPE_NAME(self));
		return(CLUTTER_EVENT_STOP);
	}

	if(!ESDASHBOARD_IS_APPLICATION_BUTTON(currentSelection))
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Current selection at source actor %s has type %s but only selections of type %s can be reordered.",
							G_OBJECT_TYPE_NAME(self),
							G_OBJECT_TYPE_NAME(currentSelection),
							g_type_name(ESDASHBOARD_TYPE_APPLICATION_BUTTON));
		return(CLUTTER_EVENT_STOP);
	}

	if(!esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(currentSelection), "favourite-app"))
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Current selection at source actor %s is not a favourite and cannot be reordered.",
							G_OBJECT_TYPE_NAME(self));
		return(CLUTTER_EVENT_STOP);
	}

	if(priv->dragPreviewIcon && currentSelection==priv->dragPreviewIcon)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Current selection at source actor %s is %s which is the drag preview icon which cannot be reordered.",
							G_OBJECT_TYPE_NAME(self),
							G_OBJECT_TYPE_NAME(currentSelection));
		return(CLUTTER_EVENT_STOP);
	}

	/* Find new position and check if current selection can be moved to this new position.
	 * The current selection cannot be moved if it is already at the beginning or the end
	 * and it cannot bypass an actor of type EsdashboardButton which is used by any
	 * non favourite actor like "switch" button, trash button etc. or ClutterActor used
	 * by separators. Favourites or dynamic non-favourites use actors of type
	 * EsdashboardApplicationButton. So each non-favourite button cannot get into favourites
	 * area and vice versa.
	 */
	if(inDirection==ESDASHBOARD_ORIENTATION_LEFT ||
		inDirection==ESDASHBOARD_ORIENTATION_TOP)
	{
		newSelection=clutter_actor_get_previous_sibling(currentSelection);
	}
		else newSelection=clutter_actor_get_next_sibling(currentSelection);

	if(!newSelection)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Current selection %s at source actor %s is already at end of list",
							G_OBJECT_TYPE_NAME(currentSelection),
							G_OBJECT_TYPE_NAME(self));
		return(CLUTTER_EVENT_STOP);
	}

	if(!ESDASHBOARD_IS_APPLICATION_BUTTON(newSelection))
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Current selection %s at source actor %s cannot be moved because it is blocked by %s.",
							G_OBJECT_TYPE_NAME(currentSelection),
							G_OBJECT_TYPE_NAME(self),
							G_OBJECT_TYPE_NAME(newSelection));
		return(CLUTTER_EVENT_STOP);
	}

	/* Move current selection to new position */
	if(inDirection==ESDASHBOARD_ORIENTATION_LEFT ||
		inDirection==ESDASHBOARD_ORIENTATION_TOP)
	{
		clutter_actor_set_child_below_sibling(CLUTTER_ACTOR(self),
												currentSelection,
												newSelection);
	}
		else
		{
			clutter_actor_set_child_above_sibling(CLUTTER_ACTOR(self),
													currentSelection,
													newSelection);
		}

	/* Update favourites from icon order */
	_esdashboard_quicklaunch_update_property_from_icons(self);

	/* Action handled */
	return(CLUTTER_EVENT_STOP);
}

static gboolean _esdashboard_quicklaunch_favourite_reorder_left(EsdashboardQuicklaunch *self,
																EsdashboardFocusable *inSource,
																const gchar *inAction,
																ClutterEvent *inEvent)
{
	return(_esdashboard_quicklaunch_favourite_reorder_selection(self, ESDASHBOARD_ORIENTATION_LEFT));
}

static gboolean _esdashboard_quicklaunch_favourite_reorder_right(EsdashboardQuicklaunch *self,
																	EsdashboardFocusable *inSource,
																	const gchar *inAction,
																	ClutterEvent *inEvent)
{
	return(_esdashboard_quicklaunch_favourite_reorder_selection(self, ESDASHBOARD_ORIENTATION_RIGHT));
}

/* Action signal to move current selected item to up (to reorder items) was emitted */
static gboolean _esdashboard_quicklaunch_favourite_reorder_up(EsdashboardQuicklaunch *self,
																EsdashboardFocusable *inSource,
																const gchar *inAction,
																ClutterEvent *inEvent)
{
	return(_esdashboard_quicklaunch_favourite_reorder_selection(self, ESDASHBOARD_ORIENTATION_TOP));
}

/* Action signal to move current selected item to down (to reorder items) was emitted */
static gboolean _esdashboard_quicklaunch_favourite_reorder_down(EsdashboardQuicklaunch *self,
																EsdashboardFocusable *inSource,
																const gchar *inAction,
																ClutterEvent *inEvent)
{
	return(_esdashboard_quicklaunch_favourite_reorder_selection(self, ESDASHBOARD_ORIENTATION_BOTTOM));
}

/* An application was started or quitted */
static void _esdashboard_quicklaunch_on_app_tracker_state_changed(EsdashboardQuicklaunch *self,
																	const gchar *inDesktopID,
																	gboolean inIsRunning,
																	gpointer inUserData)
{
	EsdashboardQuicklaunchPrivate	*priv;
	GAppInfo						*appInfo;
	ClutterActor					*actor;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_TRACKER(inUserData));

	priv=self->priv;

	/* Get application information for desktop id */
	appInfo=esdashboard_application_database_lookup_desktop_id(priv->appDB, inDesktopID);
	if(!appInfo)
	{
		ESDASHBOARD_DEBUG(self, APPLICATIONS,
							"Unknown desktop ID '%s' changed state to '%s'",
							inDesktopID,
							inIsRunning ? "running" : "stopped");
		return;
	}

	/* If application is now running and no actor exists for it,
	 * then create an application button but mark it as dynamically added.
	 */
	if(inIsRunning)
	{
		/* Find actor for application which changed running state */
		actor=_esdashboard_quicklaunch_get_actor_for_appinfo(self, appInfo);

		/* Create application button, mark it as dynamically added and
		 * add it to quicklaunch.
		 */
		if(!actor)
		{
			actor=_esdashboard_quicklaunch_create_dynamic_actor(self, appInfo);
			clutter_actor_show(actor);
			clutter_actor_add_child(CLUTTER_ACTOR(self), actor);

			ESDASHBOARD_DEBUG(self, ACTOR,
								"Created dynamic actor %p for newly running desktop ID '%s'",
								actor,
								inDesktopID);
		}
	}

	/* If application is now stopped and an actor exists for it and it is
	 * marked as dynamically added, then destroy it.
	 */
	if(!inIsRunning)
	{
		/* Find actor for application which changed running state */
		actor=_esdashboard_quicklaunch_get_actor_for_appinfo(self, appInfo);

		/* If actor exists and is marked as dynamically added, destroy it */
		if(actor &&
			esdashboard_stylable_has_class(ESDASHBOARD_STYLABLE(actor), "dynamic-app"))
		{
			ESDASHBOARD_DEBUG(self, ACTOR,
								"Destroying dynamic actor %p for stopped desktop ID '%s'",
								actor,
								inDesktopID);

			esdashboard_actor_destroy(actor);
		}
	}
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _esdashboard_quicklaunch_get_preferred_height(ClutterActor *inActor,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	EsdashboardQuicklaunch			*self=ESDASHBOARD_QUICKLAUNCH(inActor);
	EsdashboardQuicklaunchPrivate	*priv=self->priv;
	gfloat							minHeight, naturalHeight;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gfloat							childMinHeight, childNaturalHeight;
	gint							numberChildren;
	gfloat							scale;

	/* Set up default values */
	minHeight=naturalHeight=0.0f;

	/* Determine height for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Iterate through visible children and determine heights */
		numberChildren=0;
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only check visible children */
			if(!clutter_actor_is_visible(child)) continue;

			/* Get sizes of child */
			clutter_actor_get_preferred_height(child,
												-1,
												&childMinHeight,
												&childNaturalHeight);

			/* Determine heights */
			minHeight=MAX(minHeight, childMinHeight);
			naturalHeight=MAX(naturalHeight, childNaturalHeight);

			/* Count visible children */
			numberChildren++;
		}

		/* Check if we need to scale width because of the need to fit
		 * all visible children into given limiting width
		 */
		if(inForWidth>=0.0f)
		{
			scale=_esdashboard_quicklaunch_get_scale_for_width(self, inForWidth, TRUE);
			minHeight*=scale;

			scale=_esdashboard_quicklaunch_get_scale_for_width(self, inForWidth, FALSE);
			naturalHeight*=scale;
		}

		/* Add spacing as padding */
		if(numberChildren>0)
		{
			minHeight+=2*priv->spacing;
			naturalHeight+=2*priv->spacing;
		}
	}
		/* ... otherwise determine height for vertical orientation */
		else
		{
			/* Iterate through visible children and determine height */
			numberChildren=0;
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!clutter_actor_is_visible(child)) continue;

				/* Get child's size */
				clutter_actor_get_preferred_height(child,
													inForWidth,
													&childMinHeight,
													&childNaturalHeight);

				/* Determine heights */
				minHeight+=childMinHeight;
				naturalHeight+=childNaturalHeight;

				/* Count visible children */
				numberChildren++;
			}

			/* Add spacing between children and spacing as padding */
			if(numberChildren>0)
			{
				minHeight+=(numberChildren+1)*priv->spacing;
				naturalHeight+=(numberChildren+1)*priv->spacing;
			}
		}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _esdashboard_quicklaunch_get_preferred_width(ClutterActor *inActor,
															gfloat inForHeight,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth)
{
	EsdashboardQuicklaunch			*self=ESDASHBOARD_QUICKLAUNCH(inActor);
	EsdashboardQuicklaunchPrivate	*priv=self->priv;
	gfloat							minWidth, naturalWidth;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gfloat							childMinWidth, childNaturalWidth;
	gint							numberChildren;
	gfloat							scale;

	/* Set up default values */
	minWidth=naturalWidth=0.0f;

	/* Determine width for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Iterate through visible children and determine width */
		numberChildren=0;
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only check visible children */
			if(!clutter_actor_is_visible(child)) continue;

			/* Get child's size */
			clutter_actor_get_preferred_width(child,
												inForHeight,
												&childMinWidth,
												&childNaturalWidth);

			/* Determine widths */
			minWidth+=childMinWidth;
			naturalWidth+=childNaturalWidth;

			/* Count visible children */
			numberChildren++;
		}

		/* Add spacing between children and spacing as padding */
		if(numberChildren>0)
		{
			minWidth+=(numberChildren+1)*priv->spacing;
			naturalWidth+=(numberChildren+1)*priv->spacing;
		}
	}
		/* ... otherwise determine width for vertical orientation */
		else
		{
			/* Iterate through visible children and determine widths */
			numberChildren=0;
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!clutter_actor_is_visible(child)) continue;

				/* Get sizes of child */
				clutter_actor_get_preferred_width(child,
													-1,
													&childMinWidth,
													&childNaturalWidth);

				/* Determine widths */
				minWidth=MAX(minWidth, childMinWidth);
				naturalWidth=MAX(naturalWidth, childNaturalWidth);

				/* Count visible children */
				numberChildren++;
			}

			/* Check if we need to scale width because of the need to fit
			 * all visible children into given limiting height
			 */
			if(inForHeight>=0.0f)
			{
				scale=_esdashboard_quicklaunch_get_scale_for_height(self, inForHeight, TRUE);
				minWidth*=scale;

				scale=_esdashboard_quicklaunch_get_scale_for_height(self, inForHeight, FALSE);
				naturalWidth*=scale;
			}

			/* Add spacing as padding */
			if(numberChildren>0)
			{
				minWidth+=2*priv->spacing;
				naturalWidth+=2*priv->spacing;
			}
		}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _esdashboard_quicklaunch_allocate(ClutterActor *inActor,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	EsdashboardQuicklaunch			*self=ESDASHBOARD_QUICKLAUNCH(inActor);
	EsdashboardQuicklaunchPrivate	*priv=self->priv;
	gfloat							availableWidth, availableHeight;
	gfloat							childWidth, childHeight;
	ClutterActor					*child;
	ClutterActorIter				iter;
	ClutterActorBox					childAllocation={ 0, };
	ClutterActorBox					oldChildAllocation={ 0, };
	gboolean						fixedPosition;
	gfloat							fixedX, fixedY;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(esdashboard_quicklaunch_parent_class)->allocate(inActor, inBox, inFlags);

	/* Get available size */
	clutter_actor_box_get_size(inBox, &availableWidth, &availableHeight);

	/* Find scaling to get all children fit the allocation */
	priv->scaleCurrent=_esdashboard_quicklaunch_get_scale_for_height(self, availableHeight, FALSE);

	/* Calculate new position and size of visible children */
	childAllocation.x1=childAllocation.y1=priv->spacing;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Is child visible? */
		if(!clutter_actor_is_visible(child)) continue;

		/* Calculate new position and size of child. Because we will set
		 * a scale factor to the actor we have to set the real unscaled
		 * width and height but the position should be "translated" to
		 * scaled sizes.
		 */
		clutter_actor_get_preferred_size(child, NULL, NULL, &childWidth, &childHeight);

		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			childAllocation.y1=ceil(MAX(((availableHeight-(childHeight*priv->scaleCurrent))/2.0f), priv->spacing));
			childAllocation.y2=ceil(childAllocation.y1+childHeight);
			childAllocation.x2=ceil(childAllocation.x1+childWidth);
		}
			else
			{
				childAllocation.x1=ceil(MAX(((availableWidth-(childWidth*priv->scaleCurrent))/2.0f), priv->spacing));
				childAllocation.x2=ceil(childAllocation.x1+childWidth);
				childAllocation.y2=ceil(childAllocation.y1+childHeight);
			}

		clutter_actor_set_scale(child, priv->scaleCurrent, priv->scaleCurrent);

		/* Respect fixed position of child */
		g_object_get(child,
						"fixed-position-set", &fixedPosition,
						"fixed-x", &fixedX,
						"fixed-y", &fixedY,
						NULL);

		if(fixedPosition)
		{
			oldChildAllocation.x1=childAllocation.x1;
			oldChildAllocation.x2=childAllocation.x2;
			oldChildAllocation.y1=childAllocation.y1;
			oldChildAllocation.y2=childAllocation.y2;

			childWidth=childAllocation.x2-childAllocation.x1;
			childHeight=childAllocation.y2-childAllocation.y1;

			childAllocation.x1=ceil(fixedX);
			childAllocation.x2=childAllocation.x1+childWidth;
			childAllocation.y1=ceil(fixedY);
			childAllocation.y2=childAllocation.y1+childHeight;
		}

		/* Allocate child */
		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		if(fixedPosition)
		{
			childAllocation.x1=oldChildAllocation.x1;
			childAllocation.x2=oldChildAllocation.x2;
			childAllocation.y1=oldChildAllocation.y1;
			childAllocation.y2=oldChildAllocation.y2;
		}

		childWidth*=priv->scaleCurrent;
		childHeight*=priv->scaleCurrent;
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) childAllocation.x1=ceil(childAllocation.x1+childWidth+priv->spacing);
			else childAllocation.y1=ceil(childAllocation.y1+childHeight+priv->spacing);
	}
}

/* IMPLEMENTATION: Interface EsdashboardFocusable */

/* Determine if this actor supports selection */
static gboolean _esdashboard_quicklaunch_focusable_supports_selection(EsdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}


/* Get current selection */
static ClutterActor* _esdashboard_quicklaunch_focusable_get_selection(EsdashboardFocusable *inFocusable)
{
	EsdashboardQuicklaunch				*self;
	EsdashboardQuicklaunchPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(inFocusable), CLUTTER_EVENT_PROPAGATE);

	self=ESDASHBOARD_QUICKLAUNCH(inFocusable);
	priv=self->priv;

	/* Return current selection */
	return(priv->selectedItem);
}

/* Set new selection */
static gboolean _esdashboard_quicklaunch_focusable_set_selection(EsdashboardFocusable *inFocusable,
																	ClutterActor *inSelection)
{
	EsdashboardQuicklaunch				*self;
	EsdashboardQuicklaunchPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_QUICKLAUNCH(inFocusable);
	priv=self->priv;

	/* Check that selection is a child of this actor */
	if(inSelection &&
		!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning("%s is a child of %s and cannot be selected at %s",
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>",
					G_OBJECT_TYPE_NAME(self));
	}

	/* Set new selection */
	priv->selectedItem=inSelection;

	/* New selection was set successfully */
	return(TRUE);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _esdashboard_quicklaunch_focusable_find_selection(EsdashboardFocusable *inFocusable,
																		ClutterActor *inSelection,
																		EsdashboardSelectionTarget inDirection)
{
	EsdashboardQuicklaunch				*self;
	EsdashboardQuicklaunchPrivate		*priv;
	ClutterActor						*selection;
	ClutterActor						*newSelection;
	gchar								*valueName;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	self=ESDASHBOARD_QUICKLAUNCH(inFocusable);
	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;

	/* If there is nothing selected, select first actor and return */
	if(!inSelection)
	{
		selection=clutter_actor_get_first_child(CLUTTER_ACTOR(self));

		valueName=esdashboard_get_enum_value_name(ESDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
		ESDASHBOARD_DEBUG(self, ACTOR,
							"No selection at %s, so select first child %s for direction %s",
							G_OBJECT_TYPE_NAME(self),
							selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
							valueName);
		g_free(valueName);

		return(selection);
	}

	/* Check that selection is a child of this actor otherwise return NULL */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning("Cannot lookup selection target at %s because %s is a child of %s",
					G_OBJECT_TYPE_NAME(self),
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>");

		return(NULL);
	}

	/* Find target selection */
	switch(inDirection)
	{
		case ESDASHBOARD_SELECTION_TARGET_LEFT:
			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				newSelection=esdashboard_quicklaunch_get_previous_selectable(self, inSelection);
			}
			break;

		case ESDASHBOARD_SELECTION_TARGET_RIGHT:
			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				newSelection=esdashboard_quicklaunch_get_next_selectable(self, inSelection);
			}
			break;

		case ESDASHBOARD_SELECTION_TARGET_UP:
			if(priv->orientation==CLUTTER_ORIENTATION_VERTICAL)
			{
				newSelection=esdashboard_quicklaunch_get_previous_selectable(self, inSelection);
			}
			break;

		case ESDASHBOARD_SELECTION_TARGET_DOWN:
			if(priv->orientation==CLUTTER_ORIENTATION_VERTICAL)
			{
				newSelection=esdashboard_quicklaunch_get_next_selectable(self, inSelection);
			}
			break;

		case ESDASHBOARD_SELECTION_TARGET_FIRST:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_UP:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
			if(inDirection==ESDASHBOARD_SELECTION_TARGET_FIRST ||
				(inDirection==ESDASHBOARD_SELECTION_TARGET_PAGE_UP && priv->orientation==CLUTTER_ORIENTATION_VERTICAL) ||
				(inDirection==ESDASHBOARD_SELECTION_TARGET_PAGE_LEFT && priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL))
			{
				newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(self));
				while(newSelection && !clutter_actor_is_visible(newSelection))
				{
					newSelection=clutter_actor_get_next_sibling(newSelection);
				}
			}
			break;

		case ESDASHBOARD_SELECTION_TARGET_LAST:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
			if(inDirection==ESDASHBOARD_SELECTION_TARGET_LAST ||
				(inDirection==ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN && priv->orientation==CLUTTER_ORIENTATION_VERTICAL) ||
				(inDirection==ESDASHBOARD_SELECTION_TARGET_PAGE_RIGHT && priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL))
			{
				newSelection=clutter_actor_get_last_child(CLUTTER_ACTOR(self));
				while(newSelection && !clutter_actor_is_visible(newSelection))
				{
					newSelection=clutter_actor_get_previous_sibling(newSelection);
				}
			}
			break;

		case ESDASHBOARD_SELECTION_TARGET_NEXT:
			newSelection=esdashboard_quicklaunch_get_next_selectable(self, inSelection);
			while(newSelection && !clutter_actor_is_visible(newSelection))
			{
				newSelection=clutter_actor_get_next_sibling(newSelection);
			}

			if(!newSelection)
			{
				newSelection=esdashboard_quicklaunch_get_previous_selectable(self, inSelection);
				while(newSelection && !clutter_actor_is_visible(newSelection))
				{
					newSelection=clutter_actor_get_next_sibling(newSelection);
				}
			}
			break;

		default:
			{
				valueName=esdashboard_get_enum_value_name(ESDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical("Focusable object %s does not handle selection direction of type %s.",
							G_OBJECT_TYPE_NAME(self),
							valueName);
				g_free(valueName);
			}
			break;
	}

	/* If new selection could be found override current selection with it */
	if(newSelection) selection=newSelection;

	/* Return new selection found */
	ESDASHBOARD_DEBUG(self, ACTOR,
						"Selecting %s at %s for current selection %s in direction %u",
						selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
						G_OBJECT_TYPE_NAME(self),
						inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
						inDirection);

	return(selection);
}

/* Activate selection */
static gboolean _esdashboard_quicklaunch_focusable_activate_selection(EsdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	EsdashboardQuicklaunch				*self;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(inFocusable), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_QUICKLAUNCH(inFocusable);

	/* Check that selection is a child of this actor */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning("%s is a child of %s and cannot be activated at %s",
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>",
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Activate selection */
	g_signal_emit_by_name(inSelection, "clicked");

	return(TRUE);
}

/* Interface initialization
 * Set up default functions
 */
void _esdashboard_quicklaunch_focusable_iface_init(EsdashboardFocusableInterface *iface)
{
	iface->supports_selection=_esdashboard_quicklaunch_focusable_supports_selection;
	iface->get_selection=_esdashboard_quicklaunch_focusable_get_selection;
	iface->set_selection=_esdashboard_quicklaunch_focusable_set_selection;
	iface->find_selection=_esdashboard_quicklaunch_focusable_find_selection;
	iface->activate_selection=_esdashboard_quicklaunch_focusable_activate_selection;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_quicklaunch_dispose(GObject *inObject)
{
	EsdashboardQuicklaunchPrivate	*priv=ESDASHBOARD_QUICKLAUNCH(inObject)->priv;

	/* Release our allocated variables */
	if(priv->esconfFavouritesBindingID)
	{
		esconf_g_property_unbind(priv->esconfFavouritesBindingID);
		priv->esconfFavouritesBindingID=0;
	}

	if(priv->esconfChannel)
	{
		priv->esconfChannel=NULL;
	}

	if(priv->appTracker)
	{
		g_object_unref(priv->appTracker);
		priv->appTracker=NULL;
	}

	if(priv->appDB)
	{
		g_object_unref(priv->appDB);
		priv->appDB=NULL;
	}

	if(priv->favourites)
	{
		esconf_array_free(priv->favourites);
		priv->favourites=NULL;
	}

	if(priv->separatorFavouritesToDynamic)
	{
		clutter_actor_destroy(priv->separatorFavouritesToDynamic);
		priv->separatorFavouritesToDynamic=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_quicklaunch_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_quicklaunch_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardQuicklaunch			*self=ESDASHBOARD_QUICKLAUNCH(inObject);

	switch(inPropID)
	{
		case PROP_FAVOURITES:
			_esdashboard_quicklaunch_set_favourites(self, inValue);
			break;

		case PROP_NORMAL_ICON_SIZE:
			esdashboard_quicklaunch_set_normal_icon_size(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			esdashboard_quicklaunch_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_ORIENTATION:
			esdashboard_quicklaunch_set_orientation(self, g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_quicklaunch_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardQuicklaunch			*self=ESDASHBOARD_QUICKLAUNCH(inObject);
	EsdashboardQuicklaunchPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_FAVOURITES:
			g_value_set_boxed(outValue, priv->favourites);
			break;

		case PROP_NORMAL_ICON_SIZE:
			g_value_set_float(outValue, priv->normalIconSize);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_ORIENTATION:
			g_value_set_enum(outValue, priv->orientation);
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
static void esdashboard_quicklaunch_class_init(EsdashboardQuicklaunchClass *klass)
{
	EsdashboardActorClass	*actorClass=ESDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_quicklaunch_dispose;
	gobjectClass->set_property=_esdashboard_quicklaunch_set_property;
	gobjectClass->get_property=_esdashboard_quicklaunch_get_property;

	clutterActorClass->get_preferred_width=_esdashboard_quicklaunch_get_preferred_width;
	clutterActorClass->get_preferred_height=_esdashboard_quicklaunch_get_preferred_height;
	clutterActorClass->allocate=_esdashboard_quicklaunch_allocate;

	klass->selection_add_favourite=_esdashboard_quicklaunch_selection_add_favourite;
	klass->selection_remove_favourite=_esdashboard_quicklaunch_selection_remove_favourite;
	klass->favourite_reorder_left=_esdashboard_quicklaunch_favourite_reorder_left;
	klass->favourite_reorder_right=_esdashboard_quicklaunch_favourite_reorder_right;
	klass->favourite_reorder_up=_esdashboard_quicklaunch_favourite_reorder_up;
	klass->favourite_reorder_down=_esdashboard_quicklaunch_favourite_reorder_down;

	/* Define properties */
	EsdashboardQuicklaunchProperties[PROP_FAVOURITES]=
		g_param_spec_boxed("favourites",
							"Favourites",
							"An array of strings pointing to desktop files shown as icons",
							ESDASHBOARD_TYPE_POINTER_ARRAY,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardQuicklaunchProperties[PROP_NORMAL_ICON_SIZE]=
		g_param_spec_float("normal-icon-size",
								"Normal icon size",
								"Unscale size of icon",
								1.0, G_MAXFLOAT,
								1.0f,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardQuicklaunchProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
								"Spacing",
								"The spacing between children",
								0.0, G_MAXFLOAT,
								0.0,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardQuicklaunchProperties[PROP_ORIENTATION]=
		g_param_spec_enum("orientation",
							"Orientation",
							"The orientation to layout children",
							CLUTTER_TYPE_ORIENTATION,
							CLUTTER_ORIENTATION_VERTICAL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardQuicklaunchProperties);

	/* Define stylable properties */
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardQuicklaunchProperties[PROP_NORMAL_ICON_SIZE]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardQuicklaunchProperties[PROP_SPACING]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardQuicklaunchProperties[PROP_ORIENTATION]);

	/* Define signals */
	EsdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_ADDED]=
		g_signal_new("favourite-added",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardQuicklaunchClass, favourite_added),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_APP_INFO);

	EsdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_REMOVED]=
		g_signal_new("favourite-removed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardQuicklaunchClass, favourite_removed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_APP_INFO);

	EsdashboardQuicklaunchSignals[ACTION_SELECTION_ADD_FAVOURITE]=
		g_signal_new("selection-add-favourite",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardQuicklaunchClass, selection_add_favourite),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardQuicklaunchSignals[ACTION_SELECTION_REMOVE_FAVOURITE]=
		g_signal_new("selection-remove-favourite",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardQuicklaunchClass, selection_remove_favourite),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardQuicklaunchSignals[ACTION_FAVOURITE_REORDER_LEFT]=
		g_signal_new("favourite-reorder-left",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardQuicklaunchClass, favourite_reorder_left),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardQuicklaunchSignals[ACTION_FAVOURITE_REORDER_RIGHT]=
		g_signal_new("favourite-reorder-right",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardQuicklaunchClass, favourite_reorder_right),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardQuicklaunchSignals[ACTION_FAVOURITE_REORDER_UP]=
		g_signal_new("favourite-reorder-up",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardQuicklaunchClass, favourite_reorder_up),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	EsdashboardQuicklaunchSignals[ACTION_FAVOURITE_REORDER_DOWN]=
		g_signal_new("favourite-reorder-down",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(EsdashboardQuicklaunchClass, favourite_reorder_down),
						g_signal_accumulator_true_handled,
						NULL,
						_esdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						ESDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_quicklaunch_init(EsdashboardQuicklaunch *self)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterRequestMode				requestMode;
	ClutterAction					*dropAction;

	priv=self->priv=esdashboard_quicklaunch_get_instance_private(self);

	/* Set up default values */
	priv->favourites=NULL;
	priv->spacing=0.0f;
	priv->orientation=CLUTTER_ORIENTATION_VERTICAL;
	priv->normalIconSize=1.0f;
	priv->scaleCurrent=DEFAULT_SCALE_MAX;
	priv->scaleMin=DEFAULT_SCALE_MIN;
	priv->scaleMax=DEFAULT_SCALE_MAX;
	priv->scaleStep=DEFAULT_SCALE_STEP;
	priv->esconfChannel=esdashboard_application_get_esconf_channel(NULL);
	priv->dragMode=DRAG_MODE_NONE;
	priv->dragPreviewIcon=NULL;
	priv->selectedItem=NULL;
	priv->appDB=esdashboard_application_database_get_default();

	/* Set up this actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	requestMode=(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL ? CLUTTER_REQUEST_HEIGHT_FOR_WIDTH : CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);
	clutter_actor_set_request_mode(CLUTTER_ACTOR(self), requestMode);

	dropAction=esdashboard_drop_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), dropAction);
	g_signal_connect_swapped(dropAction, "begin", G_CALLBACK(_esdashboard_quicklaunch_on_drop_begin), self);
	g_signal_connect_swapped(dropAction, "drop", G_CALLBACK(_esdashboard_quicklaunch_on_drop_drop), self);
	g_signal_connect_swapped(dropAction, "end", G_CALLBACK(_esdashboard_quicklaunch_on_drop_end), self);
	g_signal_connect_swapped(dropAction, "drag-enter", G_CALLBACK(_esdashboard_quicklaunch_on_drop_enter), self);
	g_signal_connect_swapped(dropAction, "drag-motion", G_CALLBACK(_esdashboard_quicklaunch_on_drop_motion), self);
	g_signal_connect_swapped(dropAction, "drag-leave", G_CALLBACK(_esdashboard_quicklaunch_on_drop_leave), self);

	/* Add "applications" button */
	priv->appsButton=esdashboard_toggle_button_new_with_text(_("Applications"));
	clutter_actor_set_name(priv->appsButton, "applications-button");
	esdashboard_label_set_icon_size(ESDASHBOARD_LABEL(priv->appsButton), priv->normalIconSize);
	esdashboard_label_set_sync_icon_size(ESDASHBOARD_LABEL(priv->appsButton), FALSE);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->appsButton);

	/* Next add trash button to box but initially hidden and register as drop target */
	priv->trashButton=esdashboard_toggle_button_new_with_text(_("Remove"));
	clutter_actor_set_name(priv->trashButton, "trash-button");
	clutter_actor_hide(priv->trashButton);
	esdashboard_label_set_icon_size(ESDASHBOARD_LABEL(priv->trashButton), priv->normalIconSize);
	esdashboard_label_set_sync_icon_size(ESDASHBOARD_LABEL(priv->trashButton), FALSE);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->trashButton);

	dropAction=esdashboard_drop_action_new();
	clutter_actor_add_action(priv->trashButton, CLUTTER_ACTION(dropAction));
	g_signal_connect_swapped(dropAction, "begin", G_CALLBACK(_esdashboard_quicklaunch_on_trash_drop_begin), self);
	g_signal_connect_swapped(dropAction, "drop", G_CALLBACK(_esdashboard_quicklaunch_on_trash_drop_drop), self);
	g_signal_connect_swapped(dropAction, "end", G_CALLBACK(_esdashboard_quicklaunch_on_trash_drop_end), self);
	g_signal_connect_swapped(dropAction, "drag-enter", G_CALLBACK(_esdashboard_quicklaunch_on_trash_drop_enter), self);
	g_signal_connect_swapped(dropAction, "drag-leave", G_CALLBACK(_esdashboard_quicklaunch_on_trash_drop_leave), self);

	/* Add a hidden actor used as separator between application buttons for
	 * favourites and dynamically added one (for running non-favourite applications).
	 * It used to add application buttons for favourites before dynamically
	 * added ones.
	 */
	priv->separatorFavouritesToDynamic=clutter_actor_new();
	clutter_actor_hide(priv->separatorFavouritesToDynamic);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->separatorFavouritesToDynamic);

	/* Bind to esconf to react on changes */
	priv->esconfFavouritesBindingID=esconf_g_property_bind(priv->esconfChannel,
															FAVOURITES_ESCONF_PROP,
															ESDASHBOARD_TYPE_POINTER_ARRAY,
															self,
															"favourites");

	/* Set up default favourite items if property in channel does not exist
	 * because it indicates first start.
	 */
	if(!esconf_channel_has_property(priv->esconfChannel, FAVOURITES_ESCONF_PROP))
	{
		_esdashboard_quicklaunch_setup_default_favourites(self);
	}

	/* Connect to application tracker to recognize other running application
	 * which are not known favourites.
	 */
	priv->appTracker=esdashboard_application_tracker_get_default();
	g_signal_connect_swapped(priv->appTracker, "state-changed", G_CALLBACK(_esdashboard_quicklaunch_on_app_tracker_state_changed), self);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* esdashboard_quicklaunch_new(void)
{
	return(g_object_new(ESDASHBOARD_TYPE_QUICKLAUNCH, NULL));
}

ClutterActor* esdashboard_quicklaunch_new_with_orientation(ClutterOrientation inOrientation)
{
	g_return_val_if_fail(inOrientation==CLUTTER_ORIENTATION_HORIZONTAL || inOrientation==CLUTTER_ORIENTATION_VERTICAL, NULL);

	return(g_object_new(ESDASHBOARD_TYPE_QUICKLAUNCH,
						"orientation", inOrientation,
						NULL));
}

/* Get/set spacing between children */
gfloat esdashboard_quicklaunch_get_normal_icon_size(EsdashboardQuicklaunch *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);

	return(self->priv->normalIconSize);
}

void esdashboard_quicklaunch_set_normal_icon_size(EsdashboardQuicklaunch *self, const gfloat inIconSize)
{
	EsdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inIconSize>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->normalIconSize!=inIconSize)
	{
		/* Set value */
		priv->normalIconSize=inIconSize;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		esdashboard_label_set_icon_size(ESDASHBOARD_LABEL(priv->appsButton), priv->normalIconSize);
		esdashboard_label_set_icon_size(ESDASHBOARD_LABEL(priv->trashButton), priv->normalIconSize);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardQuicklaunchProperties[PROP_NORMAL_ICON_SIZE]);
	}
}

/* Get/set spacing between children */
gfloat esdashboard_quicklaunch_get_spacing(EsdashboardQuicklaunch *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);

	return(self->priv->spacing);
}

void esdashboard_quicklaunch_set_spacing(EsdashboardQuicklaunch *self, const gfloat inSpacing)
{
	EsdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set value */
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		esdashboard_background_set_corner_radius(ESDASHBOARD_BACKGROUND(self), priv->spacing);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardQuicklaunchProperties[PROP_SPACING]);
	}
}

/* Get/set orientation */
ClutterOrientation esdashboard_quicklaunch_get_orientation(EsdashboardQuicklaunch *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), CLUTTER_ORIENTATION_VERTICAL);

	return(self->priv->orientation);
}

void esdashboard_quicklaunch_set_orientation(EsdashboardQuicklaunch *self, ClutterOrientation inOrientation)
{
	EsdashboardQuicklaunchPrivate	*priv;
	ClutterRequestMode				requestMode;

	g_return_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inOrientation==CLUTTER_ORIENTATION_HORIZONTAL ||
						inOrientation==CLUTTER_ORIENTATION_VERTICAL);

	priv=self->priv;

	/* Set value if changed */
	if(priv->orientation!=inOrientation)
	{
		/* Set value */
		priv->orientation=inOrientation;

		requestMode=(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL ? CLUTTER_REQUEST_HEIGHT_FOR_WIDTH : CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);
		clutter_actor_set_request_mode(CLUTTER_ACTOR(self), requestMode);

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardQuicklaunchProperties[PROP_ORIENTATION]);
	}
}

/* Get apps button */
EsdashboardToggleButton* esdashboard_quicklaunch_get_apps_button(EsdashboardQuicklaunch *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_QUICKLAUNCH(self), NULL);

	return(ESDASHBOARD_TOGGLE_BUTTON(self->priv->appsButton));
}
