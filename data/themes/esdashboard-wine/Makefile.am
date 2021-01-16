themedir = $(datadir)/themes/esdashboard-wine/esdashboard-1.0

theme_in_files = \
	esdashboard.theme.in

theme_DATA = \
	$(theme_in_files:.theme.in=.theme) \
	esdashboard-wine.css \
	esdashboard.css \
	esdashboard.xml \
	esdashboard-secondary.xml \
	animations.xml \
	effects.xml \
	screenshot-esdashboard-wine.png

# Substitute bindir so it works when built in a non-system prefix
esdashboard.theme.in:esdashboard.theme.in.in
	$(AM_V_GEN) sed -e "s|@PACKAGE_VERSION[@]|$(PACKAGE_VERSION)|" \
	    $< > $@ || rm $@

@INTLTOOL_THEME_RULE@

CLEANFILES = \
	$(theme_in_files) \
	$(theme_in_files:.theme.in=.theme)

EXTRA_DIST = \
	$(theme_in_files:.theme.in=.theme.in.in) \
	$(theme_DATA)

DISTCLEANFILES = \
	$(theme_in_files) \
	$(theme_in_files:.theme.in=.theme)
