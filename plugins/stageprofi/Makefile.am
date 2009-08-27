libdir = $(plugindir)

EXTRA_DIST = StageProfiPlugin.h StageProfiDevice.h StageProfiPort.h \
             StageProfiWidget.h StageProfiWidgetLan.h StageProfiWidgetUsb.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

lib_LTLIBRARIES = libolastageprofi.la
libolastageprofi_la_SOURCES = StageProfiPlugin.cpp StageProfiDevice.cpp \
                              StageProfiPort.cpp StageProfiWidget.cpp \
                              StageProfiWidgetLan.cpp StageProfiWidgetUsb.cpp

