
INCLUDES =  -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

noinst_LTLIBRARIES = liblogging.la
liblogging_la_SOURCES = Logging.cpp

TESTS = LoggingTester
check_PROGRAMS = $(TESTS)

LoggingTester_SOURCES = LoggingTester.cpp LoggingTest.cpp
LoggingTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
LoggingTester_LDFLAGS = $(CPPUNIT_LIBS)
LoggingTester_LDADD = ./liblogging.la ../utils/libllautils.la
