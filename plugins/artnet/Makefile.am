include $(top_srcdir)/common.mk
libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArtNetPlugin.h ArtNetDevice.h ArtNetPort.h

if HAVE_ARTNET
  lib_LTLIBRARIES = libolaartnet.la
  libolaartnet_la_SOURCES = ArtNetPlugin.cpp ArtNetDevice.cpp ArtNetPort.cpp
  libolaartnet_la_LIBADD = messages/libolaartnetconf.la -lartnet
endif
