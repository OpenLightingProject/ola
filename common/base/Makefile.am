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

CredentialsTester_SOURCES = CredentialsTest.cpp \
                            GenericTester.cpp
CredentialsTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
CredentialsTester_LDADD = $(CPPUNIT_LIBS) \
                          ../libolacommon.la

FlagsTester_SOURCES = FlagsTest.cpp \
                      GenericTester.cpp
FlagsTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
FlagsTester_LDADD = $(CPPUNIT_LIBS) \
                    ../libolacommon.la

LoggingTester_SOURCES = LoggingTester.cpp LoggingTest.cpp
LoggingTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
LoggingTester_LDADD = $(CPPUNIT_LIBS) \
                      ../libolacommon.la
