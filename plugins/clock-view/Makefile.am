plugindir = $(libdir)/esdashboard/plugins
PLUGIN_ID = clock-view

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"esdashboard-plugin-clock_view\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPLUGIN_ID=\"$(PLUGIN_ID)\"

plugin_LTLIBRARIES = \
	clock-view.la

clock_view_la_SOURCES = \
	clock-view.c \
	clock-view.h \
	clock-view-settings.c \
	clock-view-settings.h \
	plugin.c

clock_view_la_CFLAGS = \
	$(LIBEXPIDUS1UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(LIBESCONF_CFLAGS) \
	$(MARKON_CFLAGS) \
	$(PLATFORM_CFLAGS)

clock_view_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-export-symbols-regex '^plugin_init$$' \
	-no-undefined \
	-module \
	-shared \
	$(PLATFORM_LDFLAGS)

clock_view_la_LIBADD = \
	$(LIBEXPIDUS1UTIL_LIBS) \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(LIBESCONF_LIBS) \
	$(MARKON_LIBS) \
	$(top_builddir)/libesdashboard/libesdashboard.la

CLEANFILES = \
	$(plugin_DATA)

EXTRA_DIST = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)
