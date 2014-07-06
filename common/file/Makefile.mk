# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += common/file/Util.cpp

# TESTS
##################################################
test_programs += common/file/UtilTester

common_file_UtilTester_SOURCES = common/file/UtilTest.cpp
common_file_UtilTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_file_UtilTester_LDADD = $(COMMON_TESTING_LIBS)
