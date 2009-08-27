libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArtNetPlugin.h ArtNetDevice.h ArtNetPort.h

if HAVE_ARTNET

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat

lib_LTLIBRARIES = libolaartnet.la
libolaartnet_la_SOURCES = ArtNetPlugin.cpp ArtNetDevice.cpp ArtNetPort.cpp
libolaartnet_la_LIBADD = messages/libolaartnetconf.la -lartnet

endif
