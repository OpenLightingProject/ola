libdir = $(plugindir)

EXTRA_DIST = SandNetPlugin.h SandNetDevice.h SandNetPort.h SandNetNode.h \
             SandNetCommon.h SandNetPackets.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

lib_LTLIBRARIES = libolasandnet.la
libolasandnet_la_SOURCES = SandNetPlugin.cpp SandNetDevice.cpp \
                           SandNetPort.cpp SandNetNode.cpp
libolasandnet_la_LIBADD = @LIBS@
