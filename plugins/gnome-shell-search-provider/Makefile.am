plugindir = $(libdir)/esdashboard/plugins
PLUGIN_ID = gnome-shell-search-provider

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"esdashboard-plugin-gnome_shell_search_provider\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPLUGIN_ID=\"$(PLUGIN_ID)\" \
	-DGNOME_SHELL_PROVIDERS_PATH=\"$(prefix)/share/gnome-shell/search-providers\"

plugin_LTLIBRARIES = \
	gnome-shell-search-provider.la

gnome_shell_search_provider_la_SOURCES = \
	gnome-shell-search-provider.c \
	gnome-shell-search-provider.h \
	plugin.c

gnome_shell_search_provider_la_CFLAGS = \
	$(LIBEXPIDUS1UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(LIBESCONF_CFLAGS) \
	$(MARKON_CFLAGS) \
	$(PLATFORM_CFLAGS)

gnome_shell_search_provider_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-export-symbols-regex '^plugin_init$$' \
	-no-undefined \
	-module \
	-shared \
	$(PLATFORM_LDFLAGS)

gnome_shell_search_provider_la_LIBADD = \
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
