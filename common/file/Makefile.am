include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
noinst_LTLIBRARIES = libolafile.la
libolafile_la_SOURCES = Util.cpp

# TESTS
##################################################

if BUILD_TESTS
TESTS = UtilTester
endif
check_PROGRAMS = $(TESTS)

# required for all tests
COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ../libolacommon.la

UtilTester_SOURCES = UtilTest.cpp
UtilTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
UtilTester_LDADD = $(COMMON_TEST_LDADD)

