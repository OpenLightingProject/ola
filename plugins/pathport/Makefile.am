include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_PATHPORT
plugin_LTLIBRARIES = libolapathport.la
libolapathport_la_SOURCES = PathportDevice.cpp PathportDevice.h \
                            PathportNode.cpp PathportNode.h \
                            PathportPackets.h PathportPlugin.cpp \
                            PathportPlugin.h PathportPort.cpp PathportPort.h
libolapathport_la_LIBADD = ../../common/libolacommon.la
endif
