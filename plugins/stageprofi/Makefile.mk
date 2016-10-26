# LIBRARIES
##################################################
if USE_STAGEPROFI
lib_LTLIBRARIES += plugins/stageprofi/libolastageprofi.la
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

EXTRA_DIST += \
    plugins/stageprofi/README.md

