include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_DMX4LINUX
plugin_LTLIBRARIES = liboladmx4linux.la
liboladmx4linux_la_SOURCES = Dmx4LinuxDevice.cpp Dmx4LinuxDevice.h \
                             Dmx4LinuxPlugin.cpp Dmx4LinuxPlugin.h \
                             Dmx4LinuxPort.cpp Dmx4LinuxPort.h \
                             Dmx4LinuxSocket.h
liboladmx4linux_la_LIBADD = ../../common/libolacommon.la
endif
