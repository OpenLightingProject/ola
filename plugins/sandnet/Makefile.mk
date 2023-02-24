# LIBRARIES
##################################################
if USE_SANDNET
lib_LTLIBRARIES += plugins/sandnet/libolasandnet.la
plugins_sandnet_libolasandnet_la_SOURCES = \
    plugins/sandnet/SandNetCommon.h \
    plugins/sandnet/SandNetDevice.cpp \
    plugins/sandnet/SandNetDevice.h \
    plugins/sandnet/SandNetNode.cpp \
    plugins/sandnet/SandNetNode.h \
    plugins/sandnet/SandNetPackets.h \
    plugins/sandnet/SandNetPlugin.cpp \
    plugins/sandnet/SandNetPlugin.h \
    plugins/sandnet/SandNetPort.cpp \
    plugins/sandnet/SandNetPort.h
plugins_sandnet_libolasandnet_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif
