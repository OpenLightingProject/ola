include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolaweb.la
libolaweb_la_SOURCES = JsonSections.cpp

TESTS = WebTester
check_PROGRAMS = $(TESTS)
WebTester_SOURCES = JsonSectionsTest.cpp WebTester.cpp
WebTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
WebTester_LDFLAGS = $(CPPUNIT_LIBS)
WebTester_LDADD = libolaweb.la ../logging/liblogging.la ../utils/libolautils.la
