include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolaweb.la
libolaweb_la_SOURCES = JsonSections.cpp

check_PROGRAMS = WebTester
WebTester_SOURCES = JsonSectionsTest.cpp WebTester.cpp
WebTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
WebTester_LDADD = $(CPPUNIT_LIBS) \
                  libolaweb.la \
                  ../logging/liblogging.la \
                  ../utils/libolautils.la
