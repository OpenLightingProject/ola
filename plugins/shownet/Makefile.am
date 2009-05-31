
libdir = $(plugindir)

lib_LTLIBRARIES = libllashownet.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

libllashownet_la_SOURCES = ShowNetPlugin.cpp ShowNetPlugin.h ShowNetDevice.cpp\
                           ShowNetDevice.h ShowNetPort.cpp ShowNetPort.h
libllashownet_la_LIBADD = @LIBS@
