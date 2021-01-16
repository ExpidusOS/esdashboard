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

#ifndef __LIBESDASHBOARD_APPLICATION__
#define __LIBESDASHBOARD_APPLICATION__

#if !defined(__LIBESDASHBOARD_H_INSIDE__) && !defined(LIBESDASHBOARD_COMPILATION)
#error "Only <libesdashboard/libesdashboard.h> can be included directly."
#endif

#include <gio/gio.h>
#include <esconf/esconf.h>

#include <libesdashboard/theme.h>
#include <libesdashboard/focusable.h>
#include <libesdashboard/stage.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * EsdashboardApplicationErrorCode:
 * @ESDASHBOARD_APPLICATION_ERROR_NONE: Application started successfully without any problems
 * @ESDASHBOARD_APPLICATION_ERROR_FAILED: Application failed to start
 * @ESDASHBOARD_APPLICATION_ERROR_RESTART: Application needs to be restarted to start-up successfully
 * @ESDASHBOARD_APPLICATION_ERROR_QUIT: Application was quitted and shuts down
 *
 * The start-up status codes returned by EsdashboardApplication.
 */
typedef enum /*< skip,prefix=ESDASHBOARD_APPLICATION_ERROR >*/
{
	ESDASHBOARD_APPLICATION_ERROR_NONE=0,
	ESDASHBOARD_APPLICATION_ERROR_FAILED,
	ESDASHBOARD_APPLICATION_ERROR_RESTART,
	ESDASHBOARD_APPLICATION_ERROR_QUIT
} EsdashboardApplicationErrorCode;


/* Object declaration */
#define ESDASHBOARD_TYPE_APPLICATION				(esdashboard_application_get_type())
#define ESDASHBOARD_APPLICATION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), ESDASHBOARD_TYPE_APPLICATION, EsdashboardApplication))
#define ESDASHBOARD_IS_APPLICATION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), ESDASHBOARD_TYPE_APPLICATION))
#define ESDASHBOARD_APPLICATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), ESDASHBOARD_TYPE_APPLICATION, EsdashboardApplicationClass))
#define ESDASHBOARD_IS_APPLICATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), ESDASHBOARD_TYPE_APPLICATION))
#define ESDASHBOARD_APPLICATION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), ESDASHBOARD_TYPE_APPLICATION, EsdashboardApplicationClass))

typedef struct _EsdashboardApplication				EsdashboardApplication;
typedef struct _EsdashboardApplicationClass			EsdashboardApplicationClass;
typedef struct _EsdashboardApplicationPrivate		EsdashboardApplicationPrivate;

/**
 * EsdashboardApplication:
 *
 * The #EsdashboardApplication structure contains only private data and
 * should be accessed using the provided API
 */
struct _EsdashboardApplication
{
	/*< private >*/
	/* Parent instance */
	GApplication					parent_instance;

	/* Private structure */
	EsdashboardApplicationPrivate	*priv;
};

/**
 * EsdashboardApplicationClass:
 * @initialized: Class handler for the #EsdashboardApplicationClass::initialized signal
 * @suspend: Class handler for the #EsdashboardApplicationClass::suspend signal
 * @resume: Class handler for the #EsdashboardApplicationClass::resume signal
 * @quit: Class handler for the #EsdashboardApplicationClass::quit signal
 * @shutdown_final: Class handler for the #EsdashboardApplicationClass::shutdown_final signal
 * @theme_changed: Class handler for the #EsdashboardApplicationClass::theme_changed signal
 * @application_launched: Class handler for the #EsdashboardApplicationClass::application_launched signal
 * @exit: Class handler for the #EsdashboardApplicationClass::exit signal
 *
 * The #EsdashboardApplicationClass structure contains only private data
 */
struct _EsdashboardApplicationClass
{
	/*< private >*/
	/* Parent class */
	GApplicationClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*initialized)(EsdashboardApplication *self);

	void (*suspend)(EsdashboardApplication *self);
	void (*resume)(EsdashboardApplication *self);

	void (*quit)(EsdashboardApplication *self);
	void (*shutdown_final)(EsdashboardApplication *self);

	void (*theme_loading)(EsdashboardApplication *self, EsdashboardTheme *inTheme);
	void (*theme_loaded)(EsdashboardApplication *self, EsdashboardTheme *inTheme);
	void (*theme_changed)(EsdashboardApplication *self, EsdashboardTheme *inTheme);

	void (*application_launched)(EsdashboardApplication *self, GAppInfo *inAppInfo);

	/* Binding actions */
	gboolean (*exit)(EsdashboardApplication *self,
						EsdashboardFocusable *inSource,
						const gchar *inAction,
						ClutterEvent *inEvent);
};


/* Public API */
GType esdashboard_application_get_type(void) G_GNUC_CONST;

gboolean esdashboard_application_has_default(void);
EsdashboardApplication* esdashboard_application_get_default(void);

gboolean esdashboard_application_is_daemonized(EsdashboardApplication *self);
gboolean esdashboard_application_is_suspended(EsdashboardApplication *self);

gboolean esdashboard_application_is_quitting(EsdashboardApplication *self);
void esdashboard_application_resume(EsdashboardApplication *self);
void esdashboard_application_suspend_or_quit(EsdashboardApplication *self);
void esdashboard_application_quit_forced(EsdashboardApplication *self);

EsdashboardStage* esdashboard_application_get_stage(EsdashboardApplication *self);
EsdashboardTheme* esdashboard_application_get_theme(EsdashboardApplication *self);

EsconfChannel* esdashboard_application_get_esconf_channel(EsdashboardApplication *self);

G_END_DECLS

#endif	/* __LIBESDASHBOARD_APPLICATION__ */
