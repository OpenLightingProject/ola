
libdir = $(plugindir)

if HAVE_ARTNET

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

lib_LTLIBRARIES = libllaartnet.la
libllaartnet_la_SOURCES = artnetplugin.cpp artnetplugin.h artnetdevice.cpp\
						 artnetdevice.h	artnetport.cpp artnetport.h

libllaartnet_la_LIBADD = @LIBS@  -lartnet -lllaartnetconf

SUBDIRS = conf_msgs

endif
