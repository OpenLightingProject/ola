include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolabase.la
libolabase_la_SOURCES = Credentials.cpp \
                        Init.cpp

if BUILD_TESTS
TESTS = CredentialsTester
endif
check_PROGRAMS = $(TESTS)


CredentialsTester_SOURCES = CredentialsTest.cpp \
                            CredentialsTester.cpp
CredentialsTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
CredentialsTester_LDADD = $(CPPUNIT_LIBS) \
                          ../libolacommon.la

