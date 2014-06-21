include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_SANDNET
plugin_LTLIBRARIES = libolasandnet.la
libolasandnet_la_SOURCES = SandNetCommon.h SandNetDevice.cpp SandNetDevice.h \
                           SandNetNode.cpp SandNetNode.h SandNetPackets.h \
                           SandNetPlugin.cpp SandNetPlugin.h SandNetPort.cpp \
                           SandNetPort.h
libolasandnet_la_LIBADD = ../../common/libolacommon.la
endif
