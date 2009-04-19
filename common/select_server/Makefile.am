
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

noinst_LTLIBRARIES = libllaselectserver.la
libllaselectserver_la_SOURCES = SelectServer.cpp Socket.cpp

TESTS = SelectServerTester
check_PROGRAMS = $(TESTS)
SelectServerTester_SOURCES = SelectServerTester.cpp SocketTest.cpp
SelectServerTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
SelectServerTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl
SelectServerTester_LDADD = ./libllaselectserver.la \
                           ../export_map/libllaexportmap.la \
                           ../logging/liblogging.la
