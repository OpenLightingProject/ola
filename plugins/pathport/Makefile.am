libdir = $(plugindir)

EXTRA_DIST = PathportPlugin.h PathportDevice.h PathportPort.h PathportCommon.h

if HAVE_PATHPORT

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

lib_LTLIBRARIES = libolapathport.la
libolapathport_la_SOURCES = PathportPlugin.cpp PathportDevice.cpp \
                            PathportPort.cpp
libolapathport_la_LIBADD = @LIBS@  -lpathport

endif
