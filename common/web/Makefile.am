include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolaweb.la
libolaweb_la_SOURCES = Json.cpp JsonSections.cpp

if BUILD_TESTS
TESTS = WebTester
endif
check_PROGRAMS = $(TESTS)
WebTester_SOURCES = JsonTest.cpp JsonSectionsTest.cpp WebTester.cpp
WebTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
WebTester_LDADD = $(CPPUNIT_LIBS) \
                  libolaweb.la \
                  ../base/libolabase.la \
                  ../utils/libolautils.la
