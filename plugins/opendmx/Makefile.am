include $(top_srcdir)/common.mk

libdir = $(plugindir)
EXTRA_DIST = OpenDmxPlugin.h OpenDmxDevice.h OpenDmxPort.h OpenDmxThread.h

if USE_OPENDMX
lib_LTLIBRARIES = libolaopendmx.la
libolaopendmx_la_SOURCES =  OpenDmxPlugin.cpp OpenDmxDevice.cpp \
                            OpenDmxThread.cpp
libolaopendmx_la_LIBADD = ../../common/libolacommon.la
endif
