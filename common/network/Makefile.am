include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolanetwork.la
libolanetwork_la_SOURCES = IPV4Address.cpp InterfacePicker.cpp \
                           NetworkUtils.cpp SelectServer.cpp Socket.cpp

if USING_WIN32
libolanetwork_la_SOURCES += WindowsInterfacePicker.cpp
else
libolanetwork_la_SOURCES += PosixInterfacePicker.cpp
endif

EXTRA_DIST = PosixInterfacePicker.h WindowsInterfacePicker.h

TESTS = NetworkTester
check_PROGRAMS = $(TESTS)
NetworkTester_SOURCES = IPAddressTest.cpp InterfacePickerTest.cpp \
                        SelectServerTester.cpp SocketTest.cpp \
                        SelectServerTest.cpp NetworkUtilsTest.cpp
NetworkTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
NetworkTester_LDFLAGS = $(CPPUNIT_LIBS)
NetworkTester_LDADD = ./libolanetwork.la \
                      ../export_map/libolaexportmap.la \
                      ../logging/liblogging.la \
                      ../utils/libolautils.la

if USING_WIN32
  NetworkTester_LDFLAGS += -no-undefined -liphlpapi -lnetapi32 -lcap -lws2_32 -ldpnet -lwsock32
endif
