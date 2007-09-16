
libdir = $(plugindir)

if HAVE_DMX4LINUX

lib_LTLIBRARIES = liblladmx4linux.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

liblladmx4linux_la_SOURCES = Dmx4LinuxPlugin.cpp Dmx4LinuxDevice.cpp Dmx4LinuxPort.cpp

EXTRA_DIST = Dmx4LinuxPlugin.h Dmx4LinuxDevice.h Dmx4LinuxPort.h

endif
