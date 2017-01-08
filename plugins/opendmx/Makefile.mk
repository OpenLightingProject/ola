# LIBRARIES
##################################################
if USE_OPENDMX
lib_LTLIBRARIES += plugins/opendmx/libolaopendmx.la

# Plugin description is generated from README.md
built_sources += plugins/opendmx/OpenDmxPluginDescription.h
nodist_plugins_opendmx_libolaopendmx_la_SOURCES = \
    plugins/opendmx/OpenDmxPluginDescription.h
plugins/opendmx/OpenDmxPluginDescription.h: plugins/opendmx/README.md plugins/opendmx/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/opendmx $(top_builddir)/plugins/opendmx/OpenDmxPluginDescription.h

plugins_opendmx_libolaopendmx_la_SOURCES = \
    plugins/opendmx/OpenDmxDevice.cpp \
    plugins/opendmx/OpenDmxDevice.h \
    plugins/opendmx/OpenDmxPlugin.cpp \
    plugins/opendmx/OpenDmxPlugin.h \
    plugins/opendmx/OpenDmxPort.h \
    plugins/opendmx/OpenDmxThread.cpp \
    plugins/opendmx/OpenDmxThread.h
plugins_opendmx_libolaopendmx_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += plugins/opendmx/README.md
