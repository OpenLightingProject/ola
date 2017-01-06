# LIBRARIES
##################################################
if USE_KARATE
# Plugin description is generated from README.md
built_sources += plugins/karate/KaratePluginDescription.h
plugins/karate/KaratePluginDescription.h: plugins/karate/README.md plugins/karate/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/karate $(top_builddir)/plugins/karate/KaratePluginDescription.h

lib_LTLIBRARIES += plugins/karate/libolakarate.la
plugins_karate_libolakarate_la_SOURCES = \
    plugins/karate/KaratePlugin.cpp \
    plugins/karate/KaratePluginDescription.h \
    plugins/karate/KarateDevice.cpp \
    plugins/karate/KarateThread.cpp \
    plugins/karate/KarateLight.cpp \
    plugins/karate/KaratePlugin.h \
    plugins/karate/KarateDevice.h \
    plugins/karate/KaratePort.h \
    plugins/karate/KarateThread.h \
    plugins/karate/KarateLight.h
plugins_karate_libolakarate_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += \
    plugins/karate/README.md \
    plugins/karate/README.protocol
