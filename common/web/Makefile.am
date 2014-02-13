include $(top_srcdir)/common.mk

# LIBRARIES
################################################
noinst_LTLIBRARIES = libolaweb.la
libolaweb_la_SOURCES = Json.cpp JsonSections.cpp

# TESTS
################################################

if BUILD_TESTS
TESTS = WebTester
endif
check_PROGRAMS = $(TESTS)

WebTester_SOURCES = JsonTest.cpp JsonSectionsTest.cpp
WebTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
WebTester_LDADD = $(COMMON_TESTING_LIBS) \
                  libolaweb.la \
                  $(top_builddir)/common/base/libolabase.la \
                  $(top_builddir)/common/utils/libolautils.la
