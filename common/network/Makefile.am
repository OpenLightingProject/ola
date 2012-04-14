include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolanetwork.la
libolanetwork_la_SOURCES = ExponentialTCPConnector.cpp \
                           HealthCheckedConnection.cpp \
                           IPV4Address.cpp \
                           Interface.cpp \
                           InterfacePicker.cpp \
                           NetworkUtils.cpp \
                           SelectServer.cpp \
                           Socket.cpp \
                           TCPConnector.cpp

if USING_WIN32
libolanetwork_la_SOURCES += WindowsInterfacePicker.cpp
else
libolanetwork_la_SOURCES += PosixInterfacePicker.cpp
endif

EXTRA_DIST = PosixInterfacePicker.h WindowsInterfacePicker.h

TESTS = HealthCheckedConnectionTester NetworkTester TCPConnectorTester
check_PROGRAMS = $(TESTS)

HealthCheckedConnectionTester_SOURCES = HealthCheckedConnectionTest.cpp \
                                        HealthCheckedConnectionTester.cpp
HealthCheckedConnectionTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
HealthCheckedConnectionTester_LDADD = $(CPPUNIT_LIBS) \
                                      ./libolanetwork.la \
                                      ../export_map/libolaexportmap.la \
                                      ../logging/liblogging.la \
                                      ../thread/libthread.la \
                                      ../utils/libolautils.la

NetworkTester_SOURCES = IPAddressTest.cpp \
                        InterfacePickerTest.cpp \
                        InterfaceTest.cpp \
                        NetworkUtilsTest.cpp \
                        SelectServerTest.cpp \
                        SelectServerTester.cpp \
                        SelectServerThreadTest.cpp \
                        SocketTest.cpp
NetworkTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
NetworkTester_LDADD = $(CPPUNIT_LIBS) \
                      ./libolanetwork.la \
                      ../export_map/libolaexportmap.la \
                      ../logging/liblogging.la \
                      ../thread/libthread.la \
                      ../utils/libolautils.la

if USING_WIN32
  NetworkTester_LDFLAGS = -no-undefined -liphlpapi -lnetapi32 -lcap -lws2_32 -ldpnet -lwsock32
endif

TCPConnectorTester_SOURCES = ExponentialTCPConnectorTest.cpp \
                             TCPConnectorTest.cpp \
                             TCPConnectorTester.cpp
TCPConnectorTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
TCPConnectorTester_LDADD = $(CPPUNIT_LIBS) \
                           ./libolanetwork.la \
                           ../export_map/libolaexportmap.la \
                           ../logging/liblogging.la \
                           ../thread/libthread.la \
                           ../utils/libolautils.la
