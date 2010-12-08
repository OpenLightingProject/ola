include $(top_srcdir)/common.mk
SUBDIRS = www testdata


OLASERVER_SOURCES = Client.cpp ClientBroker.cpp Device.cpp DeviceManager.cpp \
                    DmxSource.cpp DynamicPluginLoader.cpp \
                    OlaServerServiceImpl.cpp \
                    Plugin.cpp PluginAdaptor.cpp PluginManager.cpp \
                    Preferences.cpp Port.cpp PortBroker.cpp PortManager.cpp \
                    Universe.cpp UniverseStore.cpp

# lib olaserver
lib_LTLIBRARIES = libolaserver.la
libolaserver_la_SOURCES = $(OLASERVER_SOURCES) OlaServer.cpp OlaDaemon.cpp
libolaserver_la_LDFLAGS = -lprotobuf ${PLUGIN_LIBS} ${MICROHTTPD_LIBS}

if HAVE_LIBMICROHTTPD
libolaserver_la_SOURCES += HttpServer.cpp HttpServerActions.cpp \
                           OlaHttpServer.cpp HttpModule.cpp \
                           RDMHttpModule.cpp
endif

libolaserver_la_LIBADD = $(top_builddir)/common/libolacommon.la \
                         $(top_builddir)/common/web/libolaweb.la \
                         $(top_builddir)/ola/libola.la
libolaserver_la_CXXFLAGS = -Wall -Wformat -W @OLAD_DEFINES@ $(libusb_CFLAGS)

# Header files
pkgincludedir = $(includedir)/olad
pkginclude_HEADERS = OlaDaemon.h OlaServer.h

EXTRA_DIST = Client.h ClientBroker.h DeviceManager.h DlOpenPluginLoader.h \
             DynamicPluginLoader.h HttpModule.h \
             HttpServer.h HttpServerActions.h \
             OlaHttpServer.h OlaVersion.h \
             OlaServerServiceImpl.h PluginLoader.h PluginManager.h \
             PortManager.h RDMHttpModule.h TestCommon.h \
             UniverseStore.h

# Olad Server
bin_PROGRAMS = olad
olad_SOURCES = Olad.cpp
#olad_CXXFLAGS = -Wall -Werror  -Wformat -W
olad_CXXFLAGS = -Wall -Wformat -W
olad_LDFLAGS = -rdynamic -ldl -lprotobuf $(UUID_LIBS)
olad_LDADD = @LIBS@ libolaserver.la \
             $(top_builddir)/common/libolacommon.la

# Test Programs
TESTS = OlaTester
check_PROGRAMS = $(TESTS)
OlaTester_SOURCES = OlaServerTester.cpp \
                    UniverseTest.cpp DeviceTest.cpp DeviceManagerTest.cpp \
                    DmxSourceTest.cpp PluginManagerTest.cpp \
                    PreferencesTest.cpp PortManagerTest.cpp PortTest.cpp \
                    OlaServerServiceImplTest.cpp ClientTest.cpp
OlaTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
OlaTester_LDFLAGS = $(CPPUNIT_LIBS)
OlaTester_LDADD = @LIBS@ $(top_builddir)/olad/libolaserver.la
