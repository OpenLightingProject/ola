include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = liblogging.la
liblogging_la_SOURCES = Logging.cpp

if BUILD_TESTS
TESTS = LoggingTester
endif
check_PROGRAMS = $(TESTS)
LoggingTester_SOURCES = LoggingTester.cpp LoggingTest.cpp
LoggingTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
LoggingTester_LDADD = $(CPPUNIT_LIBS) \
                      ./liblogging.la \
                      ../base/libolabase.la \
                      ../utils/libolautils.la
