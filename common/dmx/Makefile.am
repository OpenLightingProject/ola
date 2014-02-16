include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
noinst_LTLIBRARIES = liboladmx.la
liboladmx_la_SOURCES = RunLengthEncoder.cpp

# TESTS
##################################################

if BUILD_TESTS
TESTS = RunLengthEncoderTester
endif
check_PROGRAMS = $(TESTS)

RunLengthEncoderTester_SOURCES = RunLengthEncoderTest.cpp
RunLengthEncoderTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RunLengthEncoderTester_LDADD = $(COMMON_TESTING_LIBS) \
                               ../libolacommon.la
