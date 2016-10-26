# LIBRARIES
##################################################
if USE_OPENDMX
lib_LTLIBRARIES += plugins/opendmx/libolaopendmx.la
plugins_opendmx_libolaopendmx_la_SOURCES = \
    plugins/opendmx/OpenDmxDevice.cpp \
    plugins/opendmx/OpenDmxDevice.h \
    plugins/opendmx/OpenDmxPlugin.cpp \
    plugins/opendmx/OpenDmxPlugin.h \
    plugins/opendmx/OpenDmxPort.h \
    plugins/opendmx/OpenDmxThread.cpp \
    plugins/opendmx/OpenDmxThread.h
plugins_opendmx_libolaopendmx_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += \
    plugins/opendmx/README.md
