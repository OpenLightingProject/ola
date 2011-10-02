include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolaexportmap.la
libolaexportmap_la_SOURCES = ExportMap.cpp

check_PROGRAMS = ExportMapTester
ExportMapTester_SOURCES = ExportMapTester.cpp ExportMapTest.cpp
ExportMapTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ExportMapTester_LDADD = $(CPPUNIT_LIBS) \
                        ./libolaexportmap.la \
                        ../utils/libolautils.la
