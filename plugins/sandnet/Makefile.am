
libdir = $(plugindir)

if HAVE_SANDNET

lib_LTLIBRARIES = libllasandnet.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CFLAGS = -Wall -Werror -Wformat -W

libllasandnet_la_SOURCES = sandnetplugin.cpp sandnetplugin.h sandnetdevice.cpp\
							sandnetdevice.h sandnetport.cpp sandnetport.h common.h
libllasandnet_la_LIBADD = @LIBS@  -lsandnet

endif
