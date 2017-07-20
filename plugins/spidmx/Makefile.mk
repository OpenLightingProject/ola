# LIBRARIES
##################################################
if USE_SPIDMX
lib_LTLIBRARIES += plugins/spidmx/libolaspidmx.la

# Plugin description is generated from README.md
built_sources += plugins/spidmx/SpiDmxPluginDescription.h
nodist_plugins_spidmx_libolaspidmx_la_SOURCES = \
    plugins/spidmx/SpiDmxPluginDescription.h
plugins/spidmx/SpiDmxPluginDescription.h: plugins/spidmx/README.md plugins/spidmx/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/spidmx $(top_builddir)/plugins/spidmx/SpiDmxPluginDescription.h

plugins_spidmx_libolaspidmx_la_SOURCES = \
    plugins/spidmx/SpiDmxDevice.cpp \
    plugins/spidmx/SpiDmxDevice.h \
    plugins/spidmx/SpiDmxParser.cpp \
    plugins/spidmx/SpiDmxParser.h \
    plugins/spidmx/SpiDmxPlugin.cpp \
    plugins/spidmx/SpiDmxPlugin.h \
    plugins/spidmx/SpiDmxPort.h \
    plugins/spidmx/SpiDmxThread.cpp \
    plugins/spidmx/SpiDmxThread.h \
    plugins/spidmx/SpiDmxWidget.cpp \
    plugins/spidmx/SpiDmxWidget.h
plugins_spidmx_libolaspidmx_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += plugins/spidmx/README.md
