
libdir = $(plugindir)

if HAVE_ARTNET

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

lib_LTLIBRARIES = libllaartnet.la
libllaartnet_la_SOURCES = ArtNetPlugin.cpp ArtNetPlugin.h ArtNetDevice.cpp\
                          ArtNetDevice.h ArtNetPort.cpp ArtNetPort.h

libllaartnet_la_LIBADD = -lartnet

endif
