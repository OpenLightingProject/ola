include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = StageProfiPlugin.h StageProfiDevice.h StageProfiPort.h \
             StageProfiWidget.h StageProfiWidgetLan.h StageProfiWidgetUsb.h

if USE_STAGEPROFI
lib_LTLIBRARIES = libolastageprofi.la
libolastageprofi_la_SOURCES = StageProfiPlugin.cpp StageProfiDevice.cpp \
                              StageProfiPort.cpp StageProfiWidget.cpp \
                              StageProfiWidgetLan.cpp StageProfiWidgetUsb.cpp
libolastageprofi_la_LIBADD = ../../common/libolacommon.la
endif
