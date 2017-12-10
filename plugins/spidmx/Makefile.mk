# LIBRARIES
##################################################
if USE_SPIDMX
lib_LTLIBRARIES += plugins/spidmx/libolaspidmx.la

# Plugin description is generated from README.md
built_sources += plugins/spidmx/SPIDMXPluginDescription.h
nodist_plugins_spidmx_libolaspidmx_la_SOURCES = \
    plugins/spidmx/SPIDMXPluginDescription.h
plugins/spidmx/SPIDMXPluginDescription.h: plugins/spidmx/README.md plugins/spidmx/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/spidmx $(top_builddir)/plugins/spidmx/SPIDMXPluginDescription.h

plugins_spidmx_libolaspidmx_la_SOURCES = \
    plugins/spidmx/SPIDMXDevice.cpp \
    plugins/spidmx/SPIDMXDevice.h \
    plugins/spidmx/SPIDMXParser.cpp \
    plugins/spidmx/SPIDMXParser.h \
    plugins/spidmx/SPIDMXPlugin.cpp \
    plugins/spidmx/SPIDMXPlugin.h \
    plugins/spidmx/SPIDMXPort.h \
    plugins/spidmx/SPIDMXThread.cpp \
    plugins/spidmx/SPIDMXThread.h \
    plugins/spidmx/SPIDMXWidget.cpp \
    plugins/spidmx/SPIDMXWidget.h
plugins_spidmx_libolaspidmx_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += plugins/spidmx/README.md
