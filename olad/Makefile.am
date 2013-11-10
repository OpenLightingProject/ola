include $(top_srcdir)/common.mk
SUBDIRS = www testdata

# Header files
pkgincludedir = $(includedir)/olad
pkginclude_HEADERS = OlaDaemon.h OlaServer.h


OLASERVER_SOURCES = AvahiDiscoveryAgent.h Client.cpp ClientBroker.cpp \
                    Device.cpp DeviceManager.cpp DmxSource.cpp \
                    DiscoveryAgent.cpp \
                    DynamicPluginLoader.cpp \
                    OlaServerServiceImpl.cpp \
                    Plugin.cpp PluginAdaptor.cpp PluginManager.cpp \
                    Preferences.cpp Port.cpp PortBroker.cpp PortManager.cpp \
                    Universe.cpp UniverseStore.cpp

if HAVE_DNSSD
OLASERVER_SOURCES += BonjourDiscoveryAgent.cpp
endif

if HAVE_AVAHI
OLASERVER_SOURCES += AvahiDiscoveryAgent.cpp
endif

# lib olaserver
lib_LTLIBRARIES = libolaserver.la
libolaserver_la_SOURCES = $(OLASERVER_SOURCES) OlaServer.cpp OlaDaemon.cpp
libolaserver_la_CXXFLAGS = $(COMMON_CXXFLAGS) @OLAD_DEFINES@
libolaserver_la_LIBADD = $(PLUGIN_LIBS) \
                         $(top_builddir)/common/libolacommon.la \
                         $(top_builddir)/common/web/libolaweb.la \
                         $(top_builddir)/ola/libola.la

if HAVE_LIBMICROHTTPD
libolaserver_la_SOURCES += HttpServerActions.cpp \
                           OladHTTPServer.cpp \
                           RDMHTTPModule.cpp
libolaserver_la_LIBADD += $(top_builddir)/common/http/libolahttp.la
endif


EXTRA_DIST = BonjourDiscoveryAgent.h Client.h ClientBroker.h DeviceManager.h \
             DiscoveryAgent.h DynamicPluginLoader.h \
             HttpServerActions.h \
             OladHTTPServer.h OlaVersion.h \
             OlaServerServiceImpl.h PluginLoader.h PluginManager.h \
             PortManager.h RDMHTTPModule.h TestCommon.h \
             UniverseStore.h

# Olad Server
bin_PROGRAMS = olad
olad_SOURCES = Olad.cpp
olad_CXXFLAGS = $(COMMON_CXXFLAGS) @OLAD_DEFINES@

if SUPPORTS_RDYNAMIC
olad_LDFLAGS = -rdynamic
endif
olad_LDADD = libolaserver.la \
             $(top_builddir)/common/libolacommon.la

if HAVE_LIBFTDI
olad_LDADD += -lftdi -lusb
endif

# Test Programs
if BUILD_TESTS
TESTS = DeviceTester OlaTester PortTester UniverseTester
endif
check_PROGRAMS = $(TESTS)
COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) $(libprotobuf_LIBS) \
                    $(top_builddir)/olad/libolaserver.la \
                    $(top_builddir)/common/libolacommon.la

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
