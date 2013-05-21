include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = SandNetPlugin.h SandNetDevice.h SandNetPort.h SandNetNode.h \
             SandNetCommon.h SandNetPackets.h

if USE_SANDNET
lib_LTLIBRARIES = libolasandnet.la
libolasandnet_la_SOURCES = SandNetPlugin.cpp SandNetDevice.cpp \
                           SandNetPort.cpp SandNetNode.cpp
libolasandnet_la_LIBADD = ../../common/libolacommon.la
endif
