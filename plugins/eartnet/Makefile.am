
libdir = $(plugindir)

if HAVE_EARTNET

lib_LTLIBRARIES = libllaeartnet.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CFLAGS = -Wall -Werror -Wformat -W


libllaeartnet_la_SOURCES = eartnetplugin.cpp eartnetplugin.h eartnetdevice.cpp\
						 eartnetdevice.h	eartnetport.cpp eartnetport.h
libllaeartnet_la_LIBADD = @LIBS@  -leartnet

endif
