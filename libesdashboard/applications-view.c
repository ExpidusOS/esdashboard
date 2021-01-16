/*
 * applications-view: A view showing all installed applications as menu
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

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <libesdashboard/applications-view.h>
#include <libesdashboard/view.h>
#include <libesdashboard/applications-menu-model.h>
#include <libesdashboard/types.h>
#include <libesdashboard/button.h>
#include <libesdashboard/application.h>
#include <libesdashboard/application-button.h>
#include <libesdashboard/enums.h>
#include <libesdashboard/drag-action.h>
#include <libesdashboard/stylable.h>
#include <libesdashboard/focusable.h>
#include <libesdashboard/focus-manager.h>
#include <libesdashboard/dynamic-table-layout.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/desktop-app-info.h>
#include <libesdashboard/application-database.h>
#include <libesdashboard/click-action.h>
#include <libesdashboard/popup-menu.h>
#include <libesdashboard/popup-menu-item-button.h>
#include <libesdashboard/popup-menu-item-separator.h>
#include <libesdashboard/application-tracker.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
static void _esdashboard_applications_view_focusable_iface_init(EsdashboardFocusableInterface *iface);

struct _EsdashboardApplicationsViewPrivate
{
	/* Properties related */
	EsdashboardViewMode					viewMode;
	gfloat								spacing;
	gchar								*parentMenuIcon;
	gchar								*formatTitleOnly;
	gchar								*formatTitleDescription;

	/* Instance related */
	ClutterLayoutManager				*layout;
	EsdashboardApplicationsMenuModel	*apps;
	MarkonMenuElement					*currentRootMenuElement;

	gpointer							selectedItem;

	EsconfChannel						*esconfChannel;
	gboolean							showAllAppsMenu;
	guint								esconfShowAllAppsMenuBindingID;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardApplicationsView,
						esdashboard_applications_view,
						ESDASHBOARD_TYPE_VIEW,
						G_ADD_PRIVATE(EsdashboardApplicationsView)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_FOCUSABLE, _esdashboard_applications_view_focusable_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_VIEW_MODE,
	PROP_SPACING,

	PROP_PARENT_MENU_ICON,
	PROP_FORMAT_TITLE_ONLY,
	PROP_FORMAT_TITLE_DESCRIPTION,

	PROP_SHOW_ALL_APPS,

	PROP_LAST
};

static GParamSpec* EsdashboardApplicationsViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define ALL_APPLICATIONS_MENU_ICON		"applications-other"
#define SHOW_ALL_APPS_ESCONF_PROP		"/components/applications-view/show-all-apps"

/* Forward declarations */
static void _esdashboard_applications_view_on_item_clicked(EsdashboardApplicationsView *self, gpointer inUserData);

/* Set up child actor for current view mode */
static void _esdashboard_applications_view_setup_actor_for_view_mode(EsdashboardApplicationsView *self, ClutterActor *inActor)
{
	EsdashboardApplicationsViewPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));

	priv=self->priv;

	/* In list mode just fill all available space and align to top-left corner */
	if(priv->viewMode==ESDASHBOARD_VIEW_MODE_LIST)
	{
		clutter_actor_set_x_expand(inActor, TRUE);
		clutter_actor_set_y_expand(inActor, TRUE);
		clutter_actor_set_x_align(inActor, CLUTTER_ACTOR_ALIGN_FILL);
		clutter_actor_set_y_align(inActor, CLUTTER_ACTOR_ALIGN_FILL);

		if(ESDASHBOARD_IS_STYLABLE(inActor)) esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(inActor), "view-mode-list");
	}
		/* In view mode do not fill all available space and align to top-center
		 * corner or middle-left corner depending on request mode of actor.
		 */
		else
		{
			clutter_actor_set_x_expand(inActor, FALSE);
			clutter_actor_set_y_expand(inActor, FALSE);
			if(clutter_actor_get_request_mode(inActor)==CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
			{
				clutter_actor_set_x_align(CLUTTER_ACTOR(inActor), CLUTTER_ACTOR_ALIGN_CENTER);
				clutter_actor_set_y_align(CLUTTER_ACTOR(inActor), CLUTTER_ACTOR_ALIGN_START);
			}
				else
				{
					clutter_actor_set_x_align(CLUTTER_ACTOR(inActor), CLUTTER_ACTOR_ALIGN_START);
					clutter_actor_set_y_align(CLUTTER_ACTOR(inActor), CLUTTER_ACTOR_ALIGN_CENTER);
				}

			if(ESDASHBOARD_IS_STYLABLE(inActor)) esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(inActor), "view-mode-icon");
		}
}


/* Drag of an menu item begins */
static void _esdashboard_applications_view_on_drag_begin(ClutterDragAction *inAction,
															ClutterActor *inActor,
															gfloat inStageX,
															gfloat inStageY,
															ClutterModifierType inModifiers,
															gpointer inUserData)
{
	GAppInfo							*appInfo;
	ClutterActor						*dragHandle;
	ClutterStage						*stage;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

	/* Prevent signal "clicked" from being emitted on dragged icon */
	g_signal_handlers_block_by_func(inActor, _esdashboard_applications_view_on_item_clicked, inUserData);

	/* Get stage */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));

	/* Create a application icon for drag handle */
	appInfo=esdashboard_application_button_get_app_info(ESDASHBOARD_APPLICATION_BUTTON(inActor));

	dragHandle=esdashboard_application_button_new_from_app_info(appInfo);
	clutter_actor_set_position(dragHandle, inStageX, inStageY);
	clutter_actor_add_child(CLUTTER_ACTOR(stage), dragHandle);

	clutter_drag_action_set_drag_handle(inAction, dragHandle);
}

/* Drag of an menu item ends */
static void _esdashboard_applications_view_on_drag_end(ClutterDragAction *inAction,
														ClutterActor *inActor,
														gfloat inStageX,
														gfloat inStageY,
														ClutterModifierType inModifiers,
														gpointer inUserData)
{
	ClutterActor					*dragHandle;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

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
	g_signal_handlers_unblock_by_func(inActor, _esdashboard_applications_view_on_item_clicked, inUserData);
}

/* Filter of applications data model has changed */
static void _esdashboard_applications_view_on_menu_clicked(EsdashboardButton *inButton, gpointer inUserData)
{
	EsdashboardApplicationsView			*self;
	EsdashboardApplicationsViewPrivate	*priv;
	ClutterActor						*parent;
	MarkonMenu							*menu;

	g_return_if_fail(ESDASHBOARD_IS_BUTTON(inButton));
	g_return_if_fail(MARKON_IS_MENU(inUserData));

	menu=MARKON_MENU(inUserData);

	/* Find this view's object */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(inButton));
	while(parent && !ESDASHBOARD_IS_APPLICATIONS_VIEW(parent))
	{
		parent=clutter_actor_get_parent(parent);
	}

	if(!parent)
	{
		g_warning("Could not find view of type %s for menu '%s'",
					g_type_name(ESDASHBOARD_TYPE_APPLICATIONS_VIEW),
					markon_menu_element_get_name(MARKON_MENU_ELEMENT(menu)));
		return;
	}

	/* Get this view's object */
	self=ESDASHBOARD_APPLICATIONS_VIEW(parent);
	priv=self->priv;

	/* Change menu */
	priv->currentRootMenuElement=MARKON_MENU_ELEMENT(menu);
	esdashboard_applications_menu_model_filter_by_section(priv->apps, menu);
	esdashboard_view_scroll_to(ESDASHBOARD_VIEW(self), -1, 0);
}

/* Parent menu item to go up in menus was clicked */
static void _esdashboard_applications_view_on_parent_menu_clicked(EsdashboardApplicationsView *self, gpointer inUserData)
{
	EsdashboardApplicationsViewPrivate	*priv;
	MarkonMenuElement					*element;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=self->priv;

	/* Get associated menu element of button */
	if(priv->currentRootMenuElement &&
		MARKON_IS_MENU(priv->currentRootMenuElement))
	{
		element=MARKON_MENU_ELEMENT(markon_menu_get_parent(MARKON_MENU(priv->currentRootMenuElement)));

		priv->currentRootMenuElement=element;
		esdashboard_applications_menu_model_filter_by_section(priv->apps, MARKON_MENU(element));
		esdashboard_view_scroll_to(ESDASHBOARD_VIEW(self), -1, 0);
	}
}

/* An application menu item was clicked */
static void _esdashboard_applications_view_on_item_clicked(EsdashboardApplicationsView *self, gpointer inUserData)
{
	EsdashboardApplicationButton		*button;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	button=ESDASHBOARD_APPLICATION_BUTTON(inUserData);

	/* A menu item was clicked so execute command and quit application */
	if(esdashboard_application_button_execute(button, NULL))
	{
		/* Launching application seems to be successfuly so quit application */
		esdashboard_application_suspend_or_quit(NULL);
		return;
	}
}

/* User selected to open a new window or to launch that application at pop-up menu */
static void _esdashboard_applications_view_on_popup_menu_item_launch(EsdashboardPopupMenuItem *inMenuItem,
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

/* A right-click might have happened on an application icon */
static void _esdashboard_applications_view_on_popup_menu(EsdashboardApplicationsView *self,
															ClutterActor *inActor,
															gpointer inUserData)
{
	EsdashboardApplicationButton				*button;
	EsdashboardClickAction						*action;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(ESDASHBOARD_IS_CLICK_ACTION(inUserData));

	button=ESDASHBOARD_APPLICATION_BUTTON(inActor);
	action=ESDASHBOARD_CLICK_ACTION(inUserData);

	/* Check if right button was used when the application button was clicked */
	if(esdashboard_click_action_get_button(action)==ESDASHBOARD_CLICK_ACTION_RIGHT_BUTTON)
	{
		ClutterActor							*popup;
		ClutterActor							*menuItem;
		GAppInfo								*appInfo;
		EsdashboardApplicationTracker			*appTracker;

		/* Get app info for application button as it is needed most the time */
		appInfo=esdashboard_application_button_get_app_info(button);
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
		if(esdashboard_application_button_add_popup_menu_items_for_windows(button, ESDASHBOARD_POPUP_MENU(popup))>0)
		{
			/* Add a separator to split windows from other actions in pop-up menu */
			menuItem=esdashboard_popup_menu_item_separator_new();
			clutter_actor_set_x_expand(menuItem, TRUE);
			esdashboard_popup_menu_add_item(ESDASHBOARD_POPUP_MENU(popup), ESDASHBOARD_POPUP_MENU_ITEM(menuItem));
		}

		/* Add menu item to launch application if it is not running */
		appTracker=esdashboard_application_tracker_get_default();
		if(!esdashboard_application_tracker_is_running_by_app_info(appTracker, appInfo))
		{
			menuItem=esdashboard_popup_menu_item_button_new();
			esdashboard_label_set_text(ESDASHBOARD_LABEL(menuItem), _("Launch"));
			clutter_actor_set_x_expand(menuItem, TRUE);
			esdashboard_popup_menu_add_item(ESDASHBOARD_POPUP_MENU(popup), ESDASHBOARD_POPUP_MENU_ITEM(menuItem));

			g_signal_connect(menuItem,
								"activated",
								G_CALLBACK(_esdashboard_applications_view_on_popup_menu_item_launch),
								appInfo);
		}
		g_object_unref(appTracker);

		/* Add application actions */
		esdashboard_application_button_add_popup_menu_items_for_actions(button, ESDASHBOARD_POPUP_MENU(popup));

		/* Activate pop-up menu */
		esdashboard_popup_menu_activate(ESDASHBOARD_POPUP_MENU(popup));
	}
}

/* Parent menu of "All applications" was clicked */
static void _esdashboard_applications_view_on_all_applications_menu_parent_menu_clicked(EsdashboardApplicationsView *self, gpointer inUserData)
{
	EsdashboardApplicationsViewPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=self->priv;

	/* Go to root menu */
	priv->currentRootMenuElement=NULL;
	esdashboard_applications_menu_model_filter_by_section(priv->apps, NULL);
	esdashboard_view_scroll_to(ESDASHBOARD_VIEW(self), -1, 0);
}

/* Show sub-menu with all installed applications */
static gint _esdashboard_applications_view_on_all_applications_sort_app_info(EsdashboardDesktopAppInfo *inLeft, EsdashboardDesktopAppInfo *inRight)
{
	GAppInfo		*leftAppInfo;
	GAppInfo		*rightAppInfo;
	GFile			*leftFile;
	GFile			*rightFile;
	gchar			*leftValue;
	gchar			*rightValue;
	gint			result;

	g_return_val_if_fail(ESDASHBOARD_IS_DESKTOP_APP_INFO(inLeft), 1);
	g_return_val_if_fail(ESDASHBOARD_IS_DESKTOP_APP_INFO(inRight), -1);

	/* Check if both desktop app info are valid */
	if(!esdashboard_desktop_app_info_is_valid(inLeft)) return(1);
	if(!esdashboard_desktop_app_info_is_valid(inRight)) return(-1);

	/* If both desktop app info share the same file they are equal */
	leftFile=esdashboard_desktop_app_info_get_file(inLeft);
	rightFile=esdashboard_desktop_app_info_get_file(inRight);
	if(g_file_equal(leftFile, rightFile)) return(0);

	/* Both desktop app info have different files so check if they differ in
	 * name, display name, description or command.
	 */
	leftAppInfo=G_APP_INFO(inLeft);
	rightAppInfo=G_APP_INFO(inRight);

	leftValue=g_utf8_strdown(g_app_info_get_name(leftAppInfo), -1);
	rightValue=g_utf8_strdown(g_app_info_get_name(rightAppInfo), -1);
	result=g_strcmp0(leftValue, rightValue);
	g_free(rightValue);
	g_free(leftValue);
	if(result!=0) return(result);

	leftValue=g_utf8_strdown(g_app_info_get_display_name(leftAppInfo), -1);
	rightValue=g_utf8_strdown(g_app_info_get_display_name(rightAppInfo), -1);
	result=g_strcmp0(leftValue, rightValue);
	g_free(rightValue);
	g_free(leftValue);
	if(result!=0) return(result);

	leftValue=g_utf8_strdown(g_app_info_get_description(leftAppInfo), -1);
	rightValue=g_utf8_strdown(g_app_info_get_description(rightAppInfo), -1);
	result=g_strcmp0(leftValue, rightValue);
	g_free(rightValue);
	g_free(leftValue);
	if(result!=0) return(result);

	leftValue=g_utf8_strdown(g_app_info_get_executable(leftAppInfo), -1);
	rightValue=g_utf8_strdown(g_app_info_get_executable(rightAppInfo), -1);
	result=g_strcmp0(leftValue, rightValue);
	g_free(rightValue);
	g_free(leftValue);
	if(result!=0) return(result);

	leftValue=g_utf8_strdown(g_app_info_get_commandline(leftAppInfo), -1);
	rightValue=g_utf8_strdown(g_app_info_get_commandline(rightAppInfo), -1);
	result=g_strcmp0(leftValue, rightValue);
	g_free(rightValue);
	g_free(leftValue);
	if(result!=0) return(result);

	/* If we get here both desktop app infos are equal because all checks passed */
	return(0);
}

static void _esdashboard_applications_view_on_all_applications_menu_clicked(EsdashboardApplicationsView *self, gpointer inUserData)
{
	EsdashboardApplicationsViewPrivate	*priv;
	ClutterActor						*actor;
	GList								*allApps;
	GList								*iter;
	EsdashboardDesktopAppInfo			*appInfo;
	EsdashboardApplicationDatabase		*appDB;
	ClutterAction						*clickAction;
	ClutterAction						*dragAction;
	gchar								*actorText;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=self->priv;

	/* Destroy all children */
	esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), NULL);
	esdashboard_actor_destroy_all_children(CLUTTER_ACTOR(self));
	clutter_layout_manager_layout_changed(priv->layout);

	/* Create parent menu item */
	actor=esdashboard_button_new();

	if(priv->parentMenuIcon) esdashboard_label_set_icon_name(ESDASHBOARD_LABEL(actor), priv->parentMenuIcon);

	if(priv->viewMode==ESDASHBOARD_VIEW_MODE_LIST) actorText=g_markup_printf_escaped(priv->formatTitleDescription, _("Back"), _("Go back to previous menu"));
		else actorText=g_markup_printf_escaped(priv->formatTitleOnly, _("Back"));
	esdashboard_label_set_text(ESDASHBOARD_LABEL(actor), actorText);
	g_free(actorText);

	/* Add to view and layout */
	_esdashboard_applications_view_setup_actor_for_view_mode(self, CLUTTER_ACTOR(actor));
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(actor));
	clutter_actor_show(actor);

	g_signal_connect_swapped(actor,
								"clicked",
								G_CALLBACK(_esdashboard_applications_view_on_all_applications_menu_parent_menu_clicked),
								self);

	/* Select "parent menu" automatically */
	if(esdashboard_view_has_focus(ESDASHBOARD_VIEW(self)))
	{
		esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), CLUTTER_ACTOR(actor));
	}

	/* Create menu items for all installed applications */
	appDB=esdashboard_application_database_get_default();

	allApps=esdashboard_application_database_get_all_applications(appDB);
	allApps=g_list_sort(allApps, (GCompareFunc)_esdashboard_applications_view_on_all_applications_sort_app_info);

	for(iter=allApps; iter; iter=g_list_next(iter))
	{
		/* Get app info of application currently iterated */
		appInfo=ESDASHBOARD_DESKTOP_APP_INFO(iter->data);

		/* If desktop app info should be hidden then continue with next one */
		if(!g_app_info_should_show(G_APP_INFO(appInfo)))
		{
			continue;
		}

		/* Create actor for app info */
		actor=esdashboard_application_button_new_from_app_info(G_APP_INFO(appInfo));

		g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_esdashboard_applications_view_on_item_clicked), self);

		/* Set up and add pop-up menu click action */
		clickAction=esdashboard_click_action_new();
		g_signal_connect_swapped(clickAction, "clicked", G_CALLBACK(_esdashboard_applications_view_on_popup_menu), self);
		clutter_actor_add_action(actor, clickAction);

		/* Add to view and layout */
		_esdashboard_applications_view_setup_actor_for_view_mode(self, CLUTTER_ACTOR(actor));
		clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(actor));
		clutter_actor_show(actor);

		/* Add drag action to actor */
		dragAction=esdashboard_drag_action_new_with_source(CLUTTER_ACTOR(self));
		clutter_drag_action_set_drag_threshold(CLUTTER_DRAG_ACTION(dragAction), -1, -1);
		clutter_actor_add_action(actor, dragAction);
		g_signal_connect(dragAction, "drag-begin", G_CALLBACK(_esdashboard_applications_view_on_drag_begin), self);
		g_signal_connect(dragAction, "drag-end", G_CALLBACK(_esdashboard_applications_view_on_drag_end), self);

		/* If no item was selected (i.e. no "parent menu" item) select this one
		 * which is usually the first menu item.
		 */
		if(esdashboard_view_has_focus(ESDASHBOARD_VIEW(self)) &&
			!esdashboard_focusable_get_selection(ESDASHBOARD_FOCUSABLE(self)))
		{
			esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), CLUTTER_ACTOR(actor));
		}
	}

	g_list_free_full(allApps, g_object_unref);
	g_object_unref(appDB);
}

/* Filter to display applications has changed */
static void _esdashboard_applications_view_on_filter_changed(EsdashboardApplicationsView *self, gpointer inUserData)
{
	EsdashboardApplicationsViewPrivate	*priv;
	EsdashboardModelIter				*iterator;
	ClutterActor						*actor;
	MarkonMenuElement					*menuElement=NULL;
	MarkonMenu							*parentMenu=NULL;
	ClutterAction						*clickAction;
	ClutterAction						*dragAction;
	GAppInfo							*appInfo;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=ESDASHBOARD_APPLICATIONS_VIEW(self)->priv;

	/* Destroy all children */
	esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), NULL);
	esdashboard_actor_destroy_all_children(CLUTTER_ACTOR(self));
	clutter_layout_manager_layout_changed(priv->layout);

	/* Get parent menu */
	if(priv->currentRootMenuElement &&
		MARKON_IS_MENU(priv->currentRootMenuElement))
	{
		parentMenu=markon_menu_get_parent(MARKON_MENU(priv->currentRootMenuElement));
	}

	/* If menu element to filter by is not the root menu element, add an "up ..." entry */
	if(parentMenu)
	{
		gchar					*actorText;

		/* Create and adjust of "parent menu" button to application buttons */
		actor=esdashboard_button_new();

		if(priv->parentMenuIcon) esdashboard_label_set_icon_name(ESDASHBOARD_LABEL(actor), priv->parentMenuIcon);

		if(priv->viewMode==ESDASHBOARD_VIEW_MODE_LIST) actorText=g_markup_printf_escaped(priv->formatTitleDescription, _("Back"), _("Go back to previous menu"));
			else actorText=g_markup_printf_escaped(priv->formatTitleOnly, _("Back"));
		esdashboard_label_set_text(ESDASHBOARD_LABEL(actor), actorText);
		g_free(actorText);

		/* Add to view and layout */
		_esdashboard_applications_view_setup_actor_for_view_mode(self, CLUTTER_ACTOR(actor));
		clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(actor));
		clutter_actor_show(actor);

		g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_esdashboard_applications_view_on_parent_menu_clicked), self);

		/* Select "parent menu" automatically */
		if(esdashboard_view_has_focus(ESDASHBOARD_VIEW(self)))
		{
			esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), CLUTTER_ACTOR(actor));
		}
	}

	if(priv->showAllAppsMenu &&
		(!priv->currentRootMenuElement || !parentMenu))
	{
		gchar					*actorText;

		/* Create and adjust of "parent menu" button to application buttons */
		actor=esdashboard_button_new();
		esdashboard_label_set_icon_name(ESDASHBOARD_LABEL(actor), ALL_APPLICATIONS_MENU_ICON);

		if(priv->viewMode==ESDASHBOARD_VIEW_MODE_LIST) actorText=g_markup_printf_escaped(priv->formatTitleDescription, _("All applications"), _("List of all installed applications"));
			else actorText=g_markup_printf_escaped(priv->formatTitleOnly, _("All applications"));
		esdashboard_label_set_text(ESDASHBOARD_LABEL(actor), actorText);
		g_free(actorText);

		/* Add to view and layout */
		_esdashboard_applications_view_setup_actor_for_view_mode(self, CLUTTER_ACTOR(actor));
		clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(actor));
		clutter_actor_show(actor);

		g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_esdashboard_applications_view_on_all_applications_menu_clicked), self);

		/* Select "parent menu" automatically */
		if(esdashboard_view_has_focus(ESDASHBOARD_VIEW(self)))
		{
			esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), CLUTTER_ACTOR(actor));
		}
	}

	/* Iterate through (filtered) data model and create actor for each entry */
	iterator=esdashboard_model_iter_new(ESDASHBOARD_MODEL(priv->apps));
	if(iterator)
	{
		while(esdashboard_model_iter_next(iterator))
		{
			/* If row is filtered continue with next one immediately */
			if(!esdashboard_model_iter_filter(iterator)) continue;

			/* Get data from model */
			esdashboard_applications_menu_model_get(priv->apps,
													iterator,
													ESDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
													-1);

			if(!menuElement) continue;

			/* Create actor for menu element. Support drag'n'drop at actor if
			 * menu element is a menu item.
			 */
			if(MARKON_IS_MENU_ITEM(menuElement))
			{
				appInfo=esdashboard_desktop_app_info_new_from_menu_item(MARKON_MENU_ITEM(menuElement));
				actor=esdashboard_application_button_new_from_app_info(appInfo);
				g_object_unref(appInfo);

				g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_esdashboard_applications_view_on_item_clicked), self);
			}
				else
				{
					gchar		*actorText;
					const gchar	*iconName;
					const gchar	*title;
					const gchar	*description;

					actor=esdashboard_button_new();

					iconName=markon_menu_element_get_icon_name(menuElement);
					if(iconName) esdashboard_label_set_icon_name(ESDASHBOARD_LABEL(actor), iconName);

					title=markon_menu_element_get_name(menuElement);
					description=markon_menu_element_get_comment(menuElement);

					if(priv->viewMode==ESDASHBOARD_VIEW_MODE_LIST)
					{
						actorText=g_markup_printf_escaped(priv->formatTitleDescription,
															title ? title : "",
															description ? description : "");
					}
						else
						{
							actorText=g_markup_printf_escaped(priv->formatTitleOnly,
																title ? title : "");
						}
					esdashboard_label_set_text(ESDASHBOARD_LABEL(actor), actorText);
					g_free(actorText);

					g_signal_connect(actor, "clicked", G_CALLBACK(_esdashboard_applications_view_on_menu_clicked), menuElement);
				}

			/* Add to view and layout */
			_esdashboard_applications_view_setup_actor_for_view_mode(self, CLUTTER_ACTOR(actor));
			clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(actor));
			clutter_actor_show(actor);

			/* Set up and add pop-up menu click action and drag action */
			if(MARKON_IS_MENU_ITEM(menuElement))
			{
				clickAction=esdashboard_click_action_new();
				g_signal_connect_swapped(clickAction, "clicked", G_CALLBACK(_esdashboard_applications_view_on_popup_menu), self);
				clutter_actor_add_action(actor, clickAction);

				dragAction=esdashboard_drag_action_new_with_source(CLUTTER_ACTOR(self));
				clutter_drag_action_set_drag_threshold(CLUTTER_DRAG_ACTION(dragAction), -1, -1);
				clutter_actor_add_action(actor, dragAction);
				g_signal_connect(dragAction, "drag-begin", G_CALLBACK(_esdashboard_applications_view_on_drag_begin), self);
				g_signal_connect(dragAction, "drag-end", G_CALLBACK(_esdashboard_applications_view_on_drag_end), self);
			}

			/* If no item was selected (i.e. no "parent menu" item) select this one
			 * which is usually the first menu item.
			 */
			if(esdashboard_view_has_focus(ESDASHBOARD_VIEW(self)) &&
				!esdashboard_focusable_get_selection(ESDASHBOARD_FOCUSABLE(self)))
			{
				esdashboard_focusable_set_selection(ESDASHBOARD_FOCUSABLE(self), CLUTTER_ACTOR(actor));
			}

			/* Release allocated resources */
			g_object_unref(menuElement);
			menuElement=NULL;
		}
		g_object_unref(iterator);
	}
}

/* Application model has fully loaded */
static void _esdashboard_applications_view_on_model_loaded(EsdashboardApplicationsView *self, gpointer inUserData)
{
	EsdashboardApplicationsViewPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=ESDASHBOARD_APPLICATIONS_VIEW(self)->priv;

	/* Reset to root menu as menu referenced will not be available anymore
	 * and re-filter to update view
	 */
	priv->currentRootMenuElement=NULL;
	esdashboard_applications_menu_model_filter_by_section(priv->apps, MARKON_MENU(priv->currentRootMenuElement));
}

/* The application will be resumed */
static void _esdashboard_applications_view_on_application_resume(EsdashboardApplicationsView *self, gpointer inUserData)
{
	EsdashboardApplicationsViewPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;

	/* Go to top-level entry */
	priv->currentRootMenuElement=NULL;
	esdashboard_applications_menu_model_filter_by_section(priv->apps, NULL);
}

/* IMPLEMENTATION: Interface EsdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _esdashboard_applications_view_focusable_can_focus(EsdashboardFocusable *inFocusable)
{
	EsdashboardApplicationsView			*self;
	EsdashboardFocusableInterface		*selfIface;
	EsdashboardFocusableInterface		*parentIface;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(inFocusable), FALSE);

	self=ESDASHBOARD_APPLICATIONS_VIEW(inFocusable);

	/* Call parent class interface function */
	selfIface=ESDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->can_focus)
	{
		if(!parentIface->can_focus(inFocusable)) return(FALSE);
	}

	/* If this view is not enabled it is not focusable */
	if(!esdashboard_view_get_enabled(ESDASHBOARD_VIEW(self))) return(FALSE);

	/* If we get here this actor can be focused */
	return(TRUE);
}

/* Determine if this actor supports selection */
static gboolean _esdashboard_applications_view_focusable_supports_selection(EsdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}

/* Get current selection */
static ClutterActor* _esdashboard_applications_view_focusable_get_selection(EsdashboardFocusable *inFocusable)
{
	EsdashboardApplicationsView				*self;
	EsdashboardApplicationsViewPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(inFocusable), NULL);

	self=ESDASHBOARD_APPLICATIONS_VIEW(inFocusable);
	priv=self->priv;

	/* Return current selection */
	return(priv->selectedItem);
}

/* Set new selection */
static gboolean _esdashboard_applications_view_focusable_set_selection(EsdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	EsdashboardApplicationsView				*self;
	EsdashboardApplicationsViewPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_APPLICATIONS_VIEW(inFocusable);
	priv=self->priv;

	/* Check that selection is a child of this actor */
	if(inSelection &&
		!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		g_warning("%s is not a child of %s and cannot be selected",
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Remove weak reference at current selection */
	if(priv->selectedItem)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
	}

	/* Set new selection */
	priv->selectedItem=inSelection;
	if(priv->selectedItem)
	{
		/* Add weak reference at new selection */
		g_object_add_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);

		/* Ensure new selection is visible */
		esdashboard_view_child_ensure_visible(ESDASHBOARD_VIEW(self), priv->selectedItem);
	}

	/* New selection was set successfully */
	return(TRUE);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _esdashboard_applications_view_get_selection_from_icon_mode(EsdashboardApplicationsView *self,
																				ClutterActor *inSelection,
																				EsdashboardSelectionTarget inDirection)
{
	EsdashboardApplicationsViewPrivate		*priv;
	ClutterActor							*selection;
	ClutterActor							*newSelection;
	gint									numberChildren;
	gint									rows;
	gint									columns;
	gint									currentSelectionIndex;
	gint									currentSelectionRow;
	gint									currentSelectionColumn;
	gint									newSelectionIndex;
	ClutterActorIter						iter;
	ClutterActor							*child;

	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), NULL);

	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;

	/* Get number of rows and columns and also get number of children
	 * of layout manager.
	 */
	numberChildren=esdashboard_dynamic_table_layout_get_number_children(ESDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout));
	rows=esdashboard_dynamic_table_layout_get_rows(ESDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout));
	columns=esdashboard_dynamic_table_layout_get_columns(ESDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout));

	/* Get index of current selection */
	currentSelectionIndex=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child) &&
			child!=inSelection)
	{
		currentSelectionIndex++;
	}

	currentSelectionRow=(currentSelectionIndex / columns);
	currentSelectionColumn=(currentSelectionIndex % columns);

	/* Find target selection */
	switch(inDirection)
	{
		case ESDASHBOARD_SELECTION_TARGET_LEFT:
			currentSelectionColumn--;
			if(currentSelectionColumn<0)
			{
				currentSelectionRow++;
				newSelectionIndex=(currentSelectionRow*columns)-1;
			}
				else newSelectionIndex=currentSelectionIndex-1;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_RIGHT:
			currentSelectionColumn++;
			if(currentSelectionColumn==columns ||
				currentSelectionIndex==numberChildren)
			{
				newSelectionIndex=(currentSelectionRow*columns);
			}
				else newSelectionIndex=currentSelectionIndex+1;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_UP:
			currentSelectionRow--;
			if(currentSelectionRow<0) currentSelectionRow=rows-1;
			newSelectionIndex=(currentSelectionRow*columns)+currentSelectionColumn;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_DOWN:
			currentSelectionRow++;
			if(currentSelectionRow>=rows) currentSelectionRow=0;
			newSelectionIndex=(currentSelectionRow*columns)+currentSelectionColumn;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
			newSelectionIndex=(currentSelectionRow*columns);
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
			newSelectionIndex=((currentSelectionRow+1)*columns)-1;
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_PAGE_UP:
			newSelectionIndex=currentSelectionColumn;
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		case ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			newSelectionIndex=((rows-1)*columns)+currentSelectionColumn;
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(CLUTTER_ACTOR(self), newSelectionIndex);
			break;

		default:
			{
				gchar					*valueName;

				valueName=esdashboard_get_enum_value_name(ESDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical("Focusable object %s does not handle selection direction of type %s in icon mode.",
							G_OBJECT_TYPE_NAME(self),
							valueName);
				g_free(valueName);
			}
			break;
	}

	/* If new selection could be found override current selection with it */
	if(newSelection) selection=newSelection;

	/* Return new selection */
	ESDASHBOARD_DEBUG(self, ACTOR,
						"Selecting %s at %s for current selection %s in direction %u",
						selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
						G_OBJECT_TYPE_NAME(self),
						inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
						inDirection);
	return(selection);
}

static ClutterActor* _esdashboard_applications_view_get_selection_from_list_mode(EsdashboardApplicationsView *self,
																				ClutterActor *inSelection,
																				EsdashboardSelectionTarget inDirection)
{
	ClutterActor							*selection;
	ClutterActor							*newSelection;

	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), NULL);

	selection=inSelection;
	newSelection=NULL;

	/* Find target selection */
	switch(inDirection)
	{
		case ESDASHBOARD_SELECTION_TARGET_LEFT:
		case ESDASHBOARD_SELECTION_TARGET_RIGHT:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
			/* Do nothing here in list mode */
			break;

		case ESDASHBOARD_SELECTION_TARGET_UP:
			newSelection=clutter_actor_get_previous_sibling(inSelection);
			if(!newSelection) newSelection=clutter_actor_get_last_child(CLUTTER_ACTOR(self));
			break;

		case ESDASHBOARD_SELECTION_TARGET_DOWN:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			if(!newSelection) newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(self));
			break;

		case ESDASHBOARD_SELECTION_TARGET_PAGE_UP:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			{
				ClutterActor				*child;
				gfloat						topY;
				gfloat						bottomY;
				gfloat						pageSize;
				gfloat						currentY;
				gfloat						limitY;
				gfloat						childY1, childY2;
				ClutterActorIter			iter;

				/* Beginning from current selection go up and first child which needs scrolling */
				child=clutter_actor_get_previous_sibling(inSelection);
				while(child && !esdashboard_view_child_needs_scroll(ESDASHBOARD_VIEW(self), child))
				{
					child=clutter_actor_get_previous_sibling(child);
				}
				if(!child) child=clutter_actor_get_first_child(CLUTTER_ACTOR(self));
				topY=clutter_actor_get_y(child);

				/* Beginning from current selection go down and first child which needs scrolling */
				child=clutter_actor_get_next_sibling(inSelection);
				while(child && !esdashboard_view_child_needs_scroll(ESDASHBOARD_VIEW(self), child))
				{
					child=clutter_actor_get_next_sibling(child);
				}
				if(!child) child=clutter_actor_get_last_child(CLUTTER_ACTOR(self));
				bottomY=clutter_actor_get_y(child);

				/* Get distance between top and bottom actor we found because that's the page size */
				pageSize=bottomY-topY;

				/* Find child in distance of page size from current selection */
				currentY=clutter_actor_get_y(inSelection);

				if(inDirection==ESDASHBOARD_SELECTION_TARGET_PAGE_UP) limitY=currentY-pageSize;
					else limitY=currentY+pageSize;

				clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
				while(!newSelection && clutter_actor_iter_next(&iter, &child))
				{
					childY1=clutter_actor_get_y(child);
					childY2=childY1+clutter_actor_get_height(child);
					if(childY1>limitY || childY2>limitY) newSelection=child;
				}

				/* If no child could be found select last one */
				if(!newSelection)
				{
					if(inDirection==ESDASHBOARD_SELECTION_TARGET_PAGE_UP)
					{
						newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(self));
					}
						else
						{
							newSelection=clutter_actor_get_last_child(CLUTTER_ACTOR(self));
						}
				}
			}
			break;

		default:
			{
				gchar					*valueName;

				valueName=esdashboard_get_enum_value_name(ESDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical("Focusable object %s does not handle selection direction of type %s in list mode.",
							G_OBJECT_TYPE_NAME(self),
							valueName);
				g_free(valueName);
			}
			break;
	}

	/* If new selection could be found override current selection with it */
	if(newSelection) selection=newSelection;

	/* Return new selection */
	ESDASHBOARD_DEBUG(self, ACTOR,
						"Selecting %s at %s for current selection %s in direction %u",
						selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
						G_OBJECT_TYPE_NAME(self),
						inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
						inDirection);
	return(selection);
}

static ClutterActor* _esdashboard_applications_view_focusable_find_selection(EsdashboardFocusable *inFocusable,
																				ClutterActor *inSelection,
																				EsdashboardSelectionTarget inDirection)
{
	EsdashboardApplicationsView				*self;
	EsdashboardApplicationsViewPrivate		*priv;
	ClutterActor							*selection;
	ClutterActor							*newSelection;
	gchar									*valueName;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(inFocusable), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	self=ESDASHBOARD_APPLICATIONS_VIEW(inFocusable);
	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;

	/* If there is nothing selected, select first actor and return */
	if(!inSelection)
	{
		newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(self));

		valueName=esdashboard_get_enum_value_name(ESDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
		ESDASHBOARD_DEBUG(self, ACTOR,
							"No selection at %s, so select first child %s for direction %s",
							G_OBJECT_TYPE_NAME(self),
							newSelection ? G_OBJECT_TYPE_NAME(newSelection) : "<nil>",
							valueName);
		g_free(valueName);

		return(newSelection);
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
		case ESDASHBOARD_SELECTION_TARGET_RIGHT:
		case ESDASHBOARD_SELECTION_TARGET_UP:
		case ESDASHBOARD_SELECTION_TARGET_DOWN:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_UP:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			if(priv->viewMode==ESDASHBOARD_VIEW_MODE_LIST)
			{
				newSelection=_esdashboard_applications_view_get_selection_from_list_mode(self, inSelection, inDirection);
			}
				else
				{
					newSelection=_esdashboard_applications_view_get_selection_from_icon_mode(self, inSelection, inDirection);
				}
			break;

		case ESDASHBOARD_SELECTION_TARGET_FIRST:
			newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(self));
			break;

		case ESDASHBOARD_SELECTION_TARGET_LAST:
			newSelection=clutter_actor_get_last_child(CLUTTER_ACTOR(self));
			break;

		case ESDASHBOARD_SELECTION_TARGET_NEXT:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			if(!newSelection) newSelection=clutter_actor_get_previous_sibling(inSelection);
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
static gboolean _esdashboard_applications_view_focusable_activate_selection(EsdashboardFocusable *inFocusable,
																			ClutterActor *inSelection)
{
	EsdashboardApplicationsView				*self;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(inFocusable), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_APPLICATIONS_VIEW(inFocusable);

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
void _esdashboard_applications_view_focusable_iface_init(EsdashboardFocusableInterface *iface)
{
	iface->can_focus=_esdashboard_applications_view_focusable_can_focus;

	iface->supports_selection=_esdashboard_applications_view_focusable_supports_selection;
	iface->get_selection=_esdashboard_applications_view_focusable_get_selection;
	iface->set_selection=_esdashboard_applications_view_focusable_set_selection;
	iface->find_selection=_esdashboard_applications_view_focusable_find_selection;
	iface->activate_selection=_esdashboard_applications_view_focusable_activate_selection;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_applications_view_dispose(GObject *inObject)
{
	EsdashboardApplicationsView			*self=ESDASHBOARD_APPLICATIONS_VIEW(inObject);
	EsdashboardApplicationsViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->selectedItem)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
		priv->selectedItem=NULL;
	}

	if(priv->esconfChannel)
	{
		priv->esconfChannel=NULL;
	}

	if(priv->layout)
	{
		priv->layout=NULL;
	}

	if(priv->esconfShowAllAppsMenuBindingID)
	{
		esconf_g_property_unbind(priv->esconfShowAllAppsMenuBindingID);
		priv->esconfShowAllAppsMenuBindingID=0;
	}

	if(priv->apps)
	{
		g_object_unref(priv->apps);
		priv->apps=NULL;
	}

	if(priv->parentMenuIcon)
	{
		g_free(priv->parentMenuIcon);
		priv->parentMenuIcon=NULL;
	}

	if(priv->formatTitleDescription)
	{
		g_free(priv->formatTitleDescription);
		priv->formatTitleDescription=NULL;
	}

	if(priv->formatTitleOnly)
	{
		g_free(priv->formatTitleOnly);
		priv->formatTitleOnly=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_applications_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_applications_view_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	EsdashboardApplicationsView				*self;

	self=ESDASHBOARD_APPLICATIONS_VIEW(inObject);

	switch(inPropID)
	{
		case PROP_VIEW_MODE:
			esdashboard_applications_view_set_view_mode(self, (EsdashboardViewMode)g_value_get_enum(inValue));
			break;

		case PROP_SPACING:
			esdashboard_applications_view_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_PARENT_MENU_ICON:
			esdashboard_applications_view_set_parent_menu_icon(self, g_value_get_string(inValue));
			break;

		case PROP_FORMAT_TITLE_ONLY:
			esdashboard_applications_view_set_format_title_only(self, g_value_get_string(inValue));
			break;

		case PROP_FORMAT_TITLE_DESCRIPTION:
			esdashboard_applications_view_set_format_title_description(self, g_value_get_string(inValue));
			break;

		case PROP_SHOW_ALL_APPS:
			esdashboard_applications_view_set_show_all_apps(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_applications_view_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	EsdashboardApplicationsView				*self;
	EsdashboardApplicationsViewPrivate		*priv;

	self=ESDASHBOARD_APPLICATIONS_VIEW(inObject);
	priv=self->priv;

	switch(inPropID)
	{
		case PROP_VIEW_MODE:
			g_value_set_enum(outValue, priv->viewMode);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_PARENT_MENU_ICON:
			g_value_set_string(outValue, priv->parentMenuIcon);
			break;

		case PROP_FORMAT_TITLE_ONLY:
			g_value_set_string(outValue, priv->formatTitleOnly);
			break;

		case PROP_FORMAT_TITLE_DESCRIPTION:
			g_value_set_string(outValue, priv->formatTitleDescription);
			break;

		case PROP_SHOW_ALL_APPS:
			g_value_set_boolean(outValue, priv->showAllAppsMenu);
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
static void esdashboard_applications_view_class_init(EsdashboardApplicationsViewClass *klass)
{
	EsdashboardActorClass	*actorClass=ESDASHBOARD_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_applications_view_dispose;
	gobjectClass->set_property=_esdashboard_applications_view_set_property;
	gobjectClass->get_property=_esdashboard_applications_view_get_property;

	/* Define properties */
	EsdashboardApplicationsViewProperties[PROP_VIEW_MODE]=
		g_param_spec_enum("view-mode",
							"View mode",
							"The view mode used in this view",
							ESDASHBOARD_TYPE_VIEW_MODE,
							ESDASHBOARD_VIEW_MODE_LIST,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardApplicationsViewProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							"Spacing",
							"Spacing between each element in view",
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardApplicationsViewProperties[PROP_PARENT_MENU_ICON]=
		g_param_spec_string("parent-menu-icon",
								"Parent menu icon",
								"Name of icon to use for 'go-back-to-parent-menu' entries",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardApplicationsViewProperties[PROP_FORMAT_TITLE_ONLY]=
		g_param_spec_string("format-title-only",
								"Format title only",
								"Format string used when only title is display",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardApplicationsViewProperties[PROP_FORMAT_TITLE_DESCRIPTION]=
		g_param_spec_string("format-title-description",
								"Format title and description",
								"Format string used when title and description is display. First argument is title and second one is description.",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	EsdashboardApplicationsViewProperties[PROP_SHOW_ALL_APPS]=
		g_param_spec_boolean("show-all-apps",
								"Show all applications",
								"Whether to show a menu for all installed applications at root menu",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardApplicationsViewProperties);

	/* Define stylable properties */
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardApplicationsViewProperties[PROP_VIEW_MODE]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardApplicationsViewProperties[PROP_SPACING]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardApplicationsViewProperties[PROP_PARENT_MENU_ICON]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardApplicationsViewProperties[PROP_FORMAT_TITLE_ONLY]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardApplicationsViewProperties[PROP_FORMAT_TITLE_DESCRIPTION]);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_applications_view_init(EsdashboardApplicationsView *self)
{
	EsdashboardApplicationsViewPrivate	*priv;
	EsdashboardApplication				*application;

	self->priv=priv=esdashboard_applications_view_get_instance_private(self);

	/* Set up default values */
	priv->apps=ESDASHBOARD_APPLICATIONS_MENU_MODEL(esdashboard_applications_menu_model_new());
	priv->currentRootMenuElement=NULL;
	priv->viewMode=-1;
	priv->spacing=0.0f;
	priv->parentMenuIcon=NULL;
	priv->formatTitleOnly=g_strdup("%s");
	priv->formatTitleDescription=g_strdup("%s\n%s");
	priv->selectedItem=NULL;
	priv->showAllAppsMenu=FALSE;
	priv->esconfChannel=esdashboard_application_get_esconf_channel(NULL);
	priv->esconfShowAllAppsMenuBindingID=0;

	/* Set up view */
	esdashboard_view_set_name(ESDASHBOARD_VIEW(self), _("Applications"));
	esdashboard_view_set_icon(ESDASHBOARD_VIEW(self), "go-home");

	/* Set up actor */
	esdashboard_actor_set_can_focus(ESDASHBOARD_ACTOR(self), TRUE);

	esdashboard_view_set_view_fit_mode(ESDASHBOARD_VIEW(self), ESDASHBOARD_VIEW_FIT_MODE_HORIZONTAL);
	esdashboard_applications_view_set_view_mode(self, ESDASHBOARD_VIEW_MODE_LIST);

	/* Connect signals */
	g_signal_connect_swapped(priv->apps, "filter-changed", G_CALLBACK(_esdashboard_applications_view_on_filter_changed), self);
	g_signal_connect_swapped(priv->apps, "loaded", G_CALLBACK(_esdashboard_applications_view_on_model_loaded), self);

	/* Connect signal to application */
	application=esdashboard_application_get_default();
	g_signal_connect_swapped(application, "resume", G_CALLBACK(_esdashboard_applications_view_on_application_resume), self);

	/* Bind to esconf to react on changes */
	priv->esconfShowAllAppsMenuBindingID=esconf_g_property_bind(priv->esconfChannel,
																SHOW_ALL_APPS_ESCONF_PROP,
																G_TYPE_BOOLEAN,
																self,
																"show-all-apps");
}

/* Get/set view mode of view */
EsdashboardViewMode esdashboard_applications_view_get_view_mode(EsdashboardApplicationsView *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self), ESDASHBOARD_VIEW_MODE_LIST);

	return(self->priv->viewMode);
}

void esdashboard_applications_view_set_view_mode(EsdashboardApplicationsView *self, const EsdashboardViewMode inMode)
{
	EsdashboardApplicationsViewPrivate	*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(inMode<=ESDASHBOARD_VIEW_MODE_ICON);

	priv=self->priv;

	/* Set value if changed */
	if(priv->viewMode!=inMode)
	{
		/* Set value */
		if(priv->layout)
		{
			clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), NULL);
			priv->layout=NULL;
		}

		priv->viewMode=inMode;

		/* Set new layout manager */
		switch(priv->viewMode)
		{
			case ESDASHBOARD_VIEW_MODE_LIST:
				priv->layout=clutter_box_layout_new();
				clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(priv->layout), CLUTTER_ORIENTATION_VERTICAL);
				clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), priv->spacing);
				clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
				break;

			case ESDASHBOARD_VIEW_MODE_ICON:
				priv->layout=esdashboard_dynamic_table_layout_new();
				esdashboard_dynamic_table_layout_set_spacing(ESDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout), priv->spacing);
				clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
				break;

			default:
				g_assert_not_reached();
		}

		/* Rebuild view */
		_esdashboard_applications_view_on_filter_changed(self, NULL);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationsViewProperties[PROP_VIEW_MODE]);
	}
}

/* Get/set spacing between elements */
gfloat esdashboard_applications_view_get_spacing(EsdashboardApplicationsView *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self), 0.0f);

	return(self->priv->spacing);
}

void esdashboard_applications_view_set_spacing(EsdashboardApplicationsView *self, const gfloat inSpacing)
{
	EsdashboardApplicationsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set value */
		priv->spacing=inSpacing;

		/* Update layout manager */
		switch(priv->viewMode)
		{
			case ESDASHBOARD_VIEW_MODE_LIST:
				clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), priv->spacing);
				break;

			case ESDASHBOARD_VIEW_MODE_ICON:
				esdashboard_dynamic_table_layout_set_spacing(ESDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout), priv->spacing);
				break;

			default:
				g_assert_not_reached();
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationsViewProperties[PROP_SPACING]);
	}
}

/* IMPLEMENTATION: Public API */

/* Get/set icon name for 'go-back-to-parent-menu' entries */
const gchar* esdashboard_applications_view_get_parent_menu_icon(EsdashboardApplicationsView *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self), NULL);

	return(self->priv->parentMenuIcon);
}

void esdashboard_applications_view_set_parent_menu_icon(EsdashboardApplicationsView *self, const gchar *inIconName)
{
	EsdashboardApplicationsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->parentMenuIcon, inIconName)!=0)
	{
		/* Set value */
		if(priv->parentMenuIcon)
		{
			g_free(priv->parentMenuIcon);
			priv->parentMenuIcon=NULL;
		}

		if(inIconName) priv->parentMenuIcon=g_strdup(inIconName);

		/* Update actor */
		_esdashboard_applications_view_on_filter_changed(self, NULL);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationsViewProperties[PROP_PARENT_MENU_ICON]);
	}
}

/* Get/set format string to use when displaying only title */
const gchar* esdashboard_applications_view_get_format_title_only(EsdashboardApplicationsView *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self), NULL);

	return(self->priv->formatTitleOnly);
}

void esdashboard_applications_view_set_format_title_only(EsdashboardApplicationsView *self, const gchar *inFormat)
{
	EsdashboardApplicationsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(inFormat);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->formatTitleOnly, inFormat)!=0)
	{
		/* Set value */
		if(priv->formatTitleOnly) g_free(priv->formatTitleOnly);
		priv->formatTitleOnly=g_strdup(inFormat);

		/* Update view only if view mode is list which uses this format string */
		if(priv->viewMode==ESDASHBOARD_VIEW_MODE_ICON) _esdashboard_applications_view_on_filter_changed(self, NULL);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationsViewProperties[PROP_FORMAT_TITLE_ONLY]);
	}
}

/* Get/set format string to use when displaying title and description */
const gchar* esdashboard_applications_view_get_format_title_description(EsdashboardApplicationsView *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self), NULL);

	return(self->priv->formatTitleDescription);
}

void esdashboard_applications_view_set_format_title_description(EsdashboardApplicationsView *self, const gchar *inFormat)
{
	EsdashboardApplicationsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(inFormat);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->formatTitleDescription, inFormat)!=0)
	{
		/* Set value */
		if(priv->formatTitleDescription) g_free(priv->formatTitleDescription);
		priv->formatTitleDescription=g_strdup(inFormat);

		/* Update view only if view mode is list which uses this format string */
		if(priv->viewMode==ESDASHBOARD_VIEW_MODE_LIST) _esdashboard_applications_view_on_filter_changed(self, NULL);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationsViewProperties[PROP_FORMAT_TITLE_DESCRIPTION]);
	}
}

/* Get/set flag whether to show an "all applications" menu at root menu */
gboolean esdashboard_applications_view_get_show_all_apps(EsdashboardApplicationsView *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self), FALSE);

	return(self->priv->showAllAppsMenu);
}

void esdashboard_applications_view_set_show_all_apps(EsdashboardApplicationsView *self, gboolean inShowAllApps)
{
	EsdashboardApplicationsViewPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->showAllAppsMenu!=inShowAllApps)
	{
		/* Set value */
		priv->showAllAppsMenu=inShowAllApps;

		/* Update view if currently at root menu */
		if(!priv->currentRootMenuElement ||
			!markon_menu_get_parent(MARKON_MENU(priv->currentRootMenuElement)))
		{
			_esdashboard_applications_view_on_filter_changed(self, NULL);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardApplicationsViewProperties[PROP_SHOW_ALL_APPS]);
	}
}
