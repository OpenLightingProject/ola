libdir = $(plugindir)

EXTRA_DIST = SandnetPlugin.h SandnetDevice.h SandnetPort.h SandnetCommon.h

if HAVE_SANDNET

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

lib_LTLIBRARIES = libolasandnet.la
libolasandnet_la_SOURCES = SandnetPlugin.cpp SandnetDevice.cpp SandnetPort.cpp
libolasandnet_la_LIBADD = @LIBS@  -lsandnet

endif
