AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBESDASHBOARD_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"esdashboard\" \
	-DPACKAGE_DATADIR=\"$(datadir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPACKAGE_LIBDIR=\"$(libdir)\" \
	$(ESDASHBOARD_DEBUG_CFLAGS) \
	$(PLATFORM_CPPFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libesdashboard.pc

lib_LTLIBRARIES = \
	libesdashboard.la

esdashboard_marshal_prefix = esdashboard

libesdashboard_la_built_sources = \
	enums.h \
	enums.c \
	marshal.h \
	marshal.c \
	version.h

libesdashboard_la_headers = \
	action-button.h \
	actor.h \
	animation.h \
	application.h \
	application-button.h \
	application-database.h \
	application-tracker.h \
	applications-menu-model.h \
	applications-search-provider.h \
	applications-view.h \
	background.h \
	binding.h \
	bindings-pool.h \
	box-layout.h \
	button.h \
	click-action.h \
	collapse-box.h \
	compat.h \
	css-selector.h \
	debug.h \
	desktop-app-info.h \
	desktop-app-info-action.h \
	drag-action.h \
	drop-action.h \
	dynamic-table-layout.h \
	emblem-effect.h \
	fill-box-layout.h \
	focusable.h \
	focus-manager.h \
	image-content.h \
	label.h \
	live-window.h \
	live-window-simple.h \
	live-workspace.h \
	model.h \
	outline-effect.h \
	plugin.h \
	plugins-manager.h \
	popup-menu.h \
	popup-menu-item.h \
	popup-menu-item-button.h \
	popup-menu-item-separator.h \
	quicklaunch.h \
	scaled-table-layout.h \
	scrollbar.h \
	search-manager.h \
	search-provider.h \
	search-result-container.h \
	search-result-set.h \
	search-view.h \
	stage.h \
	stage-interface.h \
	stylable.h \
	text-box.h \
	theme.h \
	theme-animation.h \
	theme-css.h \
	theme-effects.h \
	theme-layout.h \
	toggle-button.h \
	tooltip-action.h \
	transition-group.h \
	types.h \
	utils.h \
	view.h \
	view-manager.h \
	view-selector.h \
	viewpad.h \
	window-content.h \
	window-tracker.h \
	window-tracker-backend.h \
	window-tracker-monitor.h \
	window-tracker-window.h \
	window-tracker-workspace.h \
	windows-view.h \
	workspace-selector.h

libesdashboard_la_SOURCES = \
	$(libesdashboard_la_built_sources) \
	$(esdashboard_headers) \
	action-button.c \
	actor.c \
	animation.c \
	application.c \
	application-button.c \
	application-database.c \
	application-tracker.c \
	applications-menu-model.c \
	applications-search-provider.c \
	applications-view.c \
	background.c \
	binding.c \
	bindings-pool.c \
	box-layout.c \
	button.c \
	click-action.c \
	collapse-box.c \
	css-selector.c \
	debug.c \
	desktop-app-info.c \
	desktop-app-info-action.c \
	drag-action.c \
	drop-action.c \
	dynamic-table-layout.c \
	emblem-effect.c \
	fill-box-layout.c \
	focusable.c \
	focus-manager.c \
	image-content.c \
	label.c \
	live-window.c \
	live-window-simple.c \
	live-workspace.c \
	model.c \
	outline-effect.c \
	plugin.c \
	plugins-manager.c \
	popup-menu.c \
	popup-menu-item.c \
	popup-menu-item-button.c \
	popup-menu-item-separator.c \
	quicklaunch.c \
	scaled-table-layout.c \
	scrollbar.c \
	search-manager.c \
	search-provider.c \
	search-result-container.c \
	search-result-set.c \
	search-view.c \
	stage.c \
	stage-interface.c \
	stylable.c \
	text-box.c \
	theme.c \
	theme-animation.c \
	theme-css.c \
	theme-effects.c \
	theme-layout.c \
	toggle-button.c \
	tooltip-action.c \
	transition-group.c \
	utils.c \
	view.c \
	view-manager.c \
	view-selector.c \
	viewpad.c \
	window-content.c \
	window-tracker.c \
	window-tracker-backend.c \
	window-tracker-monitor.c \
	window-tracker-window.c \
	window-tracker-workspace.c \
	windows-view.c \
	workspace-selector.c

libesdashboard_la_CFLAGS = \
	$(LIBWNCK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(CLUTTER_COGL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GIO_UNIX_CFLAGS) \
	$(MARKON_CFLAGS) \
	$(LIBESCONF_CFLAGS) \
	$(LIBEXPIDUS1UTIL_CFLAGS) \
	$(LIBEXPIDUS1UI_CFLAGS) \
	$(PLATFORM_CFLAGS)

libesdashboard_la_LIBADD = \
	$(LIBWNCK_LIBS) \
	$(CLUTTER_LIBS) \
	$(CLUTTER_COGL_LIBS) \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(GIO_UNIX_LIBS) \
	$(MARKON_LIBS) \
	$(LIBESCONF_LIBS) \
	$(LIBEXPIDUS1UTIL_LIBS) \
	$(LIBEXPIDUS1UI_LIBS) \
	$(LIBM)

if DBUS_GLIB
libesdashboard_la_CFLAGS += \
	$(DBUS_GLIB_CFLAGS)

libesdashboard_la_LIBADD += \
	$(DBUS_GLIB_LIBS)
endif

libesdashboard_la_LDFLAGS = \
	-export-dynamic \
	-version-info $(LIBESDASHBOARD_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-no-undefined

x11_headers = \
	x11/window-content-x11.h \
	x11/window-tracker-x11.h \
	x11/window-tracker-backend-x11.h \
	x11/window-tracker-monitor-x11.h \
	x11/window-tracker-window-x11.h \
	x11/window-tracker-workspace-x11.h

x11_sources = \
	x11/window-content-x11.c \
	x11/window-tracker-x11.c \
	x11/window-tracker-backend-x11.c \
	x11/window-tracker-monitor-x11.c \
	x11/window-tracker-window-x11.c \
	x11/window-tracker-workspace-x11.c

libesdashboard_la_headers += $(x11_headers)
libesdashboard_la_SOURCES += $(x11_sources)

if ESDASHBOARD_BUILD_WITH_XCOMPOSITE
libesdashboard_la_CFLAGS += \
	$(XCOMPOSITE_CFLAGS)

libesdashboard_la_LIBADD += \
	$(XCOMPOSITE_LIBS)
endif

if ESDASHBOARD_BUILD_WITH_XDAMAGE
libesdashboard_la_CFLAGS += \
	$(XDAMAGE_CFLAGS)

libesdashboard_la_LIBADD += \
	$(XDAMAGE_LIBS)
endif

if ESDASHBOARD_BUILD_WITH_XINERAMA
libesdashboard_la_CFLAGS += \
	$(XINERAMA_CFLAGS)

libesdashboard_la_LIBADD += \
	$(XINERAMA_LIBS)
endif

if ESDASHBOARD_BUILD_WITH_BACKEND_GDK
gdk_headers = \
	gdk/window-tracker-backend-gdk.h

libesdashboard_la_headers += $(gdk_headers)

gdk_sources = \
	gdk/window-tracker-backend-gdk.c

libesdashboard_la_SOURCES += $(gdk_sources)

libesdashboard_la_CFLAGS += \
	$(CLUTTER_GDK_CFLAGS)

libesdashboard_la_LIBADD += \
	$(CLUTTER_GDK_LIBS)
endif

libesdashboard_la_includedir = \
	$(includedir)/esdashboard/libesdashboard

libesdashboard_la_include_HEADERS = \
	$(libesdashboard_la_headers) \
	libesdashboard.h

if MAINTAINER_MODE

CLEANFILES = \
	xgen-xmc \
	xgen-xmh

enums.h: $(libesdashboard_la_headers) Makefile
	$(AM_V_GEN) $(GLIB_MKENUMS) --template enums.h.template $(libesdashboard_la_headers) > $@

enums.c: $(libesdashboard_la_headers) Makefile
	$(AM_V_GEN) $(GLIB_MKENUMS) --template enums.c.template $(libesdashboard_la_headers) > $@

marshal.h: stamp-marshal.h
	@true

stamp-marshal.h: marshal.list Makefile
	$(AM_V_GEN) \
	( \
		cd $(srcdir) \
		&& ( $(GLIB_GENMARSHAL) \
			--prefix=_esdashboard_marshal \
			--header marshal.list ) > xgen-xmh \
		&& ( cmp -s xgen-xmh marshal.h || cp xgen-xmh marshal.h ) \
		&& rm -f xgen-xmh \
		&& echo timestamp > $(@F) \
	)

marshal.c: marshal.list Makefile
	$(AM_V_GEN) \
	( \
		cd $(srcdir) \
		&& echo "#include \"marshal.h\"" > xgen-xmc \
		&& ( $(GLIB_GENMARSHAL) \
			--prefix=_esdashboard_marshal \
			--body marshal.list ) >> xgen-xmc \
		&& cp xgen-xmc marshal.c \
		&& rm -f xgen-xmc \
	)

version.h: version.h.in Makefile
	$(AM_V_GEN) sed -e "s|@LIBESDASHBOARD_MAJOR_VERSION[@]|$(ESDASHBOARD_VERSION_MAJOR)|" \
		-e "s|@LIBESDASHBOARD_MINOR_VERSION[@]|$(ESDASHBOARD_VERSION_MINOR)|" \
		-e "s|@LIBESDASHBOARD_MICRO_VERSION[@]|$(ESDASHBOARD_VERSION_MICRO)|" \
		$< > $@ || rm $@

DISTCLEANFILES = \
	$(libesdashboard_la_built_sources)

BUILT_SOURCES = \
	$(libesdashboard_la_built_sources)

endif

EXTRA_DIST = \
	enums.h.template \
	enums.c.template \
	marshal.list \
	version.h.in
