libdir = $(third_party_plugindir)

EXTRA_DIST = DummyPlugin.h DummyDevice.h DummyPort.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

lib_LTLIBRARIES = liboladummy.la
liboladummy_la_SOURCES =  DummyPlugin.cpp DummyDevice.cpp DummyPort.cpp
liboladummy_la_LIBADD = @LIBS@
