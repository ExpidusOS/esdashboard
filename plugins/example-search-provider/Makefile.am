plugindir = $(libdir)/esdashboard/plugins
PLUGIN_ID = example-search-provider

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"esdashboard-plugin-example_search_provider\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPLUGIN_ID=\"$(PLUGIN_ID)\"

plugin_LTLIBRARIES = \
	example-search-provider.la

example_search_provider_la_SOURCES = \
	example-search-provider.c \
	example-search-provider.h \
	plugin.c

example_search_provider_la_CFLAGS = \
	$(LIBEXPIDUS1UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(LIBESCONF_CFLAGS) \
	$(GARCON_CFLAGS) \
	$(PLATFORM_CFLAGS)

example_search_provider_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-export-symbols-regex '^plugin_init$$' \
	-no-undefined \
	-module \
	-shared \
	$(PLATFORM_LDFLAGS)

example_search_provider_la_LIBADD = \
	$(LIBEXPIDUS1UTIL_LIBS) \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(LIBESCONF_LIBS) \
	$(GARCON_LIBS) \
	$(top_builddir)/libesdashboard/libesdashboard.la

CLEANFILES = \
	$(plugin_DATA)

EXTRA_DIST = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)
