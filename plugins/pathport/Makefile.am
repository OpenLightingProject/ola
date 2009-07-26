
libdir = $(plugindir)

if HAVE_PATHPORT

lib_LTLIBRARIES = libolapathport.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

libolapathport_la_SOURCES = PathportPlugin.cpp PathportDevice.cpp PathportPort.cpp

libolapathport_la_LIBADD = @LIBS@  -lpathport

EXTRA_DIST = PathportPlugin.h PathportDevice.h PathportPort.h PathportCommon.h

endif
