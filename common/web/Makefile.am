include $(top_srcdir)/common.mk

# LIBRARIES
################################################
noinst_LTLIBRARIES = libolaweb.la
libolaweb_la_SOURCES = Json.cpp JsonParser.cpp JsonPointer.cpp \
                       JsonSections.cpp JsonWriter.cpp PointerTracker.cpp \
                       PointerTracker.h TreeHandler.cpp

# TESTS
################################################
if BUILD_TESTS
TESTS = JsonParserTester JsonPointerTester PointerTrackerTester WebTester
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

JsonPointerTester_SOURCES = JsonPointerTest.cpp
JsonPointerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
JsonPointerTester_LDADD = $(COMMON_TESTING_LIBS) \
                         ./libolaweb.la \
                         ../base/libolabase.la \
                         ../utils/libolautils.la

PointerTrackerTester_SOURCES = PointerTrackerTest.cpp
PointerTrackerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PointerTrackerTester_LDADD = $(COMMON_TESTING_LIBS) \
                             ./libolaweb.la \
                             ../base/libolabase.la \
                             ../utils/libolautils.la
