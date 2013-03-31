include $(top_srcdir)/common.mk

EXTRA_DIST = E133Endpoint.h E133Device.h \
             E133HealthCheckedConnection.h \
             E133StreamSender.h \
             E133URLParser.h \
             EndpointManager.h \
             OLASLPThread.h \
             OpenSLPThread.h \
             PacketBuilder.h \
             RootEndpoint.h \
             SLPConstants.h \
             SLPSATestHelpers.h \
             SLPSATestRunner.h \
             SLPThread.h \
             TCPConnectionStats.h

noinst_LTLIBRARIES = libolae133slp.la libolae133.la
libolae133slp_la_SOURCES = OLASLPThread.cpp \
                           E133URLParser.cpp \
                           OpenSLPThread.cpp \
                           SLPThread.cpp
libolae133slp_la_LIBADD = $(openslp_LIBS) \
                          $(top_builddir)/tools/slp/libolaslpclient.la \
                          $(top_builddir)/common/libolacommon.la

if HAVE_SLP
libolae133slp_la_SOURCES += OpenSLPThread.cpp
libolae133slp_la_LIBADD += $(openslp_LIBS)
endif

libolae133_la_SOURCES = PacketBuilder.cpp
libolae133_la_LIBADD = $(top_builddir)/plugins/e131/e131/libolae131core.la

noinst_PROGRAMS = e133_controller e133_monitor e133_receiver \
                  slp_locate slp_register slp_sa_test

e133_receiver_SOURCES = e133-receiver.cpp \
                        E133Device.cpp \
                        E133Endpoint.cpp \
                        E133HealthCheckedConnection.cpp \
                        E133StreamSender.cpp \
                        EndpointManager.cpp \
                        RootEndpoint.cpp
e133_receiver_LDADD = ./libolae133slp.la \
                      ./libolae133.la \
                      $(top_builddir)/common/libolacommon.la \
                      $(top_builddir)/plugins/dummy/liboladummyresponder.la \
                      $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_monitor_SOURCES = e133-monitor.cpp \
                       E133HealthCheckedConnection.cpp \
                       E133StreamSender.cpp
e133_monitor_CXXFLAGS = @OLAD_DEFINES@ $(COMMON_CXXFLAGS)
e133_monitor_LDADD = ./libolae133slp.la \
                      ./libolae133.la \
                     $(top_builddir)/common/libolacommon.la \
                     $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_controller_SOURCES = e133-controller.cpp
# required for PID_DATA_FILE
e133_controller_CXXFLAGS = @OLAD_DEFINES@ $(COMMON_CXXFLAGS)
e133_controller_LDADD = ./libolae133slp.la \
                        ./libolae133.la \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/e131/e131/libolae131core.la

slp_locate_SOURCES = slp-locate.cpp
slp_locate_LDADD = ./libolae133slp.la \
                   $(top_builddir)/common/libolacommon.la

slp_register_SOURCES = slp-register.cpp
slp_register_LDADD = ./libolae133slp.la \
                     $(top_builddir)/common/libolacommon.la

slp_sa_test_SOURCES = slp-sa-test.cpp \
                      SLPSATestHelpers.cpp \
                      SLPSATestRunner.cpp \
                      SLPSATests.cpp
slp_sa_test_LDADD = $(top_builddir)/common/libolacommon.la \
                    ../slp/libolaslp.la \
                    ./libolae133slp.la

# Test Programs
if BUILD_TESTS
TESTS = E133SLPTester
endif
check_PROGRAMS = $(TESTS)
E133SLPTester_SOURCES = E133SLPTester.cpp E133URLParserTest.cpp
E133SLPTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
E133SLPTester_LDADD = $(CPPUNIT_LIBS) \
                      ./libolae133slp.la \
                      $(top_builddir)/common/libolacommon.la
