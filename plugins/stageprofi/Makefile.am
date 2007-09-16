
libdir = $(plugindir)

lib_LTLIBRARIES = libllastageprofi.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include -I./conf_msgs
AM_CXXFLAGS = -Wall -Wformat -W

libllastageprofi_la_SOURCES = stageprofiplugin.cpp stageprofidevice.cpp stageprofiport.cpp \
							  StageProfiWidget.cpp StageProfiWidgetLan.cpp StageProfiWidgetUsb.cpp

EXTRA_DIST = stageprofiplugin.h stageprofidevice.h stageprofiport.h \
			  StageProfiWidget.h StageProfiWidgetLan.h StageProfiWidgetUsb.h
