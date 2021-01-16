plugindir = $(libdir)/esdashboard/plugins
PLUGIN_ID = hot-corner

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"esdashboard-plugin-hot_corner\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPLUGIN_ID=\"$(PLUGIN_ID)\"

plugin_LTLIBRARIES = \
	hot-corner.la

hot_corner_la_SOURCES = \
	hot-corner.c \
	hot-corner.h \
	hot-corner-settings.c \
	hot-corner-settings.h \
	plugin.c

hot_corner_la_CFLAGS = \
	$(LIBEXPIDUS1UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(LIBESCONF_CFLAGS) \
	$(MARKON_CFLAGS) \
	$(PLATFORM_CFLAGS)

hot_corner_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-export-symbols-regex '^plugin_init$$' \
	-no-undefined \
	-module \
	-shared \
	$(PLATFORM_LDFLAGS)

hot_corner_la_LIBADD = \
	$(LIBEXPIDUS1UTIL_LIBS) \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(LIBESCONF_LIBS) \
	$(MARKON_CLIBS) \
	$(top_builddir)/libesdashboard/libesdashboard.la

CLEANFILES = \
	$(plugin_DATA)

EXTRA_DIST = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)
