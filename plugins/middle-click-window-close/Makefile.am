plugindir = $(libdir)/esdashboard/plugins
PLUGIN_ID = middle-click-window-close

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"esdashboard-plugin-middle_click_window_close\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPLUGIN_ID=\"$(PLUGIN_ID)\"

plugin_LTLIBRARIES = \
	middle-click-window-close.la

middle_click_window_close_la_SOURCES = \
	middle-click-window-close.c \
	middle-click-window-close.h \
	plugin.c

middle_click_window_close_la_CFLAGS = \
	$(LIBEXPIDUS1UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(LIBESCONF_CFLAGS) \
	$(MARKON_CFLAGS) \
	$(PLATFORM_CFLAGS)

middle_click_window_close_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-export-symbols-regex '^plugin_init$$' \
	-no-undefined \
	-module \
	-shared \
	$(PLATFORM_LDFLAGS)

middle_click_window_close_la_LIBADD = \
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
