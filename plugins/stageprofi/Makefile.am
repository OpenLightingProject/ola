include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_STAGEPROFI
plugin_LTLIBRARIES = libolastageprofi.la
libolastageprofi_la_SOURCES = StageProfiDevice.cpp StageProfiDevice.h \
                              StageProfiPlugin.cpp StageProfiPlugin.h \
                              StageProfiPort.cpp StageProfiPort.h \
                              StageProfiWidget.cpp StageProfiWidget.h \
                              StageProfiWidgetLan.cpp StageProfiWidgetLan.h \
                              StageProfiWidgetUsb.cpp StageProfiWidgetUsb.h
libolastageprofi_la_LIBADD = ../../common/libolacommon.la
endif
