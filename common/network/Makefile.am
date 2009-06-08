
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

noinst_LTLIBRARIES = libllanetwork.la
libllanetwork_la_SOURCES = InterfacePicker.cpp SelectServer.cpp Socket.cpp

TESTS = SelectServerTester
check_PROGRAMS = $(TESTS)
SelectServerTester_SOURCES = SelectServerTester.cpp SocketTest.cpp \
                             SelectServerTest.cpp InterfacePickerTest.cpp
SelectServerTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
SelectServerTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl
SelectServerTester_LDADD = ./libllanetwork.la \
                           ../export_map/libllaexportmap.la \
                           ../logging/liblogging.la
