
libdir = $(plugindir)

if HAVE_ESPNET

lib_LTLIBRARIES = libllashownet.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir)
AM_CFLAGS = -Wall -Werror -Wformat -W

libllashownet_la_SOURCES = shownetplugin.cpp shownetdevice.cpp shownetport.cpp
libllashownet_la_LIBADD = @LIBS@  -lshownet

endif
