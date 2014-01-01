include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolamath.la
libolamath_la_SOURCES = Random.cpp

if BUILD_TESTS
TESTS = MathUtilsTester
endif
check_PROGRAMS = $(TESTS)

# required for all tests
COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ../libolacommon.la

MathUtilsTester_SOURCES = MathUtilsTest.cpp
MathUtilsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
MathUtilsTester_LDADD = $(COMMON_TEST_LDADD)