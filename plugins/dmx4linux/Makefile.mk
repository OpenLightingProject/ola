# LIBRARIES
##################################################
if USE_DMX4LINUX
lib_LTLIBRARIES += plugins/dmx4linux/liboladmx4linux.la

# Plugin description is generated from README.md
built_sources += plugins/dmx4linux/Dmx4LinuxPluginDescription.h
nodist_plugins_dmx4linux_liboladmx4linux_la_SOURCES = \
    plugins/dmx4linux/Dmx4LinuxPluginDescription.h
plugins/dmx4linux/Dmx4LinuxPluginDescription.h: plugins/dmx4linux/README.md plugins/dmx4linux/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/dmx4linux $(top_builddir)/plugins/dmx4linux/Dmx4LinuxPluginDescription.h

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

EXTRA_DIST += plugins/dmx4linux/README.md
