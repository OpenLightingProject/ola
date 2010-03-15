libdir = $(plugindir)
EXTRA_DIST = UsbDmxPlugin.h UsbDmxDevice.h UsbDmxPort.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

lib_LTLIBRARIES = libolausbdmx.la
libolausbdmx_la_SOURCES = UsbDmxPlugin.cpp UsbDmxDevice.cpp UsbDmxPort.cpp
libolausbdmx_la_CXXFLAGS = $(LIBUSB_CFLAGS)
libolausbdmx_la_LIBADD = $(LIBUSB_LIBS)
