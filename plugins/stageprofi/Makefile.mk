# LIBRARIES
##################################################
if USE_STAGEPROFI
lib_LTLIBRARIES += plugins/stageprofi/libolastageprofi.la

# Plugin description is generated from README.md
built_sources += plugins/stageprofi/StageProfiPluginDescription.h
nodist_plugins_stageprofi_libolastageprofi_la_SOURCES = \
    plugins/stageprofi/StageProfiPluginDescription.h
plugins/stageprofi/StageProfiPluginDescription.h: plugins/stageprofi/README.md plugins/stageprofi/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/stageprofi $(top_builddir)/plugins/stageprofi/StageProfiPluginDescription.h

plugins_stageprofi_libolastageprofi_la_SOURCES = \
    plugins/stageprofi/StageProfiDetector.cpp \
    plugins/stageprofi/StageProfiDetector.h \
    plugins/stageprofi/StageProfiDevice.cpp \
    plugins/stageprofi/StageProfiDevice.h \
    plugins/stageprofi/StageProfiPlugin.cpp \
    plugins/stageprofi/StageProfiPlugin.h \
    plugins/stageprofi/StageProfiPort.cpp \
    plugins/stageprofi/StageProfiPort.h \
    plugins/stageprofi/StageProfiWidget.cpp \
    plugins/stageprofi/StageProfiWidget.h
plugins_stageprofi_libolastageprofi_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += plugins/stageprofi/README.md
