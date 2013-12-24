include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolafile.la
libolafile_la_SOURCES = Util.cpp

if BUILD_TESTS
TESTS = FileTester
endif
check_PROGRAMS = $(TESTS)

# required for all tests
COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ../libolacommon.la

FileTester_SOURCES = UtilTest.cpp
FileTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
FileTester_LDADD = $(COMMON_TEST_LDADD)

