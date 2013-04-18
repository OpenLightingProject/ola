include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libtimecode.la
libtimecode_la_SOURCES = TimeCode.cpp

if BUILD_TESTS
TESTS = TimeCodeTester
endif
check_PROGRAMS = $(TESTS)
TimeCodeTester_SOURCES = TimeCodeTester.cpp \
                         TimeCodeTest.cpp
TimeCodeTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
TimeCodeTester_LDADD = $(CPPUNIT_LIBS) \
                       ./libtimecode.la \
                       ../base/libolabase.la \
                       ../logging/liblogging.la
