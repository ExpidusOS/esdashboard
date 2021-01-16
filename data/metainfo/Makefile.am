metainfodir = $(datadir)/metainfo
metainfo_DATA = $(metainfo_in_files:.xml.in=.xml)
metainfo_in_files = com.expidus.esdashboard.metainfo.xml.in

@INTLTOOL_XML_RULE@

EXTRA_DIST = \
        $(metainfo_DATA) \
        com.expidus.esdashboard.metainfo.xml.in

DISTCLEANFILES = \
        $(metainfo_in_files:.xml.in=.xml)
