# This file is included directly in Makefile.am rather than via
# olad/Makefile.mk due to the order of dependencies between them; we need to
# build olad/plugin_api, then the plugins, then olad

# HEADERS
##################################################
noinst_HEADERS += olad/plugin_api/TestCommon.h

# LIBRARIES
##################################################
# lib olaserverplugininterface
lib_LTLIBRARIES += olad/plugin_api/libolaserverplugininterface.la

olad_plugin_api_libolaserverplugininterface_la_SOURCES = \
    olad/plugin_api/Client.cpp \
    olad/plugin_api/Client.h \
    olad/plugin_api/Device.cpp \
    olad/plugin_api/DeviceManager.cpp \
    olad/plugin_api/DeviceManager.h \
    olad/plugin_api/DmxSource.cpp \
    olad/plugin_api/Plugin.cpp \
    olad/plugin_api/PluginAdaptor.cpp \
    olad/plugin_api/Port.cpp \
    olad/plugin_api/PortBroker.cpp \
    olad/plugin_api/PortManager.cpp \
    olad/plugin_api/PortManager.h \
    olad/plugin_api/Preferences.cpp \
    olad/plugin_api/Universe.cpp \
    olad/plugin_api/UniverseStore.cpp \
    olad/plugin_api/UniverseStore.h
olad_plugin_api_libolaserverplugininterface_la_CXXFLAGS = $(COMMON_CXXFLAGS)
olad_plugin_api_libolaserverplugininterface_la_LIBADD = \
    common/libolacommon.la \
    common/web/libolaweb.la \
    ola/libola.la

# TESTS
##################################################
test_programs += \
    olad/plugin_api/ClientTester \
    olad/plugin_api/DeviceTester \
    olad/plugin_api/DmxSourceTester \
    olad/plugin_api/PortTester \
    olad/plugin_api/PreferencesTester \
    olad/plugin_api/UniverseTester

COMMON_OLAD_PLUGIN_API_TEST_LDADD = \
    $(COMMON_TESTING_LIBS) \
    $(libprotobuf_LIBS) \
    olad/plugin_api/libolaserverplugininterface.la \
    common/libolacommon.la

olad_plugin_api_ClientTester_SOURCES = olad/plugin_api/ClientTest.cpp
olad_plugin_api_ClientTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_plugin_api_ClientTester_LDADD = $(COMMON_OLAD_PLUGIN_API_TEST_LDADD)

olad_plugin_api_DeviceTester_SOURCES = olad/plugin_api/DeviceManagerTest.cpp \
                                       olad/plugin_api/DeviceTest.cpp
olad_plugin_api_DeviceTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_plugin_api_DeviceTester_LDADD = $(COMMON_OLAD_PLUGIN_API_TEST_LDADD)

olad_plugin_api_DmxSourceTester_SOURCES = olad/plugin_api/DmxSourceTest.cpp
olad_plugin_api_DmxSourceTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_plugin_api_DmxSourceTester_LDADD = $(COMMON_OLAD_PLUGIN_API_TEST_LDADD)

olad_plugin_api_PortTester_SOURCES = olad/plugin_api/PortTest.cpp \
                                     olad/plugin_api/PortManagerTest.cpp
olad_plugin_api_PortTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_plugin_api_PortTester_LDADD = $(COMMON_OLAD_PLUGIN_API_TEST_LDADD)

olad_plugin_api_PreferencesTester_SOURCES = olad/plugin_api/PreferencesTest.cpp
olad_plugin_api_PreferencesTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_plugin_api_PreferencesTester_LDADD = $(COMMON_OLAD_PLUGIN_API_TEST_LDADD)

olad_plugin_api_UniverseTester_SOURCES = olad/plugin_api/UniverseTest.cpp
olad_plugin_api_UniverseTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_plugin_api_UniverseTester_LDADD = $(COMMON_OLAD_PLUGIN_API_TEST_LDADD)
