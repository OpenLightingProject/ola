libdir = $(plugindir)
if HAVE_SANDNET

lib_LTLIBRARIES = libllasandnet.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

libllasandnet_la_SOURCES = SandnetPlugin.cpp SandnetPlugin.h SandnetDevice.cpp\
                            SandnetDevice.h SandnetPort.cpp SandnetPort.h SandnetCommon.h
libllasandnet_la_LIBADD = @LIBS@  -lsandnet

endif
