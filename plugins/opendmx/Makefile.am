include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_OPENDMX
plugin_LTLIBRARIES = libolaopendmx.la
libolaopendmx_la_SOURCES = OpenDmxDevice.cpp OpenDmxDevice.h \
                           OpenDmxPlugin.cpp OpenDmxPlugin.h OpenDmxPort.h \
                           OpenDmxThread.cpp OpenDmxThread.h
libolaopendmx_la_LIBADD = ../../common/libolacommon.la
endif
