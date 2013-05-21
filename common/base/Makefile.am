include $(top_srcdir)/common.mk

EXTRA_DIST = SystemExits.h

noinst_LTLIBRARIES = libolabase.la
libolabase_la_SOURCES = Credentials.cpp \
                        Flags.cpp \
                        Init.cpp \
                        Logging.cpp

if BUILD_TESTS
TESTS = CredentialsTester FlagsTester LoggingTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ./libolabase.la \
                    ../utils/libolautils.la

CredentialsTester_SOURCES = CredentialsTest.cpp
CredentialsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
CredentialsTester_LDADD = $(COMMON_TEST_LDADD)

FlagsTester_SOURCES = FlagsTest.cpp
FlagsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
FlagsTester_LDADD = $(COMMON_TEST_LDADD)

LoggingTester_SOURCES = LoggingTest.cpp
LoggingTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
LoggingTester_LDADD = $(COMMON_TEST_LDADD)
