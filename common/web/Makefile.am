include $(top_srcdir)/common.mk

# LIBRARIES
################################################
noinst_LTLIBRARIES = libolaweb.la
libolaweb_la_SOURCES = Json.cpp JsonParser.cpp JsonSections.cpp TreeHandler.cpp

# TESTS
################################################
if BUILD_TESTS
TESTS = JsonParserTester WebTester
endif
check_PROGRAMS = $(TESTS)

WebTester_SOURCES = JsonTest.cpp JsonSectionsTest.cpp
WebTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
WebTester_LDADD = $(COMMON_TESTING_LIBS) \
                  ./libolaweb.la \
                  ../base/libolabase.la \
                  ../utils/libolautils.la

JsonParserTester_SOURCES = JsonParserTest.cpp
JsonParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
JsonParserTester_LDADD = $(COMMON_TESTING_LIBS) \
                         ./libolaweb.la \
                         ../base/libolabase.la \
                         ../utils/libolautils.la
