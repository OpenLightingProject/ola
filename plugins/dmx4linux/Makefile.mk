# LIBRARIES
##################################################
if USE_DMX4LINUX
lib_LTLIBRARIES += plugins/dmx4linux/liboladmx4linux.la
plugins_dmx4linux_liboladmx4linux_la_SOURCES = \
    plugins/dmx4linux/Dmx4LinuxDevice.cpp \
    plugins/dmx4linux/Dmx4LinuxDevice.h \
    plugins/dmx4linux/Dmx4LinuxPlugin.cpp \
    plugins/dmx4linux/Dmx4LinuxPlugin.h \
    plugins/dmx4linux/Dmx4LinuxPort.cpp \
    plugins/dmx4linux/Dmx4LinuxPort.h \
    plugins/dmx4linux/Dmx4LinuxSocket.h
plugins_dmx4linux_liboladmx4linux_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += \
    plugins/dmx4linux/README.md
