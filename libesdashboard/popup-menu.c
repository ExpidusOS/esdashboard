/*
 * popup-menu: A pop-up menu with menu items performing an action when an menu
 *             item was clicked
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
 * SECTION:popup-menu
 * @short_description: A pop-up menu showing items and perfoming an action
                       when an item was clicked
 * @include: esdashboard/popup-menu.h
 *
 * A #EsdashboardPopupMenu implements a drop down menu consisting of a list of
 * #ClutterActor objects as menu items which can be navigated and activated by
 * the user to perform the associated action of the selected menu item.
 *
 * The following example shows how create and activate a #EsdashboardPopupMenu
 * when an actor was clicked.
 * application when clicked:
 *
 * |[<!-- language="C" -->
 * ]|
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libesdashboard/popup-menu.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <math.h>

#include <libesdashboard/box-layout.h>
#include <libesdashboard/focusable.h>
#include <libesdashboard/focus-manager.h>
#include <libesdashboard/stylable.h>
#include <libesdashboard/window-tracker.h>
#include <libesdashboard/application.h>
#include <libesdashboard/click-action.h>
#include <libesdashboard/bindings-pool.h>
#include <libesdashboard/button.h>
#include <libesdashboard/enums.h>
#include <libesdashboard/utils.h>
#include <libesdashboard/compat.h>
#include <libesdashboard/debug.h>


/* Define this class in GObject system */
static void _esdashboard_popup_menu_focusable_iface_init(EsdashboardFocusableInterface *iface);

struct _EsdashboardPopupMenuPrivate
{
	/* Properties related */
	gboolean						destroyOnCancel;

	ClutterActor					*source;

	gboolean						showTitle;
	gboolean						showTitleIcon;

	/* Instance related */
	gboolean						isActive;

	ClutterActor					*title;
	ClutterActor					*itemsContainer;

	EsdashboardWindowTracker		*windowTracker;

	EsdashboardFocusManager			*focusManager;
	gpointer						oldFocusable;
	gpointer						selectedItem;

	EsdashboardStage				*stage;
	guint							capturedEventSignalID;

	guint							sourceDestroySignalID;

	guint							suspendSignalID;
};

G_DEFINE_TYPE_WITH_CODE(EsdashboardPopupMenu,
						esdashboard_popup_menu,
						ESDASHBOARD_TYPE_BACKGROUND,
						G_ADD_PRIVATE(EsdashboardPopupMenu)
						G_IMPLEMENT_INTERFACE(ESDASHBOARD_TYPE_FOCUSABLE, _esdashboard_popup_menu_focusable_iface_init));

/* Properties */
enum
{
	PROP_0,

	PROP_DESTROY_ON_CANCEL,

	PROP_SOURCE,

	PROP_SHOW_TITLE,
	PROP_TITLE,

	PROP_SHOW_TITLE_ICON,
	PROP_TITLE_ICON_NAME,
	PROP_TITLE_GICON,

	PROP_LAST
};

static GParamSpec* EsdashboardPopupMenuProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ACTIVATED,
	SIGNAL_CANCELLED,

	SIGNAL_ITEM_ACTIVATED,

	SIGNAL_ITEM_ADDED,
	SIGNAL_ITEM_REMOVED,

	SIGNAL_LAST
};

static guint EsdashboardPopupMenuSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Suspension state of application changed */
static void _esdashboard_popup_menu_on_application_suspended_changed(EsdashboardPopupMenu *self,
																		GParamSpec *inSpec,
																		gpointer inUserData)
{
	EsdashboardPopupMenuPrivate		*priv;
	EsdashboardApplication			*application;
	gboolean						isSuspended;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(ESDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;
	application=ESDASHBOARD_APPLICATION(inUserData);

	/* Get application suspend state */
	isSuspended=esdashboard_application_is_suspended(application);

	/* If application is suspended then cancel pop-up menu */
	if(isSuspended)
	{
		ESDASHBOARD_DEBUG(self, ACTOR,
							"Cancel active pop-up menu '%s' for source %s@%p because application was suspended",
							esdashboard_popup_menu_get_title(self),
							priv->source ? G_OBJECT_TYPE_NAME(priv->source) : "<nil>",
							priv->source);

		esdashboard_popup_menu_cancel(self);
	}
}

/* An event occured after a popup menu was activated so check if popup menu should
 * be cancelled because a button was pressed and release outside the popup menu.
 */
static gboolean _esdashboard_popup_menu_on_captured_event(EsdashboardPopupMenu *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	EsdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(ESDASHBOARD_IS_STAGE(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Check if popup menu should be cancelled depending on event */
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_BUTTON_RELEASE:
			/* If button was released outside popup menu cancel this popup menu */
			{
				gfloat							x, y, w, h;

				clutter_actor_get_transformed_position(CLUTTER_ACTOR(self), &x, &y);
				clutter_actor_get_size(CLUTTER_ACTOR(self), &w, &h);
				if(inEvent->button.x<x ||
					inEvent->button.x>=(x+w) ||
					inEvent->button.y<y ||
					inEvent->button.y>=(y+h))
				{
					/* Cancel popup menu */
					esdashboard_popup_menu_cancel(self);

					/* Do not let this event be handled */
					return(CLUTTER_EVENT_STOP);
				}
			}
			break;

		case CLUTTER_KEY_PRESS:
		case CLUTTER_KEY_RELEASE:
			/* If key press or key release is not a selection action for a focusable
			 * actor then cancel this popup menu.
			 */
			{
				GSList							*targetFocusables;
				const gchar						*action;
				gboolean						cancelPopupMenu;

				/* Lookup action for event and emit action if a binding was found
				 * for this event.
				 */
				targetFocusables=NULL;
				action=NULL;
				cancelPopupMenu=FALSE;

				if(esdashboard_focus_manager_get_event_targets_and_action(priv->focusManager, inEvent, ESDASHBOARD_FOCUSABLE(self), &targetFocusables, &action))
				{
					if(!targetFocusables ||
						!targetFocusables->data ||
						!ESDASHBOARD_IS_POPUP_MENU(targetFocusables->data))
					{
						cancelPopupMenu=TRUE;
					}

					/* Release allocated resources */
					g_slist_free_full(targetFocusables, g_object_unref);
				}

				/* 'ESC' is a special key as it cannot be determined by focus
				 * manager but it has to be intercepted as this key release
				 * should only cancel popup-menu but not quit application.
				 */
				if(!cancelPopupMenu &&
					clutter_event_type(inEvent)==CLUTTER_KEY_RELEASE &&
					inEvent->key.keyval==CLUTTER_KEY_Escape)
				{
					cancelPopupMenu=TRUE;
				}

				/* Cancel popup-menu if requested */
				if(cancelPopupMenu)
				{
					/* Cancel popup menu */
					esdashboard_popup_menu_cancel(self);

					/* Do not let this event be handled */
					return(CLUTTER_EVENT_STOP);
				}
			}
			break;

		default:
			/* Let all other event pass through */
			break;
	}

	/* If we get here then this event passed our filter and can be handled normally */
	return(CLUTTER_EVENT_PROPAGATE);
}

/*  Check if menu item is really part of this pop-up menu */
static gboolean _esdashboard_popup_menu_contains_menu_item(EsdashboardPopupMenu *self,
															EsdashboardPopupMenuItem *inMenuItem)
{
	ClutterActor			*parent;

	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), FALSE);

	/* Iterate through parents and for each EsdashboardPopupMenu found, check
	 * if it is this pop-up menu and return TRUE if it is.
	 */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(inMenuItem));
	while(parent)
	{
		/* Check if current iterated parent is a EsdashboardPopupMenu and if it
		 * is this pop-up menu.
		 */
		if(ESDASHBOARD_IS_POPUP_MENU(parent) &&
			ESDASHBOARD_POPUP_MENU(parent)==self)
		{
			/* This one is this pop-up menu, so return TRUE here */
			return(TRUE);
		}

		/* Continue with next parent */
		parent=clutter_actor_get_parent(parent);
	}

	/* If we get here the "menu item" actor is a menu item of this pop-up menu */
	return(FALSE);
}

/* Menu item was activated */
static void _esdashboard_popup_menu_on_menu_item_activated(EsdashboardPopupMenu *self,
															gpointer inUserData)
{
	EsdashboardPopupMenuItem		*menuItem;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inUserData));

	menuItem=ESDASHBOARD_POPUP_MENU_ITEM(inUserData);

	/* Emit "item-activated" signal */
	g_signal_emit(self, EsdashboardPopupMenuSignals[SIGNAL_ITEM_ACTIVATED], 0, menuItem);

	/* Cancel pop-up menu as menu item was activated and its callback function
	 * was called by its meta object.
	 */
	esdashboard_popup_menu_cancel(self);
}

/* Update visiblity of title actor depending on if title and/or icon of title
 * should be shown or not.
 */
static void _esdashboard_popup_menu_update_title_actors_visibility(EsdashboardPopupMenu *self)
{
	EsdashboardPopupMenuPrivate		*priv;
	EsdashboardLabelStyle			oldStyle;
	EsdashboardLabelStyle			newStyle;
	gboolean						oldVisible;
	gboolean						newVisible;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Get current visibility state */
	oldVisible=clutter_actor_is_visible(priv->title);
	oldStyle=esdashboard_label_get_style(ESDASHBOARD_LABEL(priv->title));

	/* Determine new visibility state depending on if title and/or icon of title
	 * should be shown or not.
	 */
	newStyle=0;
	newVisible=TRUE;
	if(priv->showTitle && priv->showTitleIcon) newStyle=ESDASHBOARD_LABEL_STYLE_BOTH;
		else if(priv->showTitle) newStyle=ESDASHBOARD_LABEL_STYLE_TEXT;
		else if(priv->showTitleIcon) newStyle=ESDASHBOARD_LABEL_STYLE_ICON;
		else newVisible=FALSE;

	/* Set new visibility style if changed and re-layout title actor */
	if(newStyle!=oldStyle)
	{
		esdashboard_label_set_style(ESDASHBOARD_LABEL(priv->title), newStyle);
		clutter_actor_queue_relayout(priv->title);
	}

	/* Show or hide actor */
	if(newVisible!=oldVisible)
	{
		if(newVisible) clutter_actor_show(priv->title);
			else clutter_actor_hide(priv->title);
	}
}

/* The source actor was destroyed so cancel this pop-up menu if active and
 * destroy it if automatic destruction was turned on.
 */
static void _esdashboard_popup_menu_on_source_destroy(EsdashboardPopupMenu *self,
														gpointer inUserData)
{
	EsdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	priv=self->priv;

	/* Unset and clean-up source */
	if(priv->source)
	{
		gchar						*cssClass;

		/* Disconnect signal handler */
		if(priv->sourceDestroySignalID)
		{
			g_signal_handler_disconnect(priv->source, priv->sourceDestroySignalID);
			priv->sourceDestroySignalID=0;
		}

		/* Remove style */
		cssClass=g_strdup_printf("popup-menu-source-%s", G_OBJECT_TYPE_NAME(priv->source));
		esdashboard_stylable_remove_class(ESDASHBOARD_STYLABLE(self), cssClass);
		g_free(cssClass);

		/* Release source */
		g_object_remove_weak_pointer(G_OBJECT(priv->source), (gpointer*)&priv->source);
		priv->source=NULL;
	}

	/* Enforce that pop-up menu is cancelled either by calling the cancel function
	 * if it is active or by checking and destructing it if automatic destruction
	 * flag is set.
	 */
	if(priv->isActive)
	{
		/* Pop-up menu is active so cancel it. The cancel function will also destroy
		 * it if destroy-on-cancel was enabled.
		 */
		esdashboard_popup_menu_cancel(self);
	}
		else
		{
			/* Destroy this pop-up menu actor when destroy-on-cancel was enabled */
			if(priv->destroyOnCancel)
			{
				esdashboard_actor_destroy(CLUTTER_ACTOR(self));
			}
		}
}


/* IMPLEMENTATION: ClutterActor */

/* Allocate position and size of actor and its children */
static void _esdashboard_popup_menu_allocate(ClutterActor *inActor,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	ClutterAllocationFlags		flags;

	/* Chain up to store the allocation of the actor */
	flags=inFlags | CLUTTER_DELEGATE_LAYOUT;
	CLUTTER_ACTOR_CLASS(esdashboard_popup_menu_parent_class)->allocate(inActor, inBox, flags);
}


/* IMPLEMENTATION: Interface EsdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _esdashboard_popup_menu_focusable_can_focus(EsdashboardFocusable *inFocusable)
{
	EsdashboardPopupMenu			*self;
	EsdashboardPopupMenuPrivate		*priv;
	EsdashboardFocusableInterface	*selfIface;
	EsdashboardFocusableInterface	*parentIface;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(inFocusable), FALSE);

	self=ESDASHBOARD_POPUP_MENU(inFocusable);
	priv=self->priv;

	/* Call parent class interface function */
	selfIface=ESDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->can_focus)
	{
		if(!parentIface->can_focus(inFocusable))
		{
			return(FALSE);
		}
	}

	/* Only active pop-up menus can be focused */
	if(!priv->isActive) return(FALSE);

	/* If we get here this actor can be focused */
	return(TRUE);
}

/* Actor lost focus */
static void _esdashboard_popup_menu_focusable_unset_focus(EsdashboardFocusable *inFocusable)
{
	EsdashboardPopupMenu			*self;
	EsdashboardPopupMenuPrivate		*priv;
	EsdashboardFocusableInterface	*selfIface;
	EsdashboardFocusableInterface	*parentIface;

	g_return_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable));
	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(inFocusable));

	self=ESDASHBOARD_POPUP_MENU(inFocusable);
	priv=self->priv;

	/* Call parent class interface function */
	selfIface=ESDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->unset_focus)
	{
		parentIface->unset_focus(inFocusable);
	}

	/* If this pop-up menu is active (has flag set) then it was not cancelled and
	 * this actor lost its focus in any other way than expected. So do not refocus
	 * old remembered focusable as it may not be the one which has the focus before.
	 */
	if(priv->isActive &&
		priv->oldFocusable)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->oldFocusable), &priv->oldFocusable);
		priv->oldFocusable=NULL;
	}

	/* This actor lost focus so ensure that this popup menu is cancelled */
	esdashboard_popup_menu_cancel(self);
}

/* Determine if this actor supports selection */
static gboolean _esdashboard_popup_menu_focusable_supports_selection(EsdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}

/* Get current selection */
static ClutterActor* _esdashboard_popup_menu_focusable_get_selection(EsdashboardFocusable *inFocusable)
{
	EsdashboardPopupMenu			*self;
	EsdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(inFocusable), NULL);

	self=ESDASHBOARD_POPUP_MENU(inFocusable);
	priv=self->priv;

	/* Return current selection */
	return(priv->selectedItem);
}

/* Set new selection */
static gboolean _esdashboard_popup_menu_focusable_set_selection(EsdashboardFocusable *inFocusable,
																ClutterActor *inSelection)
{
	EsdashboardPopupMenu			*self;
	EsdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=ESDASHBOARD_POPUP_MENU(inFocusable);
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

	/* Add weak reference at new selection */
	if(priv->selectedItem)
	{
		g_object_add_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
	}

	/* New selection was set successfully */
	return(TRUE);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _esdashboard_popup_menu_focusable_find_selection(EsdashboardFocusable *inFocusable,
																		ClutterActor *inSelection,
																		EsdashboardSelectionTarget inDirection)
{
	EsdashboardPopupMenu				*self;
	EsdashboardPopupMenuPrivate			*priv;
	ClutterActor						*selection;
	ClutterActor						*newSelection;
	gchar								*valueName;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(inFocusable), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=ESDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	self=ESDASHBOARD_POPUP_MENU(inFocusable);
	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;

	/* If there is nothing selected, select first actor and return */
	if(!inSelection)
	{
		selection=clutter_actor_get_first_child(CLUTTER_ACTOR(priv->itemsContainer));

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
		case ESDASHBOARD_SELECTION_TARGET_UP:
			newSelection=clutter_actor_get_previous_sibling(inSelection);
			break;

		case ESDASHBOARD_SELECTION_TARGET_DOWN:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			break;

		case ESDASHBOARD_SELECTION_TARGET_FIRST:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_UP:
			newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(priv->itemsContainer));
			break;

		case ESDASHBOARD_SELECTION_TARGET_LAST:
		case ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			newSelection=clutter_actor_get_last_child(CLUTTER_ACTOR(priv->itemsContainer));
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
static gboolean _esdashboard_popup_menu_focusable_activate_selection(EsdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	EsdashboardPopupMenu				*self;
	EsdashboardPopupMenuItem			*menuItem;

	g_return_val_if_fail(ESDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(inFocusable), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inSelection), FALSE);

	self=ESDASHBOARD_POPUP_MENU(inFocusable);
	menuItem=ESDASHBOARD_POPUP_MENU_ITEM(inSelection);

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
	esdashboard_popup_menu_item_activate(menuItem);

	/* If we get here activation of menu item was successful */
	return(TRUE);
}

/* Interface initialization
 * Set up default functions
 */
void _esdashboard_popup_menu_focusable_iface_init(EsdashboardFocusableInterface *iface)
{
	iface->can_focus=_esdashboard_popup_menu_focusable_can_focus;
	iface->unset_focus=_esdashboard_popup_menu_focusable_unset_focus;

	iface->supports_selection=_esdashboard_popup_menu_focusable_supports_selection;
	iface->get_selection=_esdashboard_popup_menu_focusable_get_selection;
	iface->set_selection=_esdashboard_popup_menu_focusable_set_selection;
	iface->find_selection=_esdashboard_popup_menu_focusable_find_selection;
	iface->activate_selection=_esdashboard_popup_menu_focusable_activate_selection;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _esdashboard_popup_menu_dispose(GObject *inObject)
{
	EsdashboardPopupMenu			*self=ESDASHBOARD_POPUP_MENU(inObject);
	EsdashboardPopupMenuPrivate		*priv=self->priv;

	/* Cancel this pop-up menu if it is still active */
	esdashboard_popup_menu_cancel(self);

	/* Release our allocated variables */
	if(priv->suspendSignalID)
	{
		g_signal_handler_disconnect(esdashboard_application_get_default(), priv->suspendSignalID);
		priv->suspendSignalID=0;
	}

	if(priv->capturedEventSignalID)
	{
		g_signal_handler_disconnect(priv->stage, priv->capturedEventSignalID);
		priv->capturedEventSignalID=0;
	}

	if(priv->source)
	{
		gchar						*cssClass;

		/* Disconnect signal handler */
		if(priv->sourceDestroySignalID)
		{
			g_signal_handler_disconnect(priv->source, priv->sourceDestroySignalID);
			priv->sourceDestroySignalID=0;
		}

		/* Remove style */
		cssClass=g_strdup_printf("popup-menu-source-%s", G_OBJECT_TYPE_NAME(priv->source));
		esdashboard_stylable_remove_class(ESDASHBOARD_STYLABLE(self), cssClass);
		g_free(cssClass);

		/* Release source */
		g_object_remove_weak_pointer(G_OBJECT(priv->source), (gpointer*)&priv->source);
		priv->source=NULL;
	}

	if(priv->selectedItem)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
		priv->selectedItem=NULL;
	}

	if(priv->oldFocusable)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->oldFocusable), &priv->oldFocusable);
		priv->oldFocusable=NULL;
	}

	if(priv->itemsContainer)
	{
		clutter_actor_destroy(priv->itemsContainer);
		priv->itemsContainer=NULL;
	}

	if(priv->focusManager)
	{
		esdashboard_focus_manager_unregister(priv->focusManager, ESDASHBOARD_FOCUSABLE(self));
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	if(priv->windowTracker)
	{
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->stage)
	{
		priv->stage=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(esdashboard_popup_menu_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _esdashboard_popup_menu_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	EsdashboardPopupMenu			*self=ESDASHBOARD_POPUP_MENU(inObject);

	switch(inPropID)
	{
		case PROP_DESTROY_ON_CANCEL:
			esdashboard_popup_menu_set_destroy_on_cancel(self, g_value_get_boolean(inValue));
			break;

		case PROP_SOURCE:
			esdashboard_popup_menu_set_source(self, CLUTTER_ACTOR(g_value_get_object(inValue)));
			break;

		case PROP_SHOW_TITLE:
			esdashboard_popup_menu_set_show_title(self, g_value_get_boolean(inValue));
			break;

		case PROP_TITLE:
			esdashboard_popup_menu_set_title(self, g_value_get_string(inValue));
			break;

		case PROP_SHOW_TITLE_ICON:
			esdashboard_popup_menu_set_show_title_icon(self, g_value_get_boolean(inValue));
			break;

		case PROP_TITLE_ICON_NAME:
			esdashboard_popup_menu_set_title_icon_name(self, g_value_get_string(inValue));
			break;

		case PROP_TITLE_GICON:
			esdashboard_popup_menu_set_title_gicon(self, G_ICON(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _esdashboard_popup_menu_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	EsdashboardPopupMenu			*self=ESDASHBOARD_POPUP_MENU(inObject);
	EsdashboardPopupMenuPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_DESTROY_ON_CANCEL:
			g_value_set_boolean(outValue, priv->destroyOnCancel);
			break;

		case PROP_SOURCE:
			g_value_set_object(outValue, priv->source);
			break;

		case PROP_SHOW_TITLE:
			g_value_set_boolean(outValue, priv->showTitle);
			break;

		case PROP_TITLE:
			g_value_set_string(outValue, esdashboard_popup_menu_get_title(self));
			break;

		case PROP_SHOW_TITLE_ICON:
			g_value_set_boolean(outValue, priv->showTitleIcon);
			break;

		case PROP_TITLE_ICON_NAME:
			g_value_set_string(outValue, esdashboard_popup_menu_get_title_icon_name(self));
			break;

		case PROP_TITLE_GICON:
			g_value_set_object(outValue, esdashboard_popup_menu_get_title_gicon(self));
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
static void esdashboard_popup_menu_class_init(EsdashboardPopupMenuClass *klass)
{
	EsdashboardActorClass	*actorClass=ESDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_esdashboard_popup_menu_dispose;
	gobjectClass->set_property=_esdashboard_popup_menu_set_property;
	gobjectClass->get_property=_esdashboard_popup_menu_get_property;

	clutterActorClass->allocate=_esdashboard_popup_menu_allocate;

	/* Define properties */
	/**
	 * EsdashboardPopupMenu:destroy-on-cancel:
	 *
	 * A flag indicating if this pop-up menu should be destroyed automatically
	 * when it is cancelled.
	 */
	EsdashboardPopupMenuProperties[PROP_DESTROY_ON_CANCEL]=
		g_param_spec_boolean("destroy-on-cancel",
								"Destroy on cancel",
								"Flag indicating this pop-up menu should be destroyed automatically when it is cancelled",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenu:source:
	 *
	 * The #ClutterActor on which this pop-up menu depends on. If this actor is
	 * destroyed then this pop-up menu is cancelled when active. 
	 */
	EsdashboardPopupMenuProperties[PROP_SOURCE]=
		g_param_spec_object("source",
							"Source",
							"The object on which this pop-up menu depends on",
							CLUTTER_TYPE_ACTOR,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenu:show-title:
	 *
	 * A flag indicating if the title of this pop-up menu should be shown.
	 */
	EsdashboardPopupMenuProperties[PROP_SHOW_TITLE]=
		g_param_spec_boolean("show-title",
								"Show title",
								"Flag indicating if the title of this pop-up menu should be shown",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenu:title:
	 *
	 * A string containing the title of this pop-up menu.
	 */
	EsdashboardPopupMenuProperties[PROP_TITLE]=
		g_param_spec_string("title",
							"Title",
							"Title of pop-up menu",
							N_(""),
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenu:show-title-icon:
	 *
	 * A flag indicating if the icon of the title of this pop-up menu should be shown.
	 */
	EsdashboardPopupMenuProperties[PROP_SHOW_TITLE_ICON]=
		g_param_spec_boolean("show-title-icon",
								"Show title icon",
								"Flag indicating if the icon of title of this pop-up menu should be shown",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenu:title-icon-name:
	 *
	 * A string containing the stock icon name or file name for the icon to use
	 * at title of this pop-up menu.
	 */
	EsdashboardPopupMenuProperties[PROP_TITLE_ICON_NAME]=
		g_param_spec_string("title-icon-name",
							"Title icon name",
							"Themed icon name or file name of icon used in title",
							N_(""),
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * EsdashboardPopupMenu:title-gicon:
	 *
	 * A #GIcon containing the icon image to use at title of this pop-up menu.
	 */
	EsdashboardPopupMenuProperties[PROP_TITLE_GICON]=
		g_param_spec_object("title-gicon",
							"Title GIcon",
							"The GIcon of icon used in title",
							G_TYPE_ICON,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, EsdashboardPopupMenuProperties);

	/* Define stylable properties */
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardPopupMenuProperties[PROP_SHOW_TITLE]);
	esdashboard_actor_install_stylable_property(actorClass, EsdashboardPopupMenuProperties[PROP_SHOW_TITLE_ICON]);

	/* Define signals */
	/**
	 * EsdashboardPopupMenu::activated:
	 * @self: The pop-up menu which was activated
	 *
	 * The ::activated signal is emitted when the pop-up menu is shown and the
	 * user can perform an action by selecting an item.
	 */
	EsdashboardPopupMenuSignals[SIGNAL_ACTIVATED]=
		g_signal_new("activated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardPopupMenuClass, activated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * EsdashboardPopupMenu::cancelled:
	 * @self: The pop-up menu which was cancelled
	 *
	 * The ::cancelled signal is emitted when the pop-up menu is hidden. This
	 * signal is emitted regardless the user has chosen an item and perform the
	 * associated action or not.
	 *
	 * Note: This signal does not indicate if a selection was made or not.
	 */
	EsdashboardPopupMenuSignals[SIGNAL_CANCELLED]=
		g_signal_new("cancelled",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardPopupMenuClass, cancelled),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * EsdashboardPopupMenu::item-activated:
	 * @self: The pop-up menu containing the activated menu item
	 * @inMenuItem: The menu item which was activated
	 *
	 * The ::item-activated signal is emitted when a menu item at pop-up menu
	 * was activated either by key-press or by clicking on it.
	 */
	EsdashboardPopupMenuSignals[SIGNAL_ITEM_ACTIVATED]=
		g_signal_new("item-activated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardPopupMenuClass, item_activated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						ESDASHBOARD_TYPE_POPUP_MENU_ITEM);

	/**
	 * EsdashboardPopupMenu::item-added:
	 * @self: The pop-up menu containing the activated menu item
	 * @inMenuItem: The menu item which was added
	 *
	 * The ::item-added signal is emitted when a menu item was added to pop-up menu.
	 */
	EsdashboardPopupMenuSignals[SIGNAL_ITEM_ADDED]=
		g_signal_new("item-added",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardPopupMenuClass, item_added),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						ESDASHBOARD_TYPE_POPUP_MENU_ITEM);

	/**
	 * EsdashboardPopupMenu::item-removed:
	 * @self: The pop-up menu containing the activated menu item
	 * @inMenuItem: The menu item which was added
	 *
	 * The ::item-added signal is emitted when a menu item was added to pop-up menu.
	 */
	EsdashboardPopupMenuSignals[SIGNAL_ITEM_REMOVED]=
		g_signal_new("item-removed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(EsdashboardPopupMenuClass, item_removed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						ESDASHBOARD_TYPE_POPUP_MENU_ITEM);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void esdashboard_popup_menu_init(EsdashboardPopupMenu *self)
{
	EsdashboardPopupMenuPrivate		*priv;
	ClutterLayoutManager			*layout;

	priv=self->priv=esdashboard_popup_menu_get_instance_private(self);

	/* Set up default values */
	priv->destroyOnCancel=FALSE;
	priv->source=NULL;
	priv->showTitle=FALSE;
	priv->showTitleIcon=FALSE;
	priv->isActive=FALSE;
	priv->title=NULL;
	priv->itemsContainer=NULL;
	priv->windowTracker=esdashboard_window_tracker_get_default();
	priv->focusManager=esdashboard_focus_manager_get_default();
	priv->oldFocusable=NULL;
	priv->selectedItem=NULL;
	priv->stage=NULL;
	priv->capturedEventSignalID=0;
	priv->sourceDestroySignalID=0;
	priv->suspendSignalID=0;

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up title actor */
	priv->title=esdashboard_button_new();
	esdashboard_label_set_style(ESDASHBOARD_LABEL(priv->title), ESDASHBOARD_LABEL_STYLE_TEXT);
	esdashboard_label_set_text(ESDASHBOARD_LABEL(priv->title), NULL);
	clutter_actor_set_x_expand(priv->title, TRUE);
	clutter_actor_set_y_expand(priv->title, TRUE);
	clutter_actor_hide(priv->title);
	esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(priv->title), "popup-menu-title");

	/* Set up items container which will hold all menu items */
	layout=esdashboard_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);

	priv->itemsContainer=esdashboard_actor_new();
	clutter_actor_set_x_expand(priv->itemsContainer, TRUE);
	clutter_actor_set_y_expand(priv->itemsContainer, TRUE);
	clutter_actor_set_layout_manager(priv->itemsContainer, layout);

	/* Set up this actor */
	layout=esdashboard_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), layout);

	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->title);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->itemsContainer);
	esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(self), "popup-menu");

	/* Register this actor at focus manager but ensure that this actor is
	 * not focusable initially */
	esdashboard_actor_set_can_focus(ESDASHBOARD_ACTOR(self), FALSE);
	esdashboard_focus_manager_register(priv->focusManager, ESDASHBOARD_FOCUSABLE(self));

	/* Add popup menu to stage */
	priv->stage=esdashboard_application_get_stage(esdashboard_application_get_default());
	clutter_actor_insert_child_above(CLUTTER_ACTOR(priv->stage), CLUTTER_ACTOR(self), NULL);

	/* Connect signal to get notified when application suspends to cancel pop-up menu */
	priv->suspendSignalID=g_signal_connect_swapped(esdashboard_application_get_default(),
													"notify::is-suspended",
													G_CALLBACK(_esdashboard_popup_menu_on_application_suspended_changed),
													self);
}

/* IMPLEMENTATION: Public API */

/**
 * esdashboard_popup_menu_new:
 *
 * Creates a new #EsdashboardPopupMenu actor
 *
 * Return value: The newly created #EsdashboardPopupMenu
 */
ClutterActor* esdashboard_popup_menu_new(void)
{
	return(g_object_new(ESDASHBOARD_TYPE_POPUP_MENU, NULL));
}

/**
 * esdashboard_popup_menu_new_for_source:
 * @inSource: A #ClutterActor which this pop-up menu should depend on
 *
 * Creates a new #EsdashboardPopupMenu actor which depends on actor @inSource.
 * When the actor @inSource is destroyed and the pop-up menu is active then it
 * will be cancelled automatically.
 *
 * Return value: The newly created #EsdashboardPopupMenu
 */
ClutterActor* esdashboard_popup_menu_new_for_source(ClutterActor *inSource)
{
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSource), NULL);

	return(g_object_new(ESDASHBOARD_TYPE_POPUP_MENU,
						"source", inSource,
						NULL));
}

/**
 * esdashboard_popup_menu_get_destroy_on_cancel:
 * @self: A #EsdashboardPopupMenu
 *
 * Retrieves the automatic destruction mode of @self. If automatic destruction mode
 * is %TRUE then the pop-up menu will be destroy by calling esdashboard_actor_destroy()
 * when it is cancelled, e.g. by calling esdashboard_popup_menu_cancel().
 *
 * Return value: Returns %TRUE if automatic destruction mode is enabled, otherwise
 *   %FALSE.
 */
gboolean esdashboard_popup_menu_get_destroy_on_cancel(EsdashboardPopupMenu *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), FALSE);

	return(self->priv->destroyOnCancel);
}

/**
 * esdashboard_popup_menu_set_destroy_on_cancel:
 * @self: A #EsdashboardPopupMenu
 * @inDestroyOnCancel: The automatic destruction mode to set at @self
 *
 * Sets the automatic destruction mode of @self. If @inDestroyOnCancel is set to
 * %TRUE then the pop-up menu will automatically destroyed by calling esdashboard_actor_destroy()
 * when it is cancelled, e.g. by calling esdashboard_popup_menu_cancel().
 */
void esdashboard_popup_menu_set_destroy_on_cancel(EsdashboardPopupMenu *self, gboolean inDestroyOnCancel)
{
	EsdashboardPopupMenuPrivate			*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->destroyOnCancel!=inDestroyOnCancel)
	{
		/* Set value */
		priv->destroyOnCancel=inDestroyOnCancel;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuProperties[PROP_DESTROY_ON_CANCEL]);
	}
}

/**
 * esdashboard_popup_menu_get_source:
 * @self: A #EsdashboardPopupMenu
 *
 * Retrieves the source actor to @inSource which the pop-up menu at @self depends on.
 *
 * Return value: (transfer none): Returns #ClutterActor which the pop-up menu or
 *   %NULL if no source actor is set.
 */
ClutterActor* esdashboard_popup_menu_get_source(EsdashboardPopupMenu *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), NULL);

	return(self->priv->source);
}

/**
 * esdashboard_popup_menu_set_source:
 * @self: A #EsdashboardPopupMenu
 * @inSource: A #ClutterActor which this pop-up menu should depend on or %NULL
 *   if it should not depend on any actor
 *
 * Sets the source actor to @inSource which the pop-up menu at @self depends on.
 * When the actor @inSource is destroyed and the pop-up menu at @self is active
 * then it will be cancelled automatically.
 *
 * In addition the CSS class "popup-menu-source-SOURCE_CLASS_NAME" will be set
 * on pop-up menu at @self, e.g. if source is of type ClutterActor the CSS class
 * "popup-menu-source-ClutterActor" will be set.
 */
void esdashboard_popup_menu_set_source(EsdashboardPopupMenu *self, ClutterActor *inSource)
{
	EsdashboardPopupMenuPrivate			*priv;
	gchar								*cssClass;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(!inSource || CLUTTER_IS_ACTOR(inSource));

	priv=self->priv;

	/* Set value if changed */
	if(priv->source!=inSource)
	{
		/* Release old source if set */
		if(priv->source)
		{
			/* Disconnect signal handler */
			g_signal_handler_disconnect(priv->source, priv->sourceDestroySignalID);
			priv->sourceDestroySignalID=0;

			/* Remove style */
			cssClass=g_strdup_printf("popup-menu-source-%s", G_OBJECT_TYPE_NAME(priv->source));
			esdashboard_stylable_remove_class(ESDASHBOARD_STYLABLE(self), cssClass);
			g_free(cssClass);

			/* Release source */
			g_object_remove_weak_pointer(G_OBJECT(priv->source), (gpointer*)&priv->source);
			priv->source=NULL;
		}

		/* Set value */
		if(inSource)
		{
			/* Set up source */
			priv->source=inSource;
			g_object_add_weak_pointer(G_OBJECT(priv->source), (gpointer*)&priv->source);

			/* Add style */
			cssClass=g_strdup_printf("popup-menu-source-%s", G_OBJECT_TYPE_NAME(priv->source));
			esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(self), cssClass);
			g_free(cssClass);

			/* Connect signal handler */
			priv->sourceDestroySignalID=g_signal_connect_swapped(priv->source,
																	"destroy",
																	G_CALLBACK(_esdashboard_popup_menu_on_source_destroy),
																	self);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuProperties[PROP_SOURCE]);
	}
}

/**
 * esdashboard_popup_menu_get_show_title:
 * @self: A #EsdashboardPopupMenu
 *
 * Retrieves the state if the title of pop-up menu at @self should be shown or not.
 *
 * Return value: Returns %TRUE if title of pop-up menu should be shown and
 *   %FALSE if not.
 */
gboolean esdashboard_popup_menu_get_show_title(EsdashboardPopupMenu *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), FALSE);

	return(self->priv->showTitle);
}

/**
 * esdashboard_popup_menu_set_show_title:
 * @self: A #EsdashboardPopupMenu
 * @inShowTitle: Flag indicating if the title of pop-up menu should be shown.
 *
 * If @inShowTitle is %TRUE then the title of the pop-up menu at @self will be
 * shown. If @inShowTitle is %FALSE it will be hidden.
 */
void esdashboard_popup_menu_set_show_title(EsdashboardPopupMenu *self, gboolean inShowTitle)
{
	EsdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->showTitle!=inShowTitle)
	{
		/* Set value */
		priv->showTitle=inShowTitle;

		/* Update visibility state of title actor */
		_esdashboard_popup_menu_update_title_actors_visibility(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuProperties[PROP_SHOW_TITLE]);
	}
}

/**
 * esdashboard_popup_menu_get_title:
 * @self: A #EsdashboardPopupMenu
 *
 * Retrieves the title of pop-up menu.
 *
 * Return value: Returns string with title of pop-up menu.
 */
const gchar* esdashboard_popup_menu_get_title(EsdashboardPopupMenu *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), NULL);

	return(esdashboard_label_get_text(ESDASHBOARD_LABEL(self->priv->title)));
}

/**
 * esdashboard_popup_menu_set_title:
 * @self: A #EsdashboardPopupMenu
 * @inMarkupTitle: The title to set
 *
 * Sets @inMarkupTitle as title of pop-up menu at @self. The title string can
 * contain markup.
 */
void esdashboard_popup_menu_set_title(EsdashboardPopupMenu *self, const gchar *inMarkupTitle)
{
	EsdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(inMarkupTitle);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(esdashboard_label_get_text(ESDASHBOARD_LABEL(priv->title)), inMarkupTitle)!=0)
	{
		/* Set value */
		esdashboard_label_set_text(ESDASHBOARD_LABEL(priv->title), inMarkupTitle);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuProperties[PROP_TITLE]);
	}
}

/**
 * esdashboard_popup_menu_get_show_title_icon:
 * @self: A #EsdashboardPopupMenu
 *
 * Retrieves the state if the icon of title of pop-up menu at @self should be
 * shown or not.
 *
 * Return value: Returns %TRUE if icon of title of pop-up menu should be shown
 *   and %FALSE if not.
 */
gboolean esdashboard_popup_menu_get_show_title_icon(EsdashboardPopupMenu *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), FALSE);

	return(self->priv->showTitleIcon);
}

/**
 * esdashboard_popup_menu_set_show_title_icon:
 * @self: A #EsdashboardPopupMenu
 * @inShowTitle: Flag indicating if the icon of title of pop-up menu should be shown.
 *
 * If @inShowTitle is %TRUE then the icon of title of the pop-up menu at @self will be
 * shown. If @inShowTitle is %FALSE it will be hidden.
 */
void esdashboard_popup_menu_set_show_title_icon(EsdashboardPopupMenu *self, gboolean inShowTitleIcon)
{
	EsdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->showTitleIcon!=inShowTitleIcon)
	{
		/* Set value */
		priv->showTitleIcon=inShowTitleIcon;

		/* Update visibility state of title actor */
		_esdashboard_popup_menu_update_title_actors_visibility(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuProperties[PROP_SHOW_TITLE_ICON]);
	}
}

/**
 * esdashboard_popup_menu_get_title_icon_name:
 * @self: A #EsdashboardPopupMenu
 *
 * Retrieves the stock icon name or file name of title icon of pop-up menu at @self.
 *
 * Return value: Returns string with icon name or file name of pop-up menu's title.
 */
const gchar* esdashboard_popup_menu_get_title_icon_name(EsdashboardPopupMenu *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), NULL);

	return(esdashboard_label_get_icon_name(ESDASHBOARD_LABEL(self->priv->title)));
}

/**
 * esdashboard_popup_menu_set_title_icon_name:
 * @self: A #EsdashboardPopupMenu
 * @inIconName: A string containing the stock icon name or file name for the icon
 *   to be place in the toogle button
 *
 * Sets the icon in title to icon at @inIconName of pop-up menu at @self. If set to
 * %NULL the title icon is hidden.
 */
void esdashboard_popup_menu_set_title_icon_name(EsdashboardPopupMenu *self, const gchar *inIconName)
{
	EsdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(inIconName);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(esdashboard_label_get_icon_name(ESDASHBOARD_LABEL(priv->title)), inIconName)!=0)
	{
		/* Set value */
		esdashboard_label_set_icon_name(ESDASHBOARD_LABEL(priv->title), inIconName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuProperties[PROP_TITLE_ICON_NAME]);
	}
}

/**
 * esdashboard_popup_menu_get_title_gicon:
 * @self: A #EsdashboardPopupMenu
 *
 * Retrieves the title's icon of type #GIcon of pop-up menu at @self.
 *
 * Return value: Returns #GIcon of pop-up menu's title.
 */
GIcon* esdashboard_popup_menu_get_title_gicon(EsdashboardPopupMenu *self)
{
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), NULL);

	return(esdashboard_label_get_gicon(ESDASHBOARD_LABEL(self->priv->title)));
}

/**
 * esdashboard_popup_menu_set_title_gicon:
 * @self: A #EsdashboardPopupMenu
 * @inIcon: A #GIcon containing the icon image
 *
 * Sets the icon in title to icon at @inIcon of pop-up menu at @self. If set to
 * %NULL the title icon is hidden.
 */
void esdashboard_popup_menu_set_title_gicon(EsdashboardPopupMenu *self, GIcon *inIcon)
{
	EsdashboardPopupMenuPrivate		*priv;
	GIcon							*icon;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(G_IS_ICON(inIcon));

	priv=self->priv;

	/* Set value if changed */
	icon=esdashboard_label_get_gicon(ESDASHBOARD_LABEL(priv->title));
	if(icon!=inIcon ||
		(icon && inIcon && !g_icon_equal(icon, inIcon)))
	{
		/* Set value */
		esdashboard_label_set_gicon(ESDASHBOARD_LABEL(priv->title), inIcon);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), EsdashboardPopupMenuProperties[PROP_TITLE_GICON]);
	}
}

/**
 * esdashboard_popup_menu_add_item:
 * @self: A #EsdashboardPopupMenu
 * @inMenuItem: A #EsdashboardPopupMenuItem to add to pop-up menu
 *
 * Adds the actor @inMenuItem to end of pop-up menu.
 * 
 * If menu item actor implements the #EsdashboardStylable interface the CSS class
 * popup-menu-item will be added.
 * 
 * Return value: Returns index where item was inserted at or -1 if it failed.
 */
gint esdashboard_popup_menu_add_item(EsdashboardPopupMenu *self,
										EsdashboardPopupMenuItem *inMenuItem)
{
	return(esdashboard_popup_menu_insert_item(self, inMenuItem, -1));
}

/**
 * esdashboard_popup_menu_insert_item:
 * @self: A #EsdashboardPopupMenu
 * @inMenuItem: A #EsdashboardPopupMenuItem to add to pop-up menu
 * @inIndex: The position where to insert this item at
 *
 * Inserts the actor @inMenuItem at position @inIndex into pop-up menu.
 *
 * If position @inIndex is greater than the number of menu items in @self or is
 * less than 0, then the menu item actor @inMenuItem is added to end to
 * pop-up menu.
 *
 * If menu item actor implements the #EsdashboardStylable interface the CSS class
 * popup-menu-item will be added.
 *
 * Return value: Returns index where item was inserted at or -1 if it failed.
 */
gint esdashboard_popup_menu_insert_item(EsdashboardPopupMenu *self,
										EsdashboardPopupMenuItem *inMenuItem,
										gint inIndex)
{
	EsdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), -1);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), -1);
	g_return_val_if_fail(clutter_actor_get_parent(CLUTTER_ACTOR(inMenuItem))==NULL, -1);

	priv=self->priv;

	/* Insert menu item actor to container at requested position */
	clutter_actor_insert_child_at_index(priv->itemsContainer, CLUTTER_ACTOR(inMenuItem), inIndex);

	/* Add CSS class 'popup-menu-item' to newly added menu item */
	if(ESDASHBOARD_IS_STYLABLE(inMenuItem))
	{
		esdashboard_stylable_add_class(ESDASHBOARD_STYLABLE(inMenuItem), "popup-menu-item");
	}

	/* Connect signal to get notified when user made a selection to cancel pop-up
	 * menu but ensure that it is called nearly at last because the pop-up menu
	 * could be configured to get destroyed automatically when user selected an
	 * item (or cancelled the menu). In this case other signal handler may not be
	 * called if pop-up menu's signal handler is called before. By calling it at
	 * last all other normally connected signal handlers will get be called.
	 */
	g_signal_connect_data(inMenuItem,
							"activated",
							G_CALLBACK(_esdashboard_popup_menu_on_menu_item_activated),
							self,
							NULL,
							G_CONNECT_AFTER | G_CONNECT_SWAPPED);

	/* Emit signal */
	g_signal_emit(self, EsdashboardPopupMenuSignals[SIGNAL_ITEM_ADDED], 0, inMenuItem);

	/* Get index where menu item actor was inserted at */
	return(esdashboard_popup_menu_get_item_index(self, inMenuItem));
}

/**
 * esdashboard_popup_menu_move_item:
 * @self: A #EsdashboardPopupMenu
 * @inMenuItem: A #EsdashboardPopupMenuItem menu item to move
 * @inIndex: The position where to insert this item at
 *
 * Moves the actor @inMenuItem to position @inIndex at pop-up menu @self. If position
 * @inIndex is greater than the number of menu items in @self or is less than 0,
 * then the menu item actor @inMenuItem is added to end to pop-up menu.
 *
 * Return value: Returns %TRUE if moving menu item was successful, otherwise %FALSE.
 */
gboolean esdashboard_popup_menu_move_item(EsdashboardPopupMenu *self,
											EsdashboardPopupMenuItem *inMenuItem,
											gint inIndex)
{
	EsdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), FALSE);

	priv=self->priv;

	/* Check if menu item is really part of this pop-up menu */
	if(!_esdashboard_popup_menu_contains_menu_item(self, inMenuItem))
	{
		g_warning("%s is not a child of %s and cannot be moved",
					G_OBJECT_TYPE_NAME(inMenuItem),
					G_OBJECT_TYPE_NAME(self));
		return(FALSE);
	}

	/* Move menu item actor to new position */
	g_object_ref(inMenuItem);
	clutter_actor_remove_child(priv->itemsContainer, CLUTTER_ACTOR(inMenuItem));
	clutter_actor_insert_child_at_index(priv->itemsContainer, CLUTTER_ACTOR(inMenuItem), inIndex);
	g_object_unref(inMenuItem);

	/* If we get here moving menu item actor was successful */
	return(TRUE);
}

/**
 * esdashboard_popup_menu_get_item:
 * @self: A #EsdashboardPopupMenu
 * @inIndex: The position whose menu item to get
 *
 * Returns the menu item actor at position @inIndex at pop-up menu @self.
 *
 * Return value: Returns #EsdashboardPopupMenuItem of the menu item at position @inIndex or
 *   %NULL in case of errors or if index is out of range that means it is greater
 *   than the number of menu items in @self or is less than 0.
 */
EsdashboardPopupMenuItem* esdashboard_popup_menu_get_item(EsdashboardPopupMenu *self, gint inIndex)
{
	EsdashboardPopupMenuPrivate		*priv;
	ClutterActor					*menuItem;

	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), NULL);
	g_return_val_if_fail(inIndex>=0 && inIndex<clutter_actor_get_n_children(self->priv->itemsContainer), NULL);

	priv=self->priv;

	/* Get and return child at requested position at items container */
	menuItem=clutter_actor_get_child_at_index(priv->itemsContainer, inIndex);
	return(ESDASHBOARD_POPUP_MENU_ITEM(menuItem));
}

/**
 * esdashboard_popup_menu_get_item_index:
 * @self: A #EsdashboardPopupMenu
 * @inMenuItem: The #EsdashboardPopupMenuItem menu item whose index to lookup
 *
 * Returns the position for menu item actor @inMenuItem of pop-up menu @self.
 *
 * Return value: Returns the position of the menu item or -1 in case of errors
 *   or if pop-up menu does not have the menu item
 */
gint esdashboard_popup_menu_get_item_index(EsdashboardPopupMenu *self, EsdashboardPopupMenuItem *inMenuItem)
{
	EsdashboardPopupMenuPrivate		*priv;
	gint							index;
	ClutterActorIter				iter;
	ClutterActor					*child;

	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), -1);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), -1);

	priv=self->priv;

	/* Iterate through menu item and return the index if the requested one was found */
	index=0;

	clutter_actor_iter_init(&iter, priv->itemsContainer);
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* If this child is the one we are looking for return index now */
		if(child==CLUTTER_ACTOR(inMenuItem)) return(index);

		/* Increase index */
		index++;
	}

	/* If we get here we did not find the requested menu item */
	return(-1);
}

/**
 * esdashboard_popup_menu_remove_item:
 * @self: A #EsdashboardPopupMenu
 * @inMenuItem: A #EsdashboardPopupMenuItem menu item to remove
 *
 * Removes the actor @inMenuItem from pop-up menu @self. When the pop-up menu holds
 * the last reference on that menu item actor then it will be destroyed otherwise
 * it will only be removed from pop-up menu.
 *
 * If the removed menu item actor implements the #EsdashboardStylable interface
 * the CSS class popup-menu-item will be removed also.
 *
 * Return value: Returns %TRUE if moving menu item was successful, otherwise %FALSE.
 */
gboolean esdashboard_popup_menu_remove_item(EsdashboardPopupMenu *self, EsdashboardPopupMenuItem *inMenuItem)
{
	EsdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU(self), FALSE);
	g_return_val_if_fail(ESDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), FALSE);

	priv=self->priv;

	/* Check if menu item is really part of this pop-up menu */
	if(!_esdashboard_popup_menu_contains_menu_item(self, inMenuItem))
	{
		g_warning("%s is not a child of %s and cannot be removed",
					G_OBJECT_TYPE_NAME(inMenuItem),
					G_OBJECT_TYPE_NAME(self));
		return(FALSE);
	}

	/* Take extra reference on actor to remove to keep it alive while working with it */
	g_object_ref(inMenuItem);

	/* Remove CSS class 'popup-menu-item' from menu item going to be removed */
	if(ESDASHBOARD_IS_STYLABLE(inMenuItem))
	{
		esdashboard_stylable_remove_class(ESDASHBOARD_STYLABLE(inMenuItem), "popup-menu-item");
	}

	/* Remove menu item actor from pop-up menu */
	clutter_actor_remove_child(priv->itemsContainer, CLUTTER_ACTOR(inMenuItem));

	/* Disconnect signal handlers from removed menu item */
	g_signal_handlers_disconnect_by_func(inMenuItem, G_CALLBACK(_esdashboard_popup_menu_on_menu_item_activated), self);

	/* Emit signal */
	g_signal_emit(self, EsdashboardPopupMenuSignals[SIGNAL_ITEM_REMOVED], 0, inMenuItem);

	/* Release extra reference on actor to took to keep it alive */
	g_object_unref(inMenuItem);

	/* If we get here we removed the menu item actor successfully */
	return(TRUE);
}

/**
 * esdashboard_popup_menu_activate:
 * @self: A #EsdashboardPopupMenu
 *
 * Displays the pop-up menu at @self and makes it available for selection.
 *
 * This actor will gain the focus automatically and will select the first menu item.
 */
void esdashboard_popup_menu_activate(EsdashboardPopupMenu *self)
{
	EsdashboardPopupMenuPrivate			*priv;
	GdkDisplay							*display;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat								*seat;
#else
	GdkDeviceManager					*deviceManager;
#endif
	GdkDevice							*pointerDevice;
	gint								pointerX, pointerY;
	EsdashboardWindowTrackerMonitor		*monitor;
	gint								monitorX, monitorY, monitorWidth, monitorHeight;
	gfloat								x, y, w, h;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* If this actor is already active, then do nothing */
	if(priv->isActive) return;

	/* Move popup menu next to pointer similar to tooltips but keep it on current monitor */
	display=gdk_display_get_default();
#if GTK_CHECK_VERSION(3, 20, 0)
	seat=gdk_display_get_default_seat(display);
	pointerDevice=gdk_seat_get_pointer(seat);
#else
	deviceManager=gdk_display_get_device_manager(display);
	pointerDevice=gdk_device_manager_get_client_pointer(deviceManager);
#endif
	gdk_device_get_position(pointerDevice, NULL, &pointerX, &pointerY);
	ESDASHBOARD_DEBUG(self, ACTOR,
						"Pointer is at position %d,%d",
						pointerX, pointerY);

	monitor=esdashboard_window_tracker_get_monitor_by_position(priv->windowTracker, pointerX, pointerY);
	if(!monitor)
	{
		/* Show error message */
		g_critical("Could not find monitor at pointer position %d,%d",
					pointerX,
					pointerY);

		return;
	}

	esdashboard_window_tracker_monitor_get_geometry(monitor, &monitorX, &monitorY, &monitorWidth, &monitorHeight);
	ESDASHBOARD_DEBUG(self, ACTOR,
						"Pointer is on monitor %d with position at %d,%d and size of %dx%d",
						esdashboard_window_tracker_monitor_get_number(monitor),
						monitorX, monitorY,
						monitorWidth, monitorHeight);

	x=pointerX;
	y=pointerY;
	clutter_actor_get_size(CLUTTER_ACTOR(self), &w, &h);
	if(x<monitorX) x=monitorX;
	if((x+w)>=(monitorX+monitorWidth)) x=(monitorX+monitorWidth)-w;
	if(y<monitorY) y=monitorY;
	if((y+h)>=(monitorY+monitorHeight)) y=(monitorY+monitorHeight)-h;
	clutter_actor_set_position(CLUTTER_ACTOR(self), floor(x), floor(y));

	/* Now start capturing event in "capture" phase to stop propagating event to
	 * other actors except this one while popup menu is active.
	 */
	priv->capturedEventSignalID=
		g_signal_connect_swapped(priv->stage,
									"captured-event",
									G_CALLBACK(_esdashboard_popup_menu_on_captured_event),
									self);

	/* Show popup menu */
	clutter_actor_show(CLUTTER_ACTOR(self));

	/* Set flag that this pop-up menu is now active otherwise we cannot focus
	 * this actor.
	 */
	priv->isActive=TRUE;

	/* Make popup menu focusable as this also marks this actor to be active */
	esdashboard_actor_set_can_focus(ESDASHBOARD_ACTOR(self), TRUE);

	/* Move focus to popup menu but remember the actor which has current focus */
	priv->oldFocusable=esdashboard_focus_manager_get_focus(priv->focusManager);
	if(priv->oldFocusable) g_object_add_weak_pointer(G_OBJECT(priv->oldFocusable), &priv->oldFocusable);

	esdashboard_focus_manager_set_focus(priv->focusManager, ESDASHBOARD_FOCUSABLE(self));
}

/**
 * esdashboard_popup_menu_cancel:
 * @self: A #EsdashboardPopupMenu
 *
 * Hides the pop-up menu if displayed and stops making it available for selection.
 *
 * The actor tries to refocus the actor which had the focus before this pop-up
 * menu was displayed. If that actor cannot be focused it move the focus to the
 * next focusable actor.
 */
void esdashboard_popup_menu_cancel(EsdashboardPopupMenu *self)
{
	EsdashboardPopupMenuPrivate			*priv;

	g_return_if_fail(ESDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Do nothing if pop-up menu is not active */
	if(!priv->isActive) return;

	/* Unset flag that pop-up menu is active to prevent recursive calls on this
	 * function, e.g. if pop-up menu is cancelled because the object instance
	 * is disposed.
	 */
	priv->isActive=FALSE;

	/* Stop capturing events in "capture" phase as this popup menu actor will not
	 * be active anymore.
	 */
	if(priv->capturedEventSignalID)
	{
		g_signal_handler_disconnect(priv->stage, priv->capturedEventSignalID);
		priv->capturedEventSignalID=0;
	}

	/* Move focus to actor which had the focus previously */
	if(priv->oldFocusable)
	{
		/* Remove weak pointer from previously focused actor */
		g_object_remove_weak_pointer(G_OBJECT(priv->oldFocusable), &priv->oldFocusable);

		/* Move focus to previous focussed actor */
		esdashboard_focus_manager_set_focus(priv->focusManager, priv->oldFocusable);

		/* Forget previous focussed actor */
		priv->oldFocusable=NULL;
	}

	/* Hide popup menu */
	clutter_actor_hide(CLUTTER_ACTOR(self));

	/* Reset popup menu to be not focusable as this also marks this actor is
	 * not active anymore.
	 */
	esdashboard_actor_set_can_focus(ESDASHBOARD_ACTOR(self), FALSE);

	/* Destroy this pop-up menu actor when destroy-on-cancel was enabled */
	if(priv->destroyOnCancel)
	{
		esdashboard_actor_destroy(CLUTTER_ACTOR(self));
	}
}
