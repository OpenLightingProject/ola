
libdir = $(plugindir)

if HAVE_SHOWNET

lib_LTLIBRARIES = libllashownet.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir)
AM_CFLAGS = -Wall -Werror -Wformat -W

libllashownet_la_SOURCES = shownetplugin.cpp shownetplugin.h shownetdevice.cpp\
							shownetdevice.h shownetport.cpp shownetport.h common.h
libllashownet_la_LIBADD = @LIBS@  -lshownet

endif
