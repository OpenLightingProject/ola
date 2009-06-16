INCLUDES = -I$(top_builddir)/common -I$(top_builddir)/include \
           -I$(top_builddir)/libltdl @LLAD_DEFINES@
SUBDIRS = www

LLASERVER_SOURCES = LlaServerServiceImpl.cpp \
                    Preferences.cpp Universe.cpp \
                    UniverseStore.cpp Client.cpp \
                    Device.cpp DeviceManager.cpp PluginAdaptor.cpp \
                    Plugin.cpp DlOpenPluginLoader.cpp

# lib llaserver
lib_LTLIBRARIES = libllaserver.la
libllaserver_la_SOURCES = $(LLASERVER_SOURCES) LlaServer.cpp LlaDaemon.cpp
libllaserver_la_LDFLAGS = -lprotobuf

if HAVE_LIBMICROHTTPD
libllaserver_la_SOURCES += HttpServer.cpp LlaHttpServer.cpp
libllaserver_la_LDFLAGS += -lctemplate
endif

libllaserver_la_LIBADD = @LIBLTDL@ $(top_builddir)/common/libllacommon.la
libllaserver_la_CXXFLAGS = -Wall -Wformat -W

# Header files
pkgincludedir = $(includedir)/llad
pkginclude_HEADERS = LlaDaemon.h LlaServer.h

EXTRA_DIST = Client.h DeviceManager.h DlOpenPluginLoader.h \
             DynamicPluginLoader.h \
             LlaServerServiceImpl.h PluginLoader.h UniverseStore.h \
             HttpServer.h LlaHttpServer.h

# Llad Server
bin_PROGRAMS = llad
llad_SOURCES = Llad.cpp
#llad_CXXFLAGS = -Wall -Werror  -Wformat -W
llad_CXXFLAGS = -Wall -Wformat -W
llad_LDFLAGS = -rdynamic -ldl -lprotobuf
llad_LDADD = @LIBS@ libllaserver.la \
             $(top_builddir)/common/libllacommon.la

# Test Programs
TESTS = LlaTester
check_PROGRAMS = $(TESTS)
LlaTester_SOURCES = $(LLASERVER_SOURCES) LlaServerTester.cpp \
                    UniverseTest.cpp DeviceTest.cpp PreferencesTest.cpp \
                    LlaServerServiceImplTest.cpp ClientTest.cpp
LlaTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
LlaTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl -lprotobuf
LlaTester_LDADD = @LIBS@ @LIBLTDL@ $(top_builddir)/common/libllacommon.la


#-------------------------------------

# crap starts here
#llad_test_SOURCES = main_test.cpp devicemanager.cpp pluginadaptor.cpp\
#				llad.cpp network.cpp plugin.cpp device.cpp port.cpp\
#				DynamicPluginLoader.cpp universe.cpp \
#				client.cpp preferences.cpp UniverseStore.cpp

#llad_test_CXXFLAGS = -Wall -Werror  -Wformat -W
#llad_test_LDADD = @LIBS@ ${PLUGIN_LIBS} -lprotobuf ../protocol/lla.pb.o
#llad_test_CXXFLAGS = -g
#-llladummy -lllaespnet \
#					-lllaopendmx -lllapathport -lllasandnet -lllashownet -lllastageprofi \
#					-lllausbpro
#llad_test_LDADD = @LIBS@
#llad_test_LDFLAGS = -rdynamic
