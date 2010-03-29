libdir = $(plugindir)
EXTRA_DIST = UsbDmxPlugin.h UsbDevice.h VellemanDevice.h VellemanOutputPort.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

if HAVE_LIBUSB
  lib_LTLIBRARIES = libolausbdmx.la
  libolausbdmx_la_SOURCES = UsbDmxPlugin.cpp VellemanDevice.cpp \
                            VellemanOutputPort.cpp
  libolausbdmx_la_CXXFLAGS = $(LIBUSB_CFLAGS)
  libolausbdmx_la_LIBADD = $(LIBUSB_LIBS)
endif
