/*
 * debug: Helpers for debugging
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

#ifndef __LIBESDASHBOARD_DEBUG__
#define __LIBESDASHBOARD_DEBUG__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <glib.h>
#include "compat.h"

G_BEGIN_DECLS

/* Public definitions */
/**
 * EsdashboardDebugFlags:
 * @ESDASHBOARD_DEBUG_MISC: Miscellaneous, if debug message does not fit in any other category
 * @ESDASHBOARD_DEBUG_ACTOR: Actor related debug messages
 * @ESDASHBOARD_DEBUG_STYLE: Style at actor debug messages (resolving CSS, applying style etc.)
 * @ESDASHBOARD_DEBUG_THEME: Theme related debug messages (loading theme and resources etc.)
 * @ESDASHBOARD_DEBUG_APPLICATIONS: Application related debug message (spawing application process, application database and tracker etc.)
 * @ESDASHBOARD_DEBUG_IMAGES: Images related debug message (image cache etc.)
 * @ESDASHBOARD_DEBUG_WINDOWS: Windows related debug message (window tracker, workspaces, windows, monitors etc.)
 * @ESDASHBOARD_DEBUG_PLUGINS: Plug-ins related debug message (plugin manager and plugin base class)
 * @ESDASHBOARD_DEBUG_ANIMATION: Animation related debug message
 *
 * Debug categories
 */
typedef enum /*< skip,flags,prefix=ESDASHBOARD_DEBUG >*/
{
	ESDASHBOARD_DEBUG_MISC			= 1 << 0,
	ESDASHBOARD_DEBUG_ACTOR			= 1 << 1,
	ESDASHBOARD_DEBUG_STYLE			= 1 << 2,
	ESDASHBOARD_DEBUG_THEME			= 1 << 3,
	ESDASHBOARD_DEBUG_APPLICATIONS	= 1 << 4,
	ESDASHBOARD_DEBUG_IMAGES		= 1 << 5,
	ESDASHBOARD_DEBUG_WINDOWS		= 1 << 6,
	ESDASHBOARD_DEBUG_PLUGINS		= 1 << 7,
	ESDASHBOARD_DEBUG_ANIMATION		= 1 << 8
} EsdashboardDebugFlags;

#ifdef ESDASHBOARD_ENABLE_DEBUG

#define ESDASHBOARD_HAS_DEBUG(inCategory) \
	((esdashboard_debug_flags & ESDASHBOARD_DEBUG_##inCategory) != FALSE)

#ifndef __GNUC__

/* Try the GCC extension for valists in macros */
#define ESDASHBOARD_DEBUG(self, inCategory, inMessage, inArgs...)              \
	G_STMT_START \
	{ \
		if(G_UNLIKELY(ESDASHBOARD_HAS_DEBUG(inCategory)) ||                    \
			G_UNLIKELY(esdashboard_debug_classes && self && g_strv_contains((const gchar * const *)esdashboard_debug_classes, G_OBJECT_TYPE_NAME(self))))\
		{ \
			esdashboard_debug_message("[%s@%p]:[" #inCategory "]:" G_STRLOC ": " inMessage, (self ? G_OBJECT_TYPE_NAME(self) : ""), self, ##inArgs);\
		} \
	} \
	G_STMT_END

#else /* !__GNUC__ */

/* Try the C99 version; unfortunately, this does not allow us to pass empty
 * arguments to the macro, which means we have to do an intemediate printf.
 */
#define ESDASHBOARD_DEBUG(self, inCategory, ...)                               \
	G_STMT_START \
	{ \
		if(G_UNLIKELY(ESDASHBOARD_HAS_DEBUG(inCategory)) ||                    \
			G_UNLIKELY(esdashboard_debug_classes && self && g_strv_contains((const gchar * const *)esdashboard_debug_classes, G_OBJECT_TYPE_NAME(self))))\
		{ \
			gchar	*_esdashboard_debug_format=g_strdup_printf(__VA_ARGS__);     \
			esdashboard_debug_message("[%s@%p]:[" #inCategory "]:" G_STRLOC ": %s", (self ? G_OBJECT_TYPE_NAME(self) : ""), self, _esdashboard_debug_format);\
			g_free(_esdashboard_debug_format);                                 \
		} \
	} \
	G_STMT_END
#endif

#else /* !ESDASHBOARD_ENABLE_DEBUG */

#define ESDASHBOARD_HAS_DEBUG(inCategory)		FALSE
#define ESDASHBOARD_DEBUG(inCategory, ...)		G_STMT_START { } G_STMT_END

#endif /* ESDASHBOARD_ENABLE_DEBUG */

/* Public data */

extern guint esdashboard_debug_flags;
extern gchar **esdashboard_debug_classes;

/* Public API */

void esdashboard_debug_messagev(const char *inFormat, va_list inArgs);
void esdashboard_debug_message(const char *inFormat, ...) G_GNUC_PRINTF(1, 2);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_DEBUG__ */
