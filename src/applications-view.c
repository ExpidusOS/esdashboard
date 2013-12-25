/*
 * applications-view: A view showing all installed applications as menu
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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

#include "applications-view.h"
#include "utils.h"
#include "view.h"
#include "applications-menu-model.h"
#include "types.h"
#include "button.h"
#include "application.h"
#include "application-button.h"
#include "enums.h"
#include "drag-action.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationsView,
				xfdashboard_applications_view,
				XFDASHBOARD_TYPE_VIEW)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATIONS_VIEW, XfdashboardApplicationsViewPrivate))

struct _XfdashboardApplicationsViewPrivate
{
	/* Properties related */
	XfdashboardViewMode					viewMode;

	/* Instance related */
	ClutterLayoutManager				*layout;
	XfdashboardApplicationsMenuModel	*apps;
	GarconMenuElement					*currentRootMenuElement;
	XfdashboardApplicationButton		*appButton;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VIEW_MODE,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationsViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define ACTOR_USER_DATA_KEY			"xfdashboard-applications-view-user-data"

#define DEFAULT_VIEW_ICON			GTK_STOCK_HOME				// TODO: Replace by settings/theming object
#define DEFAULT_VIEW_MODE			XFDASHBOARD_VIEW_MODE_LIST	// TODO: Replace by settings/theming object
#define DEFAULT_SPACING				4.0f						// TODO: Replace by settings/theming object
#define DEFAULT_MENU_ICON_SIZE		64							// TODO: Replace by settings/theming object
#define DEFAULT_PARENT_MENU_ICON	GTK_STOCK_GO_UP				// TODO: Replace by settings/theming object

/* Forward declarations */
static void _xfdashboard_applications_view_on_item_clicked(XfdashboardApplicationsView *self, gpointer inUserData);

/* Drag of an menu item begins */
static void _xfdashboard_applications_view_on_drag_begin(ClutterDragAction *inAction,
															ClutterActor *inActor,
															gfloat inStageX,
															gfloat inStageY,
															ClutterModifierType inModifiers,
															gpointer inUserData)
{
	const gchar							*desktopName;
	ClutterActor						*dragHandle;
	ClutterStage						*stage;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

	/* Prevent signal "clicked" from being emitted on dragged icon */
	g_signal_handlers_block_by_func(inActor, _xfdashboard_applications_view_on_item_clicked, inUserData);

	/* Get stage */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));

	/* Create a application icon for drag handle */
	desktopName=xfdashboard_application_button_get_desktop_filename(XFDASHBOARD_APPLICATION_BUTTON(inActor));

	dragHandle=xfdashboard_application_button_new_from_desktop_file(desktopName);
	clutter_actor_set_position(dragHandle, inStageX, inStageY);
	xfdashboard_button_set_icon_size(XFDASHBOARD_BUTTON(dragHandle), DEFAULT_MENU_ICON_SIZE);
	xfdashboard_button_set_single_line_mode(XFDASHBOARD_BUTTON(dragHandle), FALSE);
	xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(dragHandle), FALSE);
	xfdashboard_button_set_style(XFDASHBOARD_BUTTON(dragHandle), XFDASHBOARD_STYLE_ICON);
	clutter_actor_add_child(CLUTTER_ACTOR(stage), dragHandle);

	clutter_drag_action_set_drag_handle(inAction, dragHandle);
}

/* Drag of an menu item ends */
static void _xfdashboard_applications_view_on_drag_end(ClutterDragAction *inAction,
														ClutterActor *inActor,
														gfloat inStageX,
														gfloat inStageY,
														ClutterModifierType inModifiers,
														gpointer inUserData)
{
	ClutterActor					*dragHandle;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

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
		clutter_actor_destroy(dragHandle);
	}

	/* Allow signal "clicked" from being emitted again */
	g_signal_handlers_unblock_by_func(inActor, _xfdashboard_applications_view_on_item_clicked, inUserData);
}

/* Update style of all child actors */
static void _xfdashboard_applications_view_add_button_for_list_mode(XfdashboardApplicationsView *self,
																	XfdashboardButton *inButton)
{
	XfdashboardApplicationsViewPrivate	*priv;
	const gchar							*actorFormat;
	gchar								*actorText;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(inButton));

	priv=self->priv;

	/* If button is a real application button set it up */
	if(XFDASHBOARD_IS_APPLICATION_BUTTON(inButton))
	{
		xfdashboard_application_button_set_show_description(XFDASHBOARD_APPLICATION_BUTTON(inButton), TRUE);
	}
		/* Otherwise it is a normal button for "go back" */
		else
		{
			/* Get text to set and format to use in "parent menu" button */
			actorFormat=xfdashboard_application_button_get_format_title_description(priv->appButton);
			actorText=g_markup_printf_escaped(actorFormat, _("Back"), _("Go back to previous menu"));
			xfdashboard_button_set_text(inButton, actorText);
			g_free(actorText);
		}

	xfdashboard_button_set_style(inButton, XFDASHBOARD_STYLE_BOTH);
	xfdashboard_button_set_icon_size(inButton, DEFAULT_MENU_ICON_SIZE);
	xfdashboard_button_set_single_line_mode(inButton, FALSE);
	xfdashboard_button_set_sync_icon_size(inButton, FALSE);
	xfdashboard_button_set_icon_orientation(inButton, XFDASHBOARD_ORIENTATION_LEFT);
	xfdashboard_button_set_text_justification(inButton, PANGO_ALIGN_LEFT);

	/* Add to view and layout */
	clutter_actor_set_x_expand(CLUTTER_ACTOR(inButton), TRUE);
	clutter_actor_set_y_expand(CLUTTER_ACTOR(inButton), TRUE);
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(inButton));
}

static void _xfdashboard_applications_view_add_button_for_icon_mode(XfdashboardApplicationsView *self,
																	XfdashboardButton *inButton)
{
	XfdashboardApplicationsViewPrivate	*priv;
	const gchar							*actorFormat;
	gchar								*actorText;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(inButton));

	priv=self->priv;

	/* If button is a real application button set it up */
	if(XFDASHBOARD_IS_APPLICATION_BUTTON(inButton))
	{
		xfdashboard_application_button_set_show_description(XFDASHBOARD_APPLICATION_BUTTON(inButton), FALSE);
	}
		/* Otherwise it is a normal button for "go back" */
		else
		{
			/* Get text to set and format to use in "parent menu" button */
			actorFormat=xfdashboard_application_button_get_format_title_only(priv->appButton);
			actorText=g_markup_printf_escaped(actorFormat, _("Back"));
			xfdashboard_button_set_text(inButton, actorText);
			g_free(actorText);
		}

	xfdashboard_button_set_icon_size(inButton, DEFAULT_MENU_ICON_SIZE);
	xfdashboard_button_set_single_line_mode(inButton, FALSE);
	xfdashboard_button_set_sync_icon_size(inButton, FALSE);
	xfdashboard_button_set_icon_orientation(inButton, XFDASHBOARD_ORIENTATION_TOP);
	xfdashboard_button_set_text_justification(inButton, PANGO_ALIGN_CENTER);

	/* Add to view and layout */
	clutter_actor_set_x_expand(CLUTTER_ACTOR(inButton), TRUE);
	clutter_actor_set_y_expand(CLUTTER_ACTOR(inButton), TRUE);
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(inButton));
}

/* Filter of applications data model has changed */
static void _xfdashboard_applications_view_on_parent_menu_clicked(XfdashboardApplicationsView *self, gpointer inUserData)
{
	XfdashboardApplicationsViewPrivate	*priv;
	GarconMenuElement					*element;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=self->priv;

	/* Get associated menu element of button */
	if(priv->currentRootMenuElement &&
		GARCON_IS_MENU(priv->currentRootMenuElement))
	{
		element=GARCON_MENU_ELEMENT(garcon_menu_get_parent(GARCON_MENU(priv->currentRootMenuElement)));

		priv->currentRootMenuElement=element;
		xfdashboard_applications_menu_model_filter_by_section(priv->apps, GARCON_MENU(element));
		xfdashboard_view_scroll_to(XFDASHBOARD_VIEW(self), -1, 0);
	}
}

static void _xfdashboard_applications_view_on_item_clicked(XfdashboardApplicationsView *self, gpointer inUserData)
{
	XfdashboardApplicationsViewPrivate	*priv;
	XfdashboardApplicationButton		*button;
	GarconMenuElement					*element;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	priv=self->priv;
	button=XFDASHBOARD_APPLICATION_BUTTON(inUserData);

	/* Get associated menu element of button */
	element=xfdashboard_application_button_get_menu_element(button);

	/* If clicked item is a menu set it as new parent one */
	if(GARCON_IS_MENU(element))
	{
		priv->currentRootMenuElement=element;
		xfdashboard_applications_menu_model_filter_by_section(priv->apps, GARCON_MENU(element));
		xfdashboard_view_scroll_to(XFDASHBOARD_VIEW(self), -1, 0);
	}
		/* If clicked item is a menu item execute command of menu item clicked
		 * and quit application
		 */
		else if(GARCON_IS_MENU_ITEM(element))
		{
			/* Launch application */
			if(xfdashboard_application_button_execute(button, NULL))
			{
				/* Launching application seems to be successfuly so quit application */
				xfdashboard_application_quit();
				return;
			}
		}
}

static void _xfdashboard_applications_view_on_filter_changed(XfdashboardApplicationsView *self, gpointer inUserData)
{
	XfdashboardApplicationsViewPrivate	*priv;
	ClutterModelIter					*iterator;
	ClutterActor						*actor;
	GarconMenuElement					*menuElement=NULL;
	GarconMenu							*parentMenu=NULL;
	ClutterAction						*dragAction;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;

	/* Destroy all children */
	clutter_actor_destroy_all_children(CLUTTER_ACTOR(self));
	clutter_layout_manager_layout_changed(priv->layout);

	/* Get parent menu */
	if(priv->currentRootMenuElement &&
		GARCON_IS_MENU(priv->currentRootMenuElement))
	{
		parentMenu=garcon_menu_get_parent(GARCON_MENU(priv->currentRootMenuElement));
	}

	/* If menu element to filter by is not the root menu element, add an "up ..." entry */
	if(parentMenu!=NULL)
	{
		/* Create and adjust of "parent menu" button to application buttons */
		actor=xfdashboard_button_new_with_icon(DEFAULT_PARENT_MENU_ICON);
		if(priv->viewMode==XFDASHBOARD_VIEW_MODE_LIST)
		{
			_xfdashboard_applications_view_add_button_for_list_mode(self, XFDASHBOARD_BUTTON(actor));
		}
			else
			{
				_xfdashboard_applications_view_add_button_for_icon_mode(self, XFDASHBOARD_BUTTON(actor));
			}
		clutter_actor_show(actor);
		g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_parent_menu_clicked), self);
	}

	/* Iterate through (filtered) data model and create actor for each entry */
	iterator=clutter_model_get_first_iter(CLUTTER_MODEL(priv->apps));
	if(iterator && CLUTTER_IS_MODEL_ITER(iterator))
	{
		while(!clutter_model_iter_is_last(iterator))
		{
			/* Get data from model */
			clutter_model_iter_get(iterator,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
									-1);

			if(!menuElement) continue;

			/* Create actor for menu element. Support drag'n'drop at actor if
			 * menu element is a menu item.
			 */
			actor=xfdashboard_application_button_new_from_menu(menuElement);
			if(priv->viewMode==XFDASHBOARD_VIEW_MODE_LIST)
			{
				_xfdashboard_applications_view_add_button_for_list_mode(self, XFDASHBOARD_BUTTON(actor));
			}
				else
				{
					_xfdashboard_applications_view_add_button_for_icon_mode(self, XFDASHBOARD_BUTTON(actor));
				}
			clutter_actor_show(actor);
			g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_item_clicked), self);

			if(GARCON_IS_MENU_ITEM(menuElement))
			{
				dragAction=xfdashboard_drag_action_new_with_source(CLUTTER_ACTOR(self));
				clutter_drag_action_set_drag_threshold(CLUTTER_DRAG_ACTION(dragAction), -1, -1);
				clutter_actor_add_action(actor, dragAction);
				g_signal_connect(dragAction, "drag-begin", G_CALLBACK(_xfdashboard_applications_view_on_drag_begin), self);
				g_signal_connect(dragAction, "drag-end", G_CALLBACK(_xfdashboard_applications_view_on_drag_end), self);
			}

			/* Release allocated resources */
			g_object_unref(menuElement);
			menuElement=NULL;

			/* Go to next entry in model */
			iterator=clutter_model_iter_next(iterator);
		}
		g_object_unref(iterator);
	}
}

/* Application model has fully loaded */
static void _xfdashboard_applications_view_on_model_loaded(XfdashboardApplicationsView *self, gpointer inUserData)
{
	XfdashboardApplicationsViewPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;

	xfdashboard_applications_menu_model_filter_by_section(priv->apps, GARCON_MENU(priv->currentRootMenuElement));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_applications_view_dispose(GObject *inObject)
{
	XfdashboardApplicationsView			*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);
	XfdashboardApplicationsViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->layout) priv->layout=NULL;

	if(priv->apps)
	{
		g_object_unref(priv->apps);
		priv->apps=NULL;
	}

	if(priv->appButton)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->appButton));
		priv->appButton=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_applications_view_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardApplicationsView				*self;

	self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);

	switch(inPropID)
	{
		case PROP_VIEW_MODE:
			xfdashboard_applications_view_set_view_mode(self, (XfdashboardViewMode)g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_applications_view_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardApplicationsView				*self;
	XfdashboardApplicationsViewPrivate		*priv;

	self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);
	priv=self->priv;

	switch(inPropID)
	{
		case PROP_VIEW_MODE:
			g_value_set_enum(outValue, priv->viewMode);
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
static void xfdashboard_applications_view_class_init(XfdashboardApplicationsViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_applications_view_dispose;
	gobjectClass->set_property=_xfdashboard_applications_view_set_property;
	gobjectClass->get_property=_xfdashboard_applications_view_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationsViewPrivate));

	/* Define properties */
	XfdashboardApplicationsViewProperties[PROP_VIEW_MODE]=
		g_param_spec_enum("view-mode",
							_("View mode"),
							_("The view mode used in this view"),
							XFDASHBOARD_TYPE_VIEW_MODE,
							DEFAULT_VIEW_MODE,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationsViewProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_applications_view_init(XfdashboardApplicationsView *self)
{
	XfdashboardApplicationsViewPrivate	*priv;

	self->priv=priv=XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	priv->apps=XFDASHBOARD_APPLICATIONS_MENU_MODEL(xfdashboard_applications_menu_model_new());
	priv->currentRootMenuElement=NULL;
	priv->viewMode=-1;
	priv->appButton=XFDASHBOARD_APPLICATION_BUTTON(xfdashboard_application_button_new());

	/* Set up view */
	xfdashboard_view_set_internal_name(XFDASHBOARD_VIEW(self), "applications");
	xfdashboard_view_set_name(XFDASHBOARD_VIEW(self), _("Applications"));
	xfdashboard_view_set_icon(XFDASHBOARD_VIEW(self), DEFAULT_VIEW_ICON);

	/* Set up actor */
	xfdashboard_view_set_fit_mode(XFDASHBOARD_VIEW(self), XFDASHBOARD_FIT_MODE_HORIZONTAL);
	xfdashboard_applications_view_set_view_mode(self, DEFAULT_VIEW_MODE);
	clutter_model_set_sorting_column(CLUTTER_MODEL(priv->apps), XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE);

	/* Connect signals */
	g_signal_connect_swapped(priv->apps, "filter-changed", G_CALLBACK(_xfdashboard_applications_view_on_filter_changed), self);
	g_signal_connect_swapped(priv->apps, "loaded", G_CALLBACK(_xfdashboard_applications_view_on_model_loaded), self);
}

/* Get/set view mode of view */
XfdashboardViewMode xfdashboard_applications_view_get_view_mode(XfdashboardApplicationsView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self), DEFAULT_VIEW_MODE);

	return(self->priv->viewMode);
}

void xfdashboard_applications_view_set_view_mode(XfdashboardApplicationsView *self, const XfdashboardViewMode inMode)
{
	XfdashboardApplicationsViewPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(inMode<=XFDASHBOARD_VIEW_MODE_ICON);

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
			case XFDASHBOARD_VIEW_MODE_LIST:
				priv->layout=clutter_box_layout_new();
				clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(priv->layout), CLUTTER_ORIENTATION_VERTICAL);
				clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), DEFAULT_SPACING);
				clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
				break;

			case XFDASHBOARD_VIEW_MODE_ICON:
				priv->layout=clutter_flow_layout_new(CLUTTER_FLOW_HORIZONTAL);
				clutter_flow_layout_set_column_spacing(CLUTTER_FLOW_LAYOUT(priv->layout), DEFAULT_SPACING);
				clutter_flow_layout_set_row_spacing(CLUTTER_FLOW_LAYOUT(priv->layout), DEFAULT_SPACING);
				clutter_flow_layout_set_homogeneous(CLUTTER_FLOW_LAYOUT(priv->layout), TRUE);
				clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
				break;

			default:
				g_assert_not_reached();
		}

		/* Rebuild view */
		_xfdashboard_applications_view_on_filter_changed(self, NULL);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationsViewProperties[PROP_VIEW_MODE]);
	}
}
