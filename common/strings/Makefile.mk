# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/strings/Format.cpp \
    common/strings/Utils.cpp

# TESTS
################################################
test_programs += common/strings/UtilsTester

common_strings_UtilsTester_SOURCES = \
    common/strings/UtilsTest.cpp
common_strings_UtilsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_strings_UtilsTester_LDADD = $(COMMON_TESTING_LIBS)
