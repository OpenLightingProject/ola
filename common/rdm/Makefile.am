include $(top_srcdir)/common.mk


noinst_LTLIBRARIES = libolardm.la
libolardm_la_SOURCES = RDMCommand.cpp RDMAPI.cpp

TESTS = RDMTester
check_PROGRAMS = $(TESTS)
RDMTester_SOURCES = RDMAPITest.cpp RDMCommandTest.cpp RDMTester.cpp UIDTest.cpp
RDMTester_CPPFLAGS = $(CPPUNIT_CFLAGS) $(AM_CPPFLAGS)
RDMTester_LDFLAGS = $(CPPUNIT_LIBS)
RDMTester_LDADD = libolardm.la ../logging/liblogging.la \
                  ../network/libolanetwork.la ../utils/libolautils.la
