
libdir = $(plugindir)
SUBDIRS = messages

if HAVE_ARTNET

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat

lib_LTLIBRARIES = libolaartnet.la
libolaartnet_la_SOURCES = ArtNetPlugin.cpp ArtNetPlugin.h ArtNetDevice.cpp\
                          ArtNetDevice.h ArtNetPort.cpp ArtNetPort.h
libolaartnet_la_LIBADD = messages/libolaartnetconf.la -lartnet

endif
