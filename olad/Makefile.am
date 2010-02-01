INCLUDES = -I$(top_builddir)/common -I$(top_builddir)/include \
           -I$(top_builddir)/libltdl @OLAD_DEFINES@
SUBDIRS = www testdata

OLASERVER_SOURCES = Client.cpp Device.cpp DeviceManager.cpp \
                    DlOpenPluginLoader.cpp DynamicPluginLoader.cpp \
                    OlaServerServiceImpl.cpp \
                    Plugin.cpp PluginAdaptor.cpp PluginManager.cpp \
                    Preferences.cpp Port.cpp PortPatcher.cpp Universe.cpp \
                    UniverseStore.cpp

# lib olaserver
lib_LTLIBRARIES = libolaserver.la
libolaserver_la_SOURCES = $(OLASERVER_SOURCES) OlaServer.cpp OlaDaemon.cpp
libolaserver_la_LDFLAGS = -lprotobuf

if HAVE_LIBMICROHTTPD
libolaserver_la_SOURCES += HttpServer.cpp OlaHttpServer.cpp
libolaserver_la_LDFLAGS += -lctemplate
endif

libolaserver_la_LIBADD = @LIBLTDL@ $(top_builddir)/common/libolacommon.la
libolaserver_la_CXXFLAGS = -Wall -Wformat -W

# Header files
pkgincludedir = $(includedir)/olad
pkginclude_HEADERS = OlaDaemon.h OlaServer.h

EXTRA_DIST = Client.h DeviceManager.h DlOpenPluginLoader.h \
             DynamicPluginLoader.h \
             HttpServer.h OlaHttpServer.h \
             OlaServerServiceImpl.h PluginLoader.h PluginManager.h \
             PortPatcher.h TestCommon.h UniverseStore.h

# Olad Server
bin_PROGRAMS = olad
olad_SOURCES = Olad.cpp
#olad_CXXFLAGS = -Wall -Werror  -Wformat -W
olad_CXXFLAGS = -Wall -Wformat -W
olad_LDFLAGS = -rdynamic -ldl -lprotobuf $(UUID_LIBS)
olad_LDADD = @LIBS@ libolaserver.la \
             ${PLUGIN_LIBS} \
             $(top_builddir)/common/libolacommon.la

# Test Programs
TESTS = OlaTester
check_PROGRAMS = $(TESTS)
OlaTester_SOURCES = $(OLASERVER_SOURCES) OlaServerTester.cpp \
                    UniverseTest.cpp DeviceTest.cpp DeviceManagerTest.cpp \
                    PluginManagerTest.cpp PreferencesTest.cpp \
                    PortManagerTest.cpp PortTest.cpp \
                    OlaServerServiceImplTest.cpp ClientTest.cpp
OlaTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
OlaTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl -lprotobuf
OlaTester_LDADD = @LIBS@ @LIBLTDL@ ${PLUGIN_LIBS} \
                  $(top_builddir)/common/libolacommon.la
