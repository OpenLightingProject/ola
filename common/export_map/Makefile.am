include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolaexportmap.la
libolaexportmap_la_SOURCES = ExportMap.cpp


if BUILD_TESTS
TESTS = ExportMapTester
endif
check_PROGRAMS = $(TESTS)
ExportMapTester_SOURCES = ExportMapTest.cpp
ExportMapTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ExportMapTester_LDADD = $(COMMON_TESTING_LIBS) \
                        ./libolaexportmap.la \
                        ../testing/libtestmain.la \
                        ../utils/libolautils.la

