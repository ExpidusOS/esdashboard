/*
 * types: Define application specific but common types
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
 * SECTION:types
 * @title: Enums and types
 * @short_description: Common enums and types
 * @include: esdashboard/types.h
 *
 * Various miscellaneous utilility functions.
 */

#ifndef __LIBESDASHBOARD_TYPES__
#define __LIBESDASHBOARD_TYPES__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * EsdashboardViewMode:
 * @ESDASHBOARD_VIEW_MODE_LIST: Show items in view as list
 * @ESDASHBOARD_VIEW_MODE_ICON: Show items in view as icons
 *
 * Determines how to display items of a view.
 */
typedef enum /*< prefix=ESDASHBOARD_VIEW_MODE >*/
{
	ESDASHBOARD_VIEW_MODE_LIST=0,
	ESDASHBOARD_VIEW_MODE_ICON
} EsdashboardViewMode;

/**
 * EsdashboardVisibilityPolicy:
 * @ESDASHBOARD_VISIBILITY_POLICY_NEVER: The actor is always visible.
 * @ESDASHBOARD_VISIBILITY_POLICY_AUTOMATIC: The actor will appear and disappear as necessary. For example, when a view does not fit into viewpad the scrollbar will be visible.
 * @ESDASHBOARD_VISIBILITY_POLICY_ALWAYS: The actor will never appear.
 *
 * Determines when an actor will be visible, e.g. scrollbars in views.
 */
typedef enum /*< prefix=ESDASHBOARD_VISIBILITY_POLICY >*/
{
	ESDASHBOARD_VISIBILITY_POLICY_NEVER=0,
	ESDASHBOARD_VISIBILITY_POLICY_AUTOMATIC,
	ESDASHBOARD_VISIBILITY_POLICY_ALWAYS
} EsdashboardVisibilityPolicy;

/**
 * EsdashboardOrientation:
 * @ESDASHBOARD_ORIENTATION_LEFT: The actor is justified to left boundary.
 * @ESDASHBOARD_ORIENTATION_RIGHT: The actor is justified to right boundary.
 * @ESDASHBOARD_ORIENTATION_TOP: The actor is justified to top boundary.
 * @ESDASHBOARD_ORIENTATION_BOTTOM: The actor is justified to bottom boundary.
 *
 * Determines the side to which an actor is justified to. It can mostly be switched on-the-fly.
 */
typedef enum /*< prefix=ESDASHBOARD_ORIENTATION >*/
{
	ESDASHBOARD_ORIENTATION_LEFT=0,
	ESDASHBOARD_ORIENTATION_RIGHT,
	ESDASHBOARD_ORIENTATION_TOP,
	ESDASHBOARD_ORIENTATION_BOTTOM
} EsdashboardOrientation;

/**
 * EsdashboardCorners:
 * @ESDASHBOARD_CORNERS_NONE: No corner is affected.
 * @ESDASHBOARD_CORNERS_TOP_LEFT: Affects top-left corner of actor.
 * @ESDASHBOARD_CORNERS_TOP_RIGHT: Affects top-right corner of actor.
 * @ESDASHBOARD_CORNERS_BOTTOM_LEFT: Affects bottom-left corner of actor.
 * @ESDASHBOARD_CORNERS_BOTTOM_RIGHT: Affects bottom-right corner of actor.
 * @ESDASHBOARD_CORNERS_TOP: Affects corners at top side of actor - top-left and top-right.
 * @ESDASHBOARD_CORNERS_BOTTOM: Affects corners at bottom side of actor - bottom-left and bottom-right.
 * @ESDASHBOARD_CORNERS_LEFT: Affects corners at left side of actor - top-left and bottom-left.
 * @ESDASHBOARD_CORNERS_RIGHT: Affects corners at right side of actor - top-right and bottom-right.
 * @ESDASHBOARD_CORNERS_ALL: Affects all corners of actor.
 *
 * Specifies which corner of an actor is affected, e.g. used in background for rounded rectangles.
 */
typedef enum /*< flags,prefix=ESDASHBOARD_CORNERS >*/
{
	ESDASHBOARD_CORNERS_NONE=0,

	ESDASHBOARD_CORNERS_TOP_LEFT=1 << 0,
	ESDASHBOARD_CORNERS_TOP_RIGHT=1 << 1,
	ESDASHBOARD_CORNERS_BOTTOM_LEFT=1 << 2,
	ESDASHBOARD_CORNERS_BOTTOM_RIGHT=1 << 3,

	ESDASHBOARD_CORNERS_TOP=(ESDASHBOARD_CORNERS_TOP_LEFT | ESDASHBOARD_CORNERS_TOP_RIGHT),
	ESDASHBOARD_CORNERS_BOTTOM=(ESDASHBOARD_CORNERS_BOTTOM_LEFT | ESDASHBOARD_CORNERS_BOTTOM_RIGHT),
	ESDASHBOARD_CORNERS_LEFT=(ESDASHBOARD_CORNERS_TOP_LEFT | ESDASHBOARD_CORNERS_BOTTOM_LEFT),
	ESDASHBOARD_CORNERS_RIGHT=(ESDASHBOARD_CORNERS_TOP_RIGHT | ESDASHBOARD_CORNERS_BOTTOM_RIGHT),

	ESDASHBOARD_CORNERS_ALL=(ESDASHBOARD_CORNERS_TOP_LEFT | ESDASHBOARD_CORNERS_TOP_RIGHT | ESDASHBOARD_CORNERS_BOTTOM_LEFT | ESDASHBOARD_CORNERS_BOTTOM_RIGHT)
} EsdashboardCorners;

/**
 * EsdashboardBorders:
 * @ESDASHBOARD_BORDERS_NONE: No side is affected.
 * @ESDASHBOARD_BORDERS_LEFT: Affects left side of actor.
 * @ESDASHBOARD_BORDERS_TOP: Affects top side of actor.
 * @ESDASHBOARD_BORDERS_RIGHT: Affects right side of actor.
 * @ESDASHBOARD_BORDERS_BOTTOM: Affects bottom side of actor.
 * @ESDASHBOARD_BORDERS_ALL: Affects all sides of actor.
 *
 * Determines which side of an actor is affected, e.g. used in outlines.
 */
typedef enum /*< flags,prefix=ESDASHBOARD_BORDERS >*/
{
	ESDASHBOARD_BORDERS_NONE=0,

	ESDASHBOARD_BORDERS_LEFT=1 << 0,
	ESDASHBOARD_BORDERS_TOP=1 << 1,
	ESDASHBOARD_BORDERS_RIGHT=1 << 2,
	ESDASHBOARD_BORDERS_BOTTOM=1 << 3,

	ESDASHBOARD_BORDERS_ALL=(ESDASHBOARD_BORDERS_LEFT | ESDASHBOARD_BORDERS_TOP | ESDASHBOARD_BORDERS_RIGHT | ESDASHBOARD_BORDERS_BOTTOM)
} EsdashboardBorders;

/**
 * EsdashboardStageBackgroundImageType:
 * @ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE: Do not show anything at background of stage actor.
 * @ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_DESKTOP: Show current desktop image at background of stage actor.
 *
 * Determine what to show at background of a stage actor.
 */
typedef enum /*< prefix=ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE >*/
{
	ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE=0,
	ESDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_DESKTOP
} EsdashboardStageBackgroundImageType;

/**
 * EsdashboardSelectionTarget:
 * @ESDASHBOARD_SELECTION_TARGET_LEFT: Move to next selectable actor at left side.
 * @ESDASHBOARD_SELECTION_TARGET_RIGHT: Move to next selectable actor at right side.
 * @ESDASHBOARD_SELECTION_TARGET_UP: Move to next selectable actor at top side.
 * @ESDASHBOARD_SELECTION_TARGET_DOWN: Move to next selectable actor at bottom side.
 * @ESDASHBOARD_SELECTION_TARGET_FIRST: Move to first selectable actor.
 * @ESDASHBOARD_SELECTION_TARGET_LAST: Move to last selectable actor.
 * @ESDASHBOARD_SELECTION_TARGET_PAGE_LEFT: Move to next selectable actor at left side page-width.
 * @ESDASHBOARD_SELECTION_TARGET_PAGE_RIGHT: Move to next selectable actor at right side page-width.
 * @ESDASHBOARD_SELECTION_TARGET_PAGE_UP: Move to next selectable actor at top side page-width.
 * @ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN: Move to next selectable actor at bottom side page-width.
 * @ESDASHBOARD_SELECTION_TARGET_NEXT: Move to next selectable actor to current one.
 *
 * Determines the movement of selection within an actor which supports selections.
 */
typedef enum /*< prefix=ESDASHBOARD_SELECTION_TARGET >*/
{
	ESDASHBOARD_SELECTION_TARGET_LEFT=0,
	ESDASHBOARD_SELECTION_TARGET_RIGHT,
	ESDASHBOARD_SELECTION_TARGET_UP,
	ESDASHBOARD_SELECTION_TARGET_DOWN,

	ESDASHBOARD_SELECTION_TARGET_FIRST,
	ESDASHBOARD_SELECTION_TARGET_LAST,

	ESDASHBOARD_SELECTION_TARGET_PAGE_LEFT,
	ESDASHBOARD_SELECTION_TARGET_PAGE_RIGHT,
	ESDASHBOARD_SELECTION_TARGET_PAGE_UP,
	ESDASHBOARD_SELECTION_TARGET_PAGE_DOWN,

	ESDASHBOARD_SELECTION_TARGET_NEXT
} EsdashboardSelectionTarget;

/* Anchor points */
/**
 * EsdashboardAnchorPoint:
 * @ESDASHBOARD_ANCHOR_POINT_NONE: Use default anchor of actor, usually top-left.
 * @ESDASHBOARD_ANCHOR_POINT_NORTH_WEST: The anchor is at the top-left of the object. 
 * @ESDASHBOARD_ANCHOR_POINT_NORTH: The anchor is at the top of the object, centered horizontally. 
 * @ESDASHBOARD_ANCHOR_POINT_NORTH_EAST: The anchor is at the top-right of the object. 
 * @ESDASHBOARD_ANCHOR_POINT_EAST: The anchor is on the right of the object, centered vertically. 
 * @ESDASHBOARD_ANCHOR_POINT_SOUTH_EAST:  The anchor is at the bottom-right of the object. 
 * @ESDASHBOARD_ANCHOR_POINT_SOUTH: The anchor is at the bottom of the object, centered horizontally. 
 * @ESDASHBOARD_ANCHOR_POINT_SOUTH_WEST:  The anchor is at the bottom-left of the object. 
 * @ESDASHBOARD_ANCHOR_POINT_WEST: The anchor is on the left of the object, centered vertically. 
 * @ESDASHBOARD_ANCHOR_POINT_CENTER: The anchor is in the center of the object. 
 * 
 * Specifys the position of an object relative to a particular anchor point.
 */
typedef enum /*< prefix=ESDASHBOARD_ANCHOR_POINT >*/
{
	ESDASHBOARD_ANCHOR_POINT_NONE=0,
	ESDASHBOARD_ANCHOR_POINT_NORTH_WEST,
	ESDASHBOARD_ANCHOR_POINT_NORTH,
	ESDASHBOARD_ANCHOR_POINT_NORTH_EAST,
	ESDASHBOARD_ANCHOR_POINT_EAST,
	ESDASHBOARD_ANCHOR_POINT_SOUTH_EAST,
	ESDASHBOARD_ANCHOR_POINT_SOUTH,
	ESDASHBOARD_ANCHOR_POINT_SOUTH_WEST,
	ESDASHBOARD_ANCHOR_POINT_WEST,
	ESDASHBOARD_ANCHOR_POINT_CENTER
} EsdashboardAnchorPoint;

G_END_DECLS

#endif	/* __LIBESDASHBOARD_TYPES__ */
