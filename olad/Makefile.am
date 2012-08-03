include $(top_srcdir)/common.mk
SUBDIRS = www testdata

# Header files
pkgincludedir = $(includedir)/olad
pkginclude_HEADERS = OlaDaemon.h OlaServer.h


OLASERVER_SOURCES = Client.cpp ClientBroker.cpp Device.cpp DeviceManager.cpp \
                    DmxSource.cpp \
                    DynamicPluginLoader.cpp \
                    OlaServerServiceImpl.cpp \
                    Plugin.cpp PluginAdaptor.cpp PluginManager.cpp \
                    Preferences.cpp Port.cpp PortBroker.cpp PortManager.cpp \
                    Universe.cpp UniverseStore.cpp

# lib olaserver
lib_LTLIBRARIES = libolaserver.la
libolaserver_la_SOURCES = $(OLASERVER_SOURCES) OlaServer.cpp OlaDaemon.cpp
libolaserver_la_CXXFLAGS = $(COMMON_CXXFLAGS) @OLAD_DEFINES@
libolaserver_la_LIBADD = $(PLUGIN_LIBS) \
                         $(libmicrohttpd_LIBS) \
                         $(top_builddir)/common/libolacommon.la \
                         $(top_builddir)/common/web/libolaweb.la \
                         $(top_builddir)/ola/libola.la

if HAVE_LIBMICROHTTPD
libolaserver_la_SOURCES += HttpServer.cpp HttpServerActions.cpp \
                           OlaHttpServer.cpp HttpModule.cpp \
                           RDMHttpModule.cpp
endif


EXTRA_DIST = Client.h ClientBroker.h DeviceManager.h \
             DynamicPluginLoader.h HttpModule.h \
             HttpServer.h HttpServerActions.h \
             OlaHttpServer.h OlaVersion.h \
             OlaServerServiceImpl.h PluginLoader.h PluginManager.h \
             PortManager.h RDMHttpModule.h TestCommon.h \
             UniverseStore.h

# Olad Server
bin_PROGRAMS = olad
olad_SOURCES = Olad.cpp
if SUPPORTS_RDYNAMIC
olad_LDFLAGS = -rdynamic
endif
olad_LDADD = libolaserver.la \
             $(top_builddir)/common/libolacommon.la

# Test Programs
if BUILD_TESTS
TESTS = DeviceTester OlaTester PortTester UniverseTester
endif
check_PROGRAMS = $(TESTS)
common_test_sources = GenericTester.cpp
common_test_ldadd = $(CPPUNIT_LIBS) $(libprotobuf_LIBS) \
                    $(top_builddir)/olad/libolaserver.la \
                    $(top_builddir)/common/libolacommon.la

DeviceTester_SOURCES = DeviceTest.cpp DeviceManagerTest.cpp \
                       $(common_test_sources)
DeviceTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DeviceTester_LDADD = $(common_test_ldadd)

PortTester_SOURCES = PortTest.cpp PortManagerTest.cpp $(common_test_sources)
PortTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
PortTester_LDADD = $(common_test_ldadd)

OlaTester_SOURCES = DmxSourceTest.cpp PluginManagerTest.cpp \
                    PreferencesTest.cpp \
                    OlaServerServiceImplTest.cpp ClientTest.cpp \
                    $(common_test_sources)
OlaTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
OlaTester_LDADD = $(common_test_ldadd)

UniverseTester_SOURCES = UniverseTest.cpp $(common_test_sources)
UniverseTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
UniverseTester_LDADD = $(common_test_ldadd)
