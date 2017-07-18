# LIBRARIES
##################################################
if USE_OPENPIXELCONTROL

# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/openpixelcontrol/libolaopc.la
plugins_openpixelcontrol_libolaopc_la_SOURCES = \
    plugins/openpixelcontrol/OpcClient.cpp \
    plugins/openpixelcontrol/OpcClient.h \
    plugins/openpixelcontrol/OpcConstants.h \
    plugins/openpixelcontrol/OpcServer.cpp \
    plugins/openpixelcontrol/OpcServer.h
plugins_openpixelcontrol_libolaopc_la_LIBADD = \
    common/libolacommon.la

lib_LTLIBRARIES += plugins/openpixelcontrol/libolaopenpixelcontrol.la

# Plugin description is generated from README.md
built_sources += plugins/openpixelcontrol/OpcPluginDescription.h
nodist_plugins_openpixelcontrol_libolaopenpixelcontrol_la_SOURCES = \
    plugins/openpixelcontrol/OpcPluginDescription.h
plugins/openpixelcontrol/OpcPluginDescription.h: plugins/openpixelcontrol/README.md plugins/openpixelcontrol/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/openpixelcontrol $(top_builddir)/plugins/openpixelcontrol/OpcPluginDescription.h

plugins_openpixelcontrol_libolaopenpixelcontrol_la_SOURCES = \
    plugins/openpixelcontrol/OpcDevice.cpp \
    plugins/openpixelcontrol/OpcDevice.h \
    plugins/openpixelcontrol/OpcPlugin.cpp \
    plugins/openpixelcontrol/OpcPlugin.h \
    plugins/openpixelcontrol/OpcPort.cpp \
    plugins/openpixelcontrol/OpcPort.h

plugins_openpixelcontrol_libolaopenpixelcontrol_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/openpixelcontrol/libolaopc.la

# TESTS
##################################################
test_programs += \
    plugins/openpixelcontrol/OpcClientTester \
    plugins/openpixelcontrol/OpcServerTester

plugins_openpixelcontrol_OpcClientTester_SOURCES = \
    plugins/openpixelcontrol/OpcClientTest.cpp
plugins_openpixelcontrol_OpcClientTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_openpixelcontrol_OpcClientTester_LDADD = \
    $(COMMON_TESTING_LIBS) \
    plugins/openpixelcontrol/libolaopc.la

plugins_openpixelcontrol_OpcServerTester_SOURCES = \
    plugins/openpixelcontrol/OpcServerTest.cpp
plugins_openpixelcontrol_OpcServerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_openpixelcontrol_OpcServerTester_LDADD = \
    $(COMMON_TESTING_LIBS) \
    plugins/openpixelcontrol/libolaopc.la
endif

EXTRA_DIST += plugins/openpixelcontrol/README.md
