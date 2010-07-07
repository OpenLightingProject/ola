include $(top_srcdir)/common.mk
libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArtNetDevice.h ArtNetNode.h ArtNetPackets.h ArtNetPlugin.h \
             ArtNetPort.h

lib_LTLIBRARIES = libolaartnet.la
libolaartnet_la_SOURCES = ArtNetPlugin.cpp ArtNetDevice.cpp ArtNetNode.cpp \
                          ArtNetPort.cpp
libolaartnet_la_LIBADD = messages/libolaartnetconf.la
