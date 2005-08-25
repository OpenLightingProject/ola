
libdir = $(plugindir)

if HAVE_ESPNET

lib_LTLIBRARIES = libllaespnet.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir)
AM_CFLAGS = -Wall -Werror -Wformat -W

libllaespnet_la_SOURCES = espnetplugin.cpp espnetdevice.cpp espnetport.cpp
libllaespnet_la_LIBADD = @LIBS@  -lespnet

endif
