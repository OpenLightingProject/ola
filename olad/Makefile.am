include $(top_srcdir)/common.mk
SUBDIRS = www

dist_noinst_DATA = testdata/test_preferences.conf

# HEADERS
##################################################
pkgincludedir = $(includedir)/olad
pkginclude_HEADERS = OlaDaemon.h OlaServer.h

noinst_HEADERS = TestCommon.h

# LIBRARIES
##################################################
ola_server_sources = Client.h Client.cpp ClientBroker.h ClientBroker.cpp \
                     Device.cpp DeviceManager.h DeviceManager.cpp \
                     DiscoveryAgent.h DynamicPluginLoader.h DmxSource.cpp \
                     DiscoveryAgent.cpp DynamicPluginLoader.cpp \
                     HttpServerActions.h OladHTTPServer.h \
                     OlaServerServiceImpl.h OlaServerServiceImpl.cpp \
                     Plugin.cpp PluginAdaptor.cpp PluginLoader.h \
                     PluginManager.h PluginManager.cpp \
                     Preferences.cpp Port.cpp PortBroker.cpp PortManager.h \
                     PortManager.cpp RDMHTTPModule.h Universe.cpp \
                     UniverseStore.h UniverseStore.cpp
ola_server_additional_libs =

if HAVE_DNSSD
ola_server_sources += BonjourDiscoveryAgent.h BonjourDiscoveryAgent.cpp
endif

if HAVE_AVAHI
ola_server_sources += AvahiDiscoveryAgent.h AvahiDiscoveryAgent.cpp
ola_server_additional_libs += $(avahi_LIBS)
endif

if HAVE_LIBMICROHTTPD
ola_server_sources += HttpServerActions.cpp \
                     OladHTTPServer.cpp \
                     RDMHTTPModule.cpp
ola_server_additional_libs += ../common/http/libolahttp.la
endif

# lib olaserver
lib_LTLIBRARIES = libolaserver.la
libolaserver_la_SOURCES = $(ola_server_sources) OlaServer.cpp OlaDaemon.cpp
libolaserver_la_CXXFLAGS = $(COMMON_CXXFLAGS) @OLAD_DEFINES@
libolaserver_la_LIBADD = $(PLUGIN_LIBS) \
                         ../common/libolacommon.la \
                         ../common/web/libolaweb.la \
                         ../ola/libola.la \
                         $(ola_server_additional_libs)

# PROGRAMS
##################################################
bin_PROGRAMS = olad
olad_SOURCES = Olad.cpp
olad_CXXFLAGS = $(COMMON_CXXFLAGS) @OLAD_DEFINES@

if SUPPORTS_RDYNAMIC
olad_LDFLAGS = -rdynamic
endif
olad_LDADD = libolaserver.la \
             ../common/libolacommon.la \
             ../ola/libola.la

if HAVE_LIBFTDI
olad_LDADD += -lftdi -lusb
endif

# TESTS
##################################################
if BUILD_TESTS
TESTS = DeviceTester OlaTester PortTester UniverseTester
endif
check_PROGRAMS = $(TESTS)
COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) $(libprotobuf_LIBS) \
                    ../olad/libolaserver.la \
                    ../common/libolacommon.la

DeviceTester_SOURCES = DeviceTest.cpp DeviceManagerTest.cpp
DeviceTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DeviceTester_LDADD = $(COMMON_TEST_LDADD)

PortTester_SOURCES = PortTest.cpp PortManagerTest.cpp
PortTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PortTester_LDADD = $(COMMON_TEST_LDADD)

OlaTester_SOURCES = DmxSourceTest.cpp PluginManagerTest.cpp \
                    PreferencesTest.cpp \
                    OlaServerServiceImplTest.cpp ClientTest.cpp
OlaTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
OlaTester_LDADD = $(COMMON_TEST_LDADD)

UniverseTester_SOURCES = UniverseTest.cpp
UniverseTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
UniverseTester_LDADD = $(COMMON_TEST_LDADD)

CLEANFILES = testdata/ola-output.conf
