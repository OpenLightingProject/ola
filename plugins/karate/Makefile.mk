# LIBRARIES
##################################################
if USE_KARATE
BUILT_SOURCES += plugins/karate/PluginDescription.cpp
lib_LTLIBRARIES += plugins/karate/libolakarate.la
plugins_karate_libolakarate_la_SOURCES = \
    plugins/karate/PluginDescription.cpp \
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
