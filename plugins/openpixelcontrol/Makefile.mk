# LIBRARIES
##################################################
if USE_OPENPIXELCONTROL

# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/openpixelcontrol/libolaopc.la
plugins_openpixelcontrol_libolaopc_la_SOURCES = \
    plugins/openpixelcontrol/OPCClient.cpp \
    plugins/openpixelcontrol/OPCClient.h \
    plugins/openpixelcontrol/OPCConstants.h \
    plugins/openpixelcontrol/OPCServer.cpp \
    plugins/openpixelcontrol/OPCServer.h
plugins_openpixelcontrol_libolaopc_la_LIBADD = \
    common/libolacommon.la

lib_LTLIBRARIES += plugins/openpixelcontrol/libolaopenpixelcontrol.la

# Plugin description is generated from README.md
built_sources += plugins/openpixelcontrol/OPCPluginDescription.h
nodist_plugins_openpixelcontrol_libolaopenpixelcontrol_la_SOURCES = \
    plugins/openpixelcontrol/OPCPluginDescription.h
plugins/openpixelcontrol/OPCPluginDescription.h: plugins/openpixelcontrol/README.md plugins/openpixelcontrol/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/openpixelcontrol $(top_builddir)/plugins/openpixelcontrol/OPCPluginDescription.h

plugins_openpixelcontrol_libolaopenpixelcontrol_la_SOURCES = \
    plugins/openpixelcontrol/OPCDevice.cpp \
    plugins/openpixelcontrol/OPCDevice.h \
    plugins/openpixelcontrol/OPCPlugin.cpp \
    plugins/openpixelcontrol/OPCPlugin.h \
    plugins/openpixelcontrol/OPCPort.cpp \
    plugins/openpixelcontrol/OPCPort.h

plugins_openpixelcontrol_libolaopenpixelcontrol_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/openpixelcontrol/libolaopc.la

# TESTS
##################################################
test_programs += \
    plugins/openpixelcontrol/OPCClientTester \
    plugins/openpixelcontrol/OPCServerTester

plugins_openpixelcontrol_OPCClientTester_SOURCES = \
    plugins/openpixelcontrol/OPCClientTest.cpp
plugins_openpixelcontrol_OPCClientTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_openpixelcontrol_OPCClientTester_LDADD = \
    $(COMMON_TESTING_LIBS) \
    plugins/openpixelcontrol/libolaopc.la

plugins_openpixelcontrol_OPCServerTester_SOURCES = \
    plugins/openpixelcontrol/OPCServerTest.cpp
plugins_openpixelcontrol_OPCServerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_openpixelcontrol_OPCServerTester_LDADD = \
    $(COMMON_TESTING_LIBS) \
    plugins/openpixelcontrol/libolaopc.la
endif

EXTRA_DIST += plugins/openpixelcontrol/README.md
