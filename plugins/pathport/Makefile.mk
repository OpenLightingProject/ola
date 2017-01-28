# LIBRARIES
##################################################
if USE_PATHPORT
lib_LTLIBRARIES += plugins/pathport/libolapathport.la

# Plugin description is generated from README.md
built_sources += plugins/pathport/PathportPluginDescription.h
nodist_plugins_pathport_libolapathport_la_SOURCES = \
    plugins/pathport/PathportPluginDescription.h
plugins/pathport/PathportPluginDescription.h: plugins/pathport/README.md plugins/pathport/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/pathport $(top_builddir)/plugins/pathport/PathportPluginDescription.h

plugins_pathport_libolapathport_la_SOURCES = \
    plugins/pathport/PathportDevice.cpp \
    plugins/pathport/PathportDevice.h \
    plugins/pathport/PathportNode.cpp \
    plugins/pathport/PathportNode.h \
    plugins/pathport/PathportPackets.h \
    plugins/pathport/PathportPlugin.cpp \
    plugins/pathport/PathportPlugin.h \
    plugins/pathport/PathportPort.cpp \
    plugins/pathport/PathportPort.h
plugins_pathport_libolapathport_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += plugins/pathport/README.md
