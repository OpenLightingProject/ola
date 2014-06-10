include $(top_srcdir)/common.mk

dist_noinst_DATA = testdata/allof.test \
                   testdata/anyof.test \
                   testdata/arrays.test \
                   testdata/basic-keywords.test \
                   testdata/definitions.test \
                   testdata/integers.test \
                   testdata/misc.test \
                   testdata/not.test \
                   testdata/objects.test \
                   testdata/oneof.test \
                   testdata/schema.json \
                   testdata/strings.test \
                   testdata/type.test

# LIBRARIES
################################################
noinst_LTLIBRARIES = libolaweb.la
libolaweb_la_SOURCES = Json.cpp JsonData.cpp JsonLexer.cpp JsonParser.cpp \
                       JsonPatch.cpp JsonPatchParser.cpp JsonPointer.cpp \
                       JsonSchema.cpp JsonSections.cpp JsonTypes.cpp \
                       JsonWriter.cpp OptionalItem.h PointerTracker.cpp \
                       PointerTracker.h SchemaErrorLogger.cpp \
                       SchemaErrorLogger.h SchemaKeywords.cpp \
                       SchemaKeywords.h SchemaParseContext.cpp \
                       SchemaParseContext.h SchemaParser.cpp SchemaParser.h

# TESTS
################################################
# Patch test names are abbreviated to prevent Windows' UAC from blocking them.
if BUILD_TESTS
TESTS = JsonTester ParserTester PatParserTester PatTester PointerTester \
        PointerTrackerTester SchemaParserTester SchemaTester SectionsTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ./libolaweb.la \
                    ../base/libolabase.la \
                    ../utils/libolautils.la

JsonTester_SOURCES = JsonTest.cpp
JsonTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
JsonTester_LDADD = $(COMMON_TEST_LDADD)

ParserTester_SOURCES = ParserTest.cpp
ParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ParserTester_LDADD = $(COMMON_TEST_LDADD)

PatParserTester_SOURCES = PatchParserTest.cpp
PatParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PatParserTester_LDADD = $(COMMON_TEST_LDADD)

PatTester_SOURCES = PatchTest.cpp
PatTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PatTester_LDADD = $(COMMON_TEST_LDADD)

PointerTester_SOURCES = PointerTest.cpp
PointerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PointerTester_LDADD = $(COMMON_TEST_LDADD)

PointerTrackerTester_SOURCES = PointerTrackerTest.cpp
PointerTrackerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PointerTrackerTester_LDADD = $(COMMON_TEST_LDADD)

SchemaParserTester_SOURCES = SchemaParserTest.cpp
SchemaParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SchemaParserTester_LDADD = $(COMMON_TEST_LDADD) \
                           ../file/libolafile.la

SchemaTester_SOURCES = SchemaTest.cpp
SchemaTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SchemaTester_LDADD = $(COMMON_TEST_LDADD)

SectionsTester_SOURCES = SectionsTest.cpp
SectionsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SectionsTester_LDADD = $(COMMON_TEST_LDADD)
