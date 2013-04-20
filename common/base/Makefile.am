include $(top_srcdir)/common.mk

EXTRA_DIST = SystemExits.h

noinst_LTLIBRARIES = libolabase.la
libolabase_la_SOURCES = Credentials.cpp \
                        Logging.cpp \
                        Flags.cpp \
                        Init.cpp

if BUILD_TESTS
TESTS = CredentialsTester FlagsTester LoggingTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(CPPUNIT_LIBS) \
                    ./libolabase.la \
                    ../utils/libolautils.la \
                    ../testing/libolatesting.la

CredentialsTester_SOURCES = CredentialsTest.cpp \
                            GenericTester.cpp
CredentialsTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
CredentialsTester_LDADD = $(COMMON_TEST_LDADD)

FlagsTester_SOURCES = FlagsTest.cpp \
                      GenericTester.cpp
FlagsTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
FlagsTester_LDADD = $(COMMON_TEST_LDADD)

LoggingTester_SOURCES = GenericTester.cpp LoggingTest.cpp
LoggingTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
LoggingTester_LDADD = $(COMMON_TEST_LDADD)
