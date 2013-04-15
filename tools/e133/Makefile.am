include $(top_srcdir)/common.mk

EXTRA_DIST = DeviceManager.h \
             DesignatedControllerConnection.h \
             E133Device.h \
             E133Endpoint.h \
             E133HealthCheckedConnection.h \
             EndpointManager.h \
             MessageQueue.h \
             ManagementEndpoint.h \
             SLPConstants.h \
             SLPSATestHelpers.h \
             SLPSATestRunner.h \
             TCPConnectionStats.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libolae133slp.pc

# Libraries
#########################
lib_LTLIBRARIES = libolae133slp.la
noinst_LTLIBRARIES =

if INSTALL_E133
  lib_LTLIBRARIES += libolae133.la
else
  noinst_LTLIBRARIES += libolae133.la
endif

# libolae133slp.la
libolae133slp_la_SOURCES = OLASLPThread.cpp \
                           E133URLParser.cpp \
                           SLPThread.cpp
libolae133slp_la_LIBADD = $(top_builddir)/slp/libolaslpclient.la \
                          $(top_builddir)/common/libolacommon.la

if HAVE_SLP
libolae133slp_la_SOURCES += OpenSLPThread.cpp
libolae133slp_la_LIBADD += $(openslp_LIBS)
endif

# libolae133.la
libolae133_la_SOURCES = MessageQueue.cpp \
                        MessageBuilder.cpp
libolae133_la_LIBADD = $(top_builddir)/plugins/e131/e131/libolae131core.la

# Programs
#########################
noinst_PROGRAMS = e133_controller e133_monitor e133_receiver \
                  slp_locate slp_register slp_sa_test

e133_receiver_SOURCES = e133-receiver.cpp \
                        DesignatedControllerConnection.cpp \
                        E133Device.cpp \
                        E133Endpoint.cpp \
                        E133HealthCheckedConnection.cpp \
                        EndpointManager.cpp \
                        ManagementEndpoint.cpp
e133_receiver_LDADD = ./libolae133slp.la \
                      ./libolae133.la \
                      $(top_builddir)/common/libolacommon.la \
                      $(top_builddir)/plugins/dummy/liboladummyresponder.la \
                      $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_monitor_SOURCES = e133-monitor.cpp \
                       DeviceManager.cpp \
                       E133HealthCheckedConnection.cpp
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
                    $(top_builddir)/slp/libolaslpserver.la \
                    ./libolae133slp.la

# Tests
#########################
if BUILD_TESTS
TESTS = E133SLPTester
endif
check_PROGRAMS = $(TESTS)
E133SLPTester_SOURCES = E133SLPTester.cpp E133URLParserTest.cpp
E133SLPTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
E133SLPTester_LDADD = $(CPPUNIT_LIBS) \
                      ./libolae133slp.la \
                      $(top_builddir)/common/libolacommon.la
