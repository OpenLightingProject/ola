# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/timecode/TimeCode.cpp

# TESTS
##################################################
test_programs += common/timecode/TimeCodeTester

common_timecode_TimeCodeTester_SOURCES = common/timecode/TimeCodeTest.cpp
common_timecode_TimeCodeTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_timecode_TimeCodeTester_LDADD = $(COMMON_TESTING_LIBS)

