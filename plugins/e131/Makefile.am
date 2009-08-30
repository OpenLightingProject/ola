libdir = $(plugindir)
SUBDIRS = e131
EXTRA_DIST = E131Plugin.h E131Device.h E131Port.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

lib_LTLIBRARIES = libolae131.la
libolae131_la_SOURCES = E131Plugin.cpp E131Device.cpp E131Port.cpp
libolae131_la_LIBADD = @LIBS@
