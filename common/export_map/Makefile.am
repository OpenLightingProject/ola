include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolaexportmap.la
libolaexportmap_la_SOURCES = ExportMap.cpp


if BUILD_TESTS
TESTS = ExportMapTester
endif
check_PROGRAMS = $(TESTS)
ExportMapTester_SOURCES = ExportMapTester.cpp ExportMapTest.cpp
ExportMapTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ExportMapTester_LDADD = $(CPPUNIT_LIBS) \
                        ./libolaexportmap.la \
                        ../utils/libolautils.la
