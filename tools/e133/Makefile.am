include $(top_srcdir)/common.mk

EXTRA_DIST = DeviceManagerImpl.h \
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

if INSTALL_E133
pkgconfig_DATA = libolae133slp.pc libolae133common.pc libolae133controller.pc
endif

# Libraries
#########################
E133_LIBS = libolae133common.la \
            libolae133controller.la \
            libolae133slp.la

noinst_LTLIBRARIES =
lib_LTLIBRARIES =

if INSTALL_E133
  lib_LTLIBRARIES += $(E133_LIBS)
else
  noinst_LTLIBRARIES += $(E133_LIBS)
endif

# libolae133slp.la
# Everything required for SLP discovery, either via openslp or by
# communicating with the OLA SLP server.
libolae133slp_la_SOURCES = OLASLPThread.cpp \
                           E133URLParser.cpp \
                           SLPThread.cpp
libolae133slp_la_LIBADD = $(top_builddir)/slp/libolaslpclient.la \
                          $(top_builddir)/common/libolacommon.la

if HAVE_SLP
libolae133slp_la_SOURCES += OpenSLPThread.cpp
libolae133slp_la_LIBADD += $(openslp_LIBS)
endif

# libolae133common.la
# Code required by both the controller and device.
libolae133common_la_SOURCES = E133HealthCheckedConnection.cpp \
                              E133Receiver.cpp \
                              E133StatusHelper.cpp \
                              MessageQueue.cpp \
                              MessageBuilder.cpp
libolae133common_la_LIBADD = $(top_builddir)/plugins/e131/e131/libolae131core.la

# libolae133controller.la
# Controller side.
libolae133controller_la_SOURCES = DeviceManager.cpp \
                                  DeviceManagerImpl.cpp
libolae133controller_la_LIBADD = \
    $(top_builddir)/common/libolacommon.la \
    $(top_builddir)/plugins/e131/e131/libolae131core.la \
    $(top_builddir)/slp/libolaslpclient.la \
    ./libolae133common.la


# Programs
#########################
noinst_PROGRAMS = e133_controller e133_monitor e133_receiver \
                  slp_locate slp_register slp_sa_test

e133_receiver_SOURCES = e133-receiver.cpp \
                        DesignatedControllerConnection.cpp \
                        E133Device.cpp \
                        E133Endpoint.cpp \
                        EndpointManager.cpp \
                        ManagementEndpoint.cpp
e133_receiver_LDADD = ./libolae133slp.la \
                      ./libolae133common.la \
                      $(top_builddir)/common/libolacommon.la \
                      $(top_builddir)/plugins/dummy/liboladummyresponder.la \
                      $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_monitor_SOURCES = e133-monitor.cpp
e133_monitor_CXXFLAGS = @OLAD_DEFINES@ $(COMMON_CXXFLAGS)
e133_monitor_LDADD = ./libolae133slp.la \
                     ./libolae133controller.la \
                     $(top_builddir)/common/libolacommon.la

e133_controller_SOURCES = e133-controller.cpp
# required for PID_DATA_FILE
e133_controller_CXXFLAGS = @OLAD_DEFINES@ $(COMMON_CXXFLAGS)
e133_controller_LDADD = $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/e131/e131/libolae131core.la \
                        ./libolae133slp.la \
                        ./libolae133controller.la

slp_locate_SOURCES = slp-locate.cpp
slp_locate_LDADD = $(top_builddir)/common/libolacommon.la \
                   ./libolae133slp.la

slp_register_SOURCES = slp-register.cpp
slp_register_LDADD = $(top_builddir)/common/libolacommon.la \
                     ./libolae133slp.la

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
