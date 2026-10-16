# LIBRARIES
##################################################
if USE_SIGNET
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/signet/libolasignetnode.la
plugins_signet_libolasignetnode_la_SOURCES = \
    plugins/signet/SigNetEnums.h \
    plugins/signet/SigNetNode.cpp \
    plugins/signet/SigNetNode.h \
    plugins/signet/SigNetTIDs.h
plugins_signet_libolasignetnode_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(libcoap_CFLAGS)
plugins_signet_libolasignetnode_la_LIBADD = $(libcoap_LIBS)

lib_LTLIBRARIES += plugins/signet/libolasignet.la

# Plugin description is generated from README.md
built_sources += plugins/signet/SigNetPluginDescription.h
nodist_plugins_signet_libolasignet_la_SOURCES = \
    plugins/signet/SigNetPluginDescription.h
plugins/signet/SigNetPluginDescription.h: plugins/signet/README.md plugins/signet/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/signet $(top_builddir)/plugins/signet/SigNetPluginDescription.h

plugins_signet_libolasignet_la_SOURCES = \
    plugins/signet/SigNetDevice.cpp \
    plugins/signet/SigNetDevice.h \
    plugins/signet/SigNetPlugin.cpp \
    plugins/signet/SigNetPlugin.h \
    plugins/signet/SigNetPort.cpp \
    plugins/signet/SigNetPort.h
plugins_signet_libolasignet_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(libcoap_CFLAGS)
plugins_signet_libolasignet_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/signet/libolasignetnode.la
endif

EXTRA_DIST += \
    plugins/signet/README.md
