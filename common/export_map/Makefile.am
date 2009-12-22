
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

noinst_LTLIBRARIES = libolaexportmap.la
libolaexportmap_la_SOURCES = ExportMap.cpp

TESTS = ExportMapTester
check_PROGRAMS = $(TESTS)
ExportMapTester_SOURCES = ExportMapTester.cpp ExportMapTest.cpp
ExportMapTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
ExportMapTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl
ExportMapTester_LDADD = ./libolaexportmap.la ../utils/libolautils.la
