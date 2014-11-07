include olad/www/Makefile.mk

dist_noinst_DATA += olad/testdata/test_preferences.conf

# HEADERS
##################################################
oladinclude_HEADERS += olad/OlaDaemon.h olad/OlaServer.h

noinst_HEADERS += olad/TestCommon.h

# LIBRARIES
##################################################
ola_server_plugin_interface_sources = \
    olad/Device.cpp \
    olad/DeviceManager.cpp \
    olad/DeviceManager.h \
    olad/Plugin.cpp \
    olad/PluginAdaptor.cpp \
    olad/PortBroker.cpp \
    olad/PortManager.cpp \
    olad/PortManager.h

ola_server_sources = \
    olad/Client.cpp \
    olad/Client.h \
    olad/ClientBroker.cpp \
    olad/ClientBroker.h \
    olad/DiscoveryAgent.cpp \
    olad/DiscoveryAgent.h \
    olad/DmxSource.cpp \
    olad/DynamicPluginLoader.cpp \
    olad/DynamicPluginLoader.h \
    olad/HttpServerActions.h \
    olad/OlaServerServiceImpl.cpp \
    olad/OlaServerServiceImpl.h \
    olad/OladHTTPServer.h \
    olad/PluginLoader.h \
    olad/PluginManager.cpp \
    olad/PluginManager.h \
    olad/Port.cpp \
    olad/Preferences.cpp \
    olad/RDMHTTPModule.h \
    olad/Universe.cpp \
    olad/UniverseStore.cpp \
    olad/UniverseStore.h
ola_server_additional_libs =

if HAVE_DNSSD
ola_server_sources += olad/BonjourDiscoveryAgent.h \
                      olad/BonjourDiscoveryAgent.cpp
endif

if HAVE_AVAHI
ola_server_sources += olad/AvahiDiscoveryAgent.h olad/AvahiDiscoveryAgent.cpp
ola_server_additional_libs += $(avahi_LIBS)
endif

if HAVE_LIBMICROHTTPD
ola_server_sources += olad/HttpServerActions.cpp \
                      olad/OladHTTPServer.cpp \
                      olad/RDMHTTPModule.cpp
ola_server_additional_libs += common/http/libolahttp.la
endif

# lib olaserver
lib_LTLIBRARIES += olad/libolaserverplugininterface.la olad/libolaserver.la

olad_libolaserverplugininterface_la_SOURCES = \
    $(ola_server_plugin_interface_sources)
olad_libolaserverplugininterface_la_CXXFLAGS = $(COMMON_CXXFLAGS)
olad_libolaserverplugininterface_la_LIBADD = common/libolacommon.la \
                                             common/web/libolaweb.la \
                                             ola/libola.la

olad_libolaserver_la_SOURCES = $(ola_server_sources) \
                               olad/OlaServer.cpp \
                               olad/OlaDaemon.cpp
olad_libolaserver_la_CXXFLAGS = $(COMMON_CXXFLAGS) \
                                -DHTTP_DATA_DIR=\"${www_datadir}\"
olad_libolaserver_la_LIBADD = $(PLUGIN_LIBS) \
                              common/libolacommon.la \
                              common/web/libolaweb.la \
                              ola/libola.la \
                              olad/libolaserverplugininterface.la \
                              $(ola_server_additional_libs)
# Simon: I'm not too sure about this but it seems that because PLUGIN_LIBS is
# determined at configure time, we need to add them here.
EXTRA_olad_libolaserver_la_DEPENDENCIES = $(PLUGIN_LIBS)

if USE_LIBUSB
olad_libolaserver_la_CXXFLAGS += $(libusb_CFLAGS)
olad_libolaserver_la_LIBADD += $(libusb_LIBS)
endif

# PROGRAMS
##################################################
bin_PROGRAMS += olad/olad
olad_olad_SOURCES = olad/Olad.cpp

if SUPPORTS_RDYNAMIC
olad_olad_LDFLAGS = -rdynamic
endif
olad_olad_LDADD = olad/libolaserver.la \
                  common/libolacommon.la \
                  ola/libola.la

if USE_FTDI
olad_olad_LDADD += -lftdi -lusb
endif

# TESTS
##################################################
test_programs += \
    olad/ClientTester \
    olad/DeviceTester \
    olad/OlaTester \
    olad/PortTester \
    olad/UniverseTester

COMMON_OLAD_TEST_LDADD = $(COMMON_TESTING_LIBS) $(libprotobuf_LIBS) \
                         olad/libolaserver.la \
                         olad/libolaserverplugininterface.la \
                         common/libolacommon.la

olad_ClientTester_SOURCES = olad/ClientTest.cpp
olad_ClientTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_ClientTester_LDADD = $(COMMON_OLAD_TEST_LDADD)

olad_DeviceTester_SOURCES = olad/DeviceTest.cpp olad/DeviceManagerTest.cpp
olad_DeviceTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_DeviceTester_LDADD = $(COMMON_OLAD_TEST_LDADD)

olad_PortTester_SOURCES = olad/PortTest.cpp olad/PortManagerTest.cpp
olad_PortTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_PortTester_LDADD = $(COMMON_OLAD_TEST_LDADD)

olad_OlaTester_SOURCES = \
    olad/DmxSourceTest.cpp \
    olad/PluginManagerTest.cpp \
    olad/PreferencesTest.cpp \
    olad/OlaServerServiceImplTest.cpp
olad_OlaTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_OlaTester_LDADD = $(COMMON_OLAD_TEST_LDADD)

olad_UniverseTester_SOURCES = olad/UniverseTest.cpp
olad_UniverseTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
olad_UniverseTester_LDADD = $(COMMON_OLAD_TEST_LDADD)

CLEANFILES += olad/ola-output.conf
