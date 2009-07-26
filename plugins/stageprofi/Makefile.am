
libdir = $(plugindir)

lib_LTLIBRARIES = libolastageprofi.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include -I./conf_msgs
AM_CXXFLAGS = -Wall -Wformat -W

libolastageprofi_la_SOURCES = StageProfiPlugin.cpp StageProfiDevice.cpp StageProfiPort.cpp \
							  StageProfiWidget.cpp StageProfiWidgetLan.cpp StageProfiWidgetUsb.cpp

EXTRA_DIST = StageProfiPlugin.h StageProfiDevice.h StageProfiPort.h \
			  StageProfiWidget.h StageProfiWidgetLan.h StageProfiWidgetUsb.h
