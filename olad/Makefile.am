INCLUDES = -I$(top_builddir)/common -I$(top_builddir)/include \
           -I$(top_builddir)/libltdl @OLAD_DEFINES@
SUBDIRS = www

OLASERVER_SOURCES = OlaServerServiceImpl.cpp \
                    Preferences.cpp Universe.cpp \
                    UniverseStore.cpp Client.cpp \
                    Device.cpp DeviceManager.cpp PluginAdaptor.cpp \
                    Plugin.cpp DlOpenPluginLoader.cpp

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
             OlaServerServiceImpl.h PluginLoader.h UniverseStore.h \
             HttpServer.h OlaHttpServer.h

# Olad Server
bin_PROGRAMS = olad
olad_SOURCES = Olad.cpp
#olad_CXXFLAGS = -Wall -Werror  -Wformat -W
olad_CXXFLAGS = -Wall -Wformat -W
olad_LDFLAGS = -rdynamic -ldl -lprotobuf
olad_LDADD = @LIBS@ libolaserver.la \
             $(top_builddir)/common/libolacommon.la

# Test Programs
TESTS = OlaTester
check_PROGRAMS = $(TESTS)
OlaTester_SOURCES = $(OLASERVER_SOURCES) OlaServerTester.cpp \
                    UniverseTest.cpp DeviceTest.cpp PreferencesTest.cpp \
                    OlaServerServiceImplTest.cpp ClientTest.cpp
OlaTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
OlaTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl -lprotobuf
OlaTester_LDADD = @LIBS@ @LIBLTDL@ $(top_builddir)/common/libolacommon.la


#-------------------------------------

# crap starts here
#olad_test_SOURCES = main_test.cpp devicemanager.cpp pluginadaptor.cpp\
#				olad.cpp network.cpp plugin.cpp device.cpp port.cpp\
#				DynamicPluginLoader.cpp universe.cpp \
#				client.cpp preferences.cpp UniverseStore.cpp

#olad_test_CXXFLAGS = -Wall -Werror  -Wformat -W
#olad_test_LDADD = @LIBS@ ${PLUGIN_LIBS} -lprotobuf ../protocol/ola.pb.o
#olad_test_CXXFLAGS = -g
#-loladummy -lolaespnet \
#					-lolaopendmx -lolapathport -lolasandnet -lolashownet -lolastageprofi \
#					-lolausbpro
#olad_test_LDADD = @LIBS@
#olad_test_LDFLAGS = -rdynamic
