include $(top_srcdir)/common.mk

dist_noinst_DATA = testdata/arrays.test \
                   testdata/objects.test \
                   testdata/schema.json

# LIBRARIES
################################################
noinst_LTLIBRARIES = libolaweb.la
libolaweb_la_SOURCES = Json.cpp JsonParser.cpp JsonSchema.cpp \
                       JsonSections.cpp PointerTracker.h PointerTracker.cpp \
                       SchemaParser.h SchemaParser.cpp SchemaParseContext.h \
                       SchemaParseContext.cpp TreeHandler.cpp

# TESTS
################################################
if BUILD_TESTS
TESTS = JsonParserTester JsonSchemaTester JsonSchemaParserTester \
        PointerTrackerTester WebTester
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

JsonSchemaTester_SOURCES = JsonSchemaTest.cpp
JsonSchemaTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
JsonSchemaTester_LDADD = $(COMMON_TESTING_LIBS) \
                         ./libolaweb.la \
                         ../base/libolabase.la \
                         ../utils/libolautils.la

JsonSchemaParserTester_SOURCES = JsonSchemaParserTest.cpp
JsonSchemaParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
JsonSchemaParserTester_LDADD = $(COMMON_TESTING_LIBS) \
                               ./libolaweb.la \
                               ../base/libolabase.la \
                               ../file/libolafile.la \
                               ../utils/libolautils.la

PointerTrackerTester_SOURCES = PointerTrackerTest.cpp
PointerTrackerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PointerTrackerTester_LDADD = $(COMMON_TESTING_LIBS) \
                             ./libolaweb.la \
                             ../base/libolabase.la \
                             ../utils/libolautils.la
