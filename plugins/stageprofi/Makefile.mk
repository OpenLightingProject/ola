# LIBRARIES
##################################################
if USE_STAGEPROFI
plugin_LTLIBRARIES += plugins/stageprofi/libolastageprofi.la
plugins_stageprofi_libolastageprofi_la_SOURCES = \
    plugins/stageprofi/StageProfiDevice.cpp \
    plugins/stageprofi/StageProfiDevice.h \
    plugins/stageprofi/StageProfiPlugin.cpp \
    plugins/stageprofi/StageProfiPlugin.h \
    plugins/stageprofi/StageProfiPort.cpp \
    plugins/stageprofi/StageProfiPort.h \
    plugins/stageprofi/StageProfiWidget.cpp \
    plugins/stageprofi/StageProfiWidget.h \
    plugins/stageprofi/StageProfiWidgetLan.cpp \
    plugins/stageprofi/StageProfiWidgetLan.h \
    plugins/stageprofi/StageProfiWidgetUsb.cpp \
    plugins/stageprofi/StageProfiWidgetUsb.h
plugins_stageprofi_libolastageprofi_la_LIBADD = common/libolacommon.la
endif
