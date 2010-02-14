include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolanetwork.la
libolanetwork_la_SOURCES = InterfacePicker.cpp NetworkUtils.cpp \
                           SelectServer.cpp Socket.cpp

TESTS = NetworkTester
check_PROGRAMS = $(TESTS)
NetworkTester_SOURCES = SelectServerTester.cpp SocketTest.cpp \
                        NetworkTest.cpp InterfacePickerTest.cpp
NetworkTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
NetworkTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl
NetworkTester_LDADD = ./libolanetwork.la \
                      ../export_map/libolaexportmap.la \
                      ../logging/liblogging.la \
                      ../utils/libolautils.la
