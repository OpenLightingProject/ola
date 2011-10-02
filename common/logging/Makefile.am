include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = liblogging.la
liblogging_la_SOURCES = Logging.cpp

check_PROGRAMS = LoggingTester
LoggingTester_SOURCES = LoggingTester.cpp LoggingTest.cpp
LoggingTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
LoggingTester_LDADD = $(CPPUNIT_LIBS) \
                      ./liblogging.la \
                      ../utils/libolautils.la
