# LIBRARIES
##################################################
if USE_SANDNET
lib_LTLIBRARIES += plugins/sandnet/libolasandnet.la

# Plugin description is generated from README.md
built_sources += plugins/sandnet/SandNetPluginDescription.h
nodist_plugins_sandnet_libolasandnet_la_SOURCES = \
    plugins/sandnet/SandNetPluginDescription.h
plugins/sandnet/SandNetPluginDescription.h: plugins/sandnet/README.md plugins/sandnet/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/sandnet $(top_builddir)/plugins/sandnet/SandNetPluginDescription.h

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

EXTRA_DIST += plugins/sandnet/README.md
