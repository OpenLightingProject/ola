# LIBRARIES
##################################################
if USE_KARATE
# Plugin description is generated from README.md
BUILT_SOURCES += plugins/karate/PluginDescription.h
plugins/karate/PluginDescription.h: plugins/karate/README.md plugins/karate/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh plugins/karate PluginDescription.h

lib_LTLIBRARIES += plugins/karate/libolakarate.la
plugins_karate_libolakarate_la_SOURCES = \
    plugins/karate/PluginDescription.h \
    plugins/karate/KaratePlugin.cpp \
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
