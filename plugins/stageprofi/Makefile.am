
libdir = $(plugindir)

lib_LTLIBRARIES = libllastageprofi.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include -I./conf_msgs
AM_CXXFLAGS = -Wall -Wformat -W

libllastageprofi_la_SOURCES = stageprofiplugin.cpp stageprofidevice.cpp StageProfiWidget.cpp stageprofiport.cpp

EXTRA_DIST = StageProfiWidget.h stageprofiplugin.h stageprofidevice.h stageprofiport.h
