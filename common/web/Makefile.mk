dist_noinst_DATA += \
    common/web/testdata/allof.test \
    common/web/testdata/anyof.test \
    common/web/testdata/arrays.test \
    common/web/testdata/basic-keywords.test \
    common/web/testdata/definitions.test \
    common/web/testdata/integers.test \
    common/web/testdata/misc.test \
    common/web/testdata/not.test \
    common/web/testdata/objects.test \
    common/web/testdata/oneof.test \
    common/web/testdata/schema.json \
    common/web/testdata/strings.test \
    common/web/testdata/type.test

# LIBRARIES
################################################
noinst_LTLIBRARIES += common/web/libolaweb.la
common_web_libolaweb_la_SOURCES = \
    common/web/Json.cpp \
    common/web/JsonData.cpp \
    common/web/JsonLexer.cpp \
    common/web/JsonParser.cpp \
    common/web/JsonPatch.cpp \
    common/web/JsonPatchParser.cpp \
    common/web/JsonPointer.cpp \
    common/web/JsonSchema.cpp \
    common/web/JsonSections.cpp \
    common/web/JsonTypes.cpp \
    common/web/JsonWriter.cpp \
    common/web/OptionalItem.h \
    common/web/PointerTracker.cpp \
    common/web/PointerTracker.h \
    common/web/SchemaErrorLogger.cpp \
    common/web/SchemaErrorLogger.h \
    common/web/SchemaKeywords.cpp \
    common/web/SchemaKeywords.h \
    common/web/SchemaParseContext.cpp \
    common/web/SchemaParseContext.h \
    common/web/SchemaParser.cpp \
    common/web/SchemaParser.h

if USING_WIN32
#Work around limitations with Windows library linking
common_web_libolaweb_la_LIBADD = common/libolacommon.la
endif

# TESTS
################################################
# Patch test names are abbreviated to prevent Windows' UAC from blocking them.
test_programs += \
    common/web/JsonTester \
    common/web/ParserTester \
    common/web/PtchParserTester \
    common/web/PtchTester \
    common/web/PointerTester \
    common/web/PointerTrackerTester \
    common/web/SchemaParserTester \
    common/web/SchemaTester \
    common/web/SectionsTester

COMMON_WEB_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                        common/web/libolaweb.la

common_web_JsonTester_SOURCES = common/web/JsonTest.cpp
common_web_JsonTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_web_JsonTester_LDADD = $(COMMON_WEB_TEST_LDADD)

common_web_ParserTester_SOURCES = common/web/ParserTest.cpp
common_web_ParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_web_ParserTester_LDADD = $(COMMON_WEB_TEST_LDADD)

common_web_PtchParserTester_SOURCES = common/web/PatchParserTest.cpp
common_web_PtchParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_web_PtchParserTester_LDADD = $(COMMON_WEB_TEST_LDADD)

common_web_PtchTester_SOURCES = common/web/PatchTest.cpp
common_web_PtchTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_web_PtchTester_LDADD = $(COMMON_WEB_TEST_LDADD)

common_web_PointerTester_SOURCES = common/web/PointerTest.cpp
common_web_PointerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_web_PointerTester_LDADD = $(COMMON_WEB_TEST_LDADD)

common_web_PointerTrackerTester_SOURCES = common/web/PointerTrackerTest.cpp
common_web_PointerTrackerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_web_PointerTrackerTester_LDADD = $(COMMON_WEB_TEST_LDADD)

common_web_SchemaParserTester_SOURCES = common/web/SchemaParserTest.cpp
common_web_SchemaParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_web_SchemaParserTester_LDADD = $(COMMON_WEB_TEST_LDADD)

common_web_SchemaTester_SOURCES = common/web/SchemaTest.cpp
common_web_SchemaTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_web_SchemaTester_LDADD = $(COMMON_WEB_TEST_LDADD)

common_web_SectionsTester_SOURCES = common/web/SectionsTest.cpp
common_web_SectionsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_web_SectionsTester_LDADD = $(COMMON_WEB_TEST_LDADD)
