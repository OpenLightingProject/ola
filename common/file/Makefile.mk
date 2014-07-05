# LIBRARIES
##################################################
libolacommon_la_SOURCES += common/file/Util.cpp

# TESTS
##################################################
tests += common/file/UtilTester

common_file_UtilTester_SOURCES = common/file/UtilTest.cpp
common_file_UtilTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_file_UtilTester_LDADD = $(COMMON_TEST_LDADD)
