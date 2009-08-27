libdir = $(plugindir)
EXTRA_DIST = Dmx4LinuxPlugin.h Dmx4LinuxDevice.h Dmx4LinuxPort.h

if HAVE_DMX4LINUX

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

lib_LTLIBRARIES = liboladmx4linux.la
liboladmx4linux_la_SOURCES = Dmx4LinuxPlugin.cpp Dmx4LinuxDevice.cpp \
                             Dmx4LinuxPort.cpp

endif
