# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/base/Credentials.cpp \
    common/base/Env.cpp \
    common/base/Flags.cpp \
    common/base/Init.cpp \
    common/base/Logging.cpp \
    common/base/SysExits.cpp \
    common/base/Version.cpp

if HAVE_STRERROR_R
common_libolacommon_la_SOURCES += common/base/StrError_R.cpp \
                                  common/base/StrError_R_XSI.cpp
endif

# TESTS
##################################################

test_programs += common/base/CredentialsTester \
                 common/base/FlagsTester \
                 common/base/LoggingTester

common_base_CredentialsTester_SOURCES = common/base/CredentialsTest.cpp
common_base_CredentialsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_base_CredentialsTester_LDADD = $(COMMON_TESTING_LIBS)

common_base_FlagsTester_SOURCES = common/base/FlagsTest.cpp
common_base_FlagsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_base_FlagsTester_LDADD = $(COMMON_TESTING_LIBS)

common_base_LoggingTester_SOURCES = common/base/LoggingTest.cpp
common_base_LoggingTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_base_LoggingTester_LDADD = $(COMMON_TESTING_LIBS)
