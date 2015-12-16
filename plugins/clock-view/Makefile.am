plugindir = $(libdir)/xfdashboard/plugins

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/xfdashboard \
	-DG_LOG_DOMAIN=\"xfdashboard-plugin-clock_view\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

plugin_LTLIBRARIES = \
	clock-view.la

clock_view_la_SOURCES = \
	clock-view.c \
	clock-view.h \
	plugin.c

clock_view_la_CFLAGS = \
	$(PLATFORM_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS)

clock_view_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-module \
	$(PLATFORM_LDFLAGS)

clock_view_la_LIBADD = \
	$(LIBXFCE4UTIL_LIBS) \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS)

CLEANFILES = \
	$(plugin_DATA)

EXTRA_DIST = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)