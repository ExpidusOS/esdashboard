SUBDIRS = \
	metainfo \
	data \
	icons \
	themes

desktopdir = $(datadir)/applications
desktop_in_files = com.expidus.esdashboard.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = com.expidus.esdashboard-autostart.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

settingsdir = $(datadir)/applications
settings_in_files = com.expidus.esdashboard-settings.desktop.in
settings_DATA = $(settings_in_files:.desktop.in=.desktop)

# Substitute bindir so it works when built in a non-system prefix
com.expidus.esdashboard.desktop.in:com.expidus.esdashboard.desktop.in.in
	$(AM_V_GEN) sed -e "s|@bindir[@]|$(bindir)|" \
	    -e "s|@VERSION[@]|$(VERSION)|" \
	    $< > $@ || rm $@

com.expidus.esdashboard-autostart.desktop.in:com.expidus.esdashboard-autostart.desktop.in.in
	$(AM_V_GEN) sed -e "s|@bindir[@]|$(bindir)|" \
	    -e "s|@VERSION[@]|$(VERSION)|" \
	    $< > $@ || rm $@

com.expidus.esdashboard-settings.desktop.in:com.expidus.esdashboard-settings.desktop.in.in
	$(AM_V_GEN) sed -e "s|@bindir[@]|$(bindir)|" \
	    -e "s|@VERSION[@]|$(VERSION)|" \
	    $< > $@ || rm $@

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_files:.desktop.in=.desktop.in.in) \
	$(autostart_in_files:.desktop.in=.desktop.in.in) \
	$(settings_in_files:.desktop.in=.desktop.in.in)

CLEANFILES = \
	$(desktop_in_files) \
	$(desktop_DATA) \
	$(autostart_in_files) \
	$(autostart_DATA) \
	$(settings_in_files) \
	$(settings_DATA)
