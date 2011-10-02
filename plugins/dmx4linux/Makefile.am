include $(top_srcdir)/common.mk
libdir = $(plugindir)

EXTRA_DIST = Dmx4LinuxPlugin.h Dmx4LinuxDevice.h Dmx4LinuxPort.h \
             Dmx4LinuxSocket.h

if HAVE_DMX4LINUX
  lib_LTLIBRARIES = liboladmx4linux.la
  liboladmx4linux_la_SOURCES = Dmx4LinuxPlugin.cpp Dmx4LinuxDevice.cpp \
                               Dmx4LinuxPort.cpp
  liboladmx4linux_la_LIBADD = ../../common/libolacommon.la
endif
