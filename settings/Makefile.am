AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBESDASHBOARD_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"esdashboard-settings\" \
	-DPACKAGE_DATADIR=\"$(datadir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPACKAGE_LIBDIR=\"$(libdir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	esdashboard-settings

esdashboard_settings_headers = \
	general.h \
	plugins.h \
	settings.h \
	themes.h

esdashboard_settings_SOURCES = \
	$(esdashboard_settings_headers) \
	general.c \
	main.c \
	plugins.c \
	settings.c \
	themes.c

esdashboard_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(CLUTTER_CFLAGS) \
	$(LIBESCONF_CFLAGS) \
	$(LIBEXPIDUS1UTIL_CFLAGS) \
	$(LIBEXPIDUS1UI_CFLAGS) \
	$(PLATFORM_CFLAGS)

esdashboard_settings_LDADD = \
	$(GTK_LIBS) \
	$(CLUTTER_LIBS) \
	$(LIBESCONF_LIBS) \
	$(LIBEXPIDUS1UTIL_LIBS) \
	$(LIBEXPIDUS1UI_LIBS) \
	$(LIBM) \
	$(top_builddir)/libesdashboard/libesdashboard.la

esdashboard_settings_LDFLAGS = \
	-export-dynamic \
	-no-undefined \
	$(PLATFORM_LDFLAGS)
