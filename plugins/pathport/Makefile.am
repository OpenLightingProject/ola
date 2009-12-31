libdir = $(plugindir)

EXTRA_DIST = PathportPlugin.h PathportDevice.h PathportPort.h \
             PathportNode.h PathportPackets.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

lib_LTLIBRARIES = libolapathport.la
libolapathport_la_SOURCES = PathportPlugin.cpp PathportDevice.cpp \
                            PathportPort.cpp PathportNode.cpp
libolapathport_la_LIBADD = @LIBS@
