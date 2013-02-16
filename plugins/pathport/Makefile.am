include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = PathportPlugin.h PathportDevice.h PathportPort.h \
             PathportNode.h PathportPackets.h

if USE_PATHPORT
lib_LTLIBRARIES = libolapathport.la
libolapathport_la_SOURCES = PathportPlugin.cpp PathportDevice.cpp \
                            PathportPort.cpp PathportNode.cpp
libolapathport_la_LIBADD = ../../common/libolacommon.la
endif
