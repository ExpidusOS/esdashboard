AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DLIBESDASHBOARD_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"esdashboard\" \
	-DPACKAGE_DATADIR=\"$(datadir)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPACKAGE_LIBDIR=\"$(libdir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	esdashboard

esdashboard_SOURCES = \
	main.c

esdashboard_CFLAGS = \
	$(CLUTTER_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(LIBESCONF_CFLAGS) \
	$(LIBEXPIDUS1UTIL_CFLAGS) \
	$(GTK_CFLAGS) \
	$(PLATFORM_CFLAGS)

esdashboard_LDADD = \
	$(CLUTTER_LIBS) \
	$(GLIB_LIBS) \
	$(LIBESCONF_LIBS) \
	$(LIBEXPIDUS1UTIL_LIBS) \
	$(GTK_LIBS) \
	$(top_builddir)/libesdashboard/libesdashboard.la

esdashboard_LDFLAGS = \
	$(PLATFORM_LDFLAGS) \
	-export-dynamic \
	-no-undefined
