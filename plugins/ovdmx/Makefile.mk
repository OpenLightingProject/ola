# LIBRARIES
##################################################
if USE_OVDMX
lib_LTLIBRARIES += plugins/ovdmx/libolaovdmx.la
plugins_ovdmx_libolaovdmx_la_SOURCES = \
    plugins/ovdmx/OVDmxDevice.cpp \
    plugins/ovdmx/OVDmxDevice.h \
    plugins/ovdmx/OVDmxPlugin.cpp \
    plugins/ovdmx/OVDmxPlugin.h \
    plugins/ovdmx/OVDmxPort.h \
    plugins/ovdmx/OVDmxThread.cpp \
    plugins/ovdmx/OVDmxThread.h
plugins_ovdmx_libolaovdmx_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif
