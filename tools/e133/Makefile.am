include $(top_srcdir)/common.mk

EXTRA_DIST = E133Endpoint.h E133Device.h \
             E133HealthCheckedConnection.h \
             E133StreamSender.h \
             EndpointManager.h \
             RootEndpoint.h \
             SlpConstants.h SlpThread.h SlpUrlParser.h \
             TCPConnectionStats.h

if HAVE_SLP
noinst_PROGRAMS = e133_controller e133_monitor e133_receiver \
                  slp_locate slp_register

e133_receiver_SOURCES = e133-receiver.cpp \
                        E133Device.cpp \
                        E133Endpoint.cpp \
                        E133HealthCheckedConnection.cpp \
                        E133StreamSender.cpp \
                        EndpointManager.cpp \
                        RootEndpoint.cpp \
                        SlpThread.cpp
e133_receiver_LDADD = $(openslp_LIBS) \
                      $(top_builddir)/common/libolacommon.la \
                      $(top_builddir)/plugins/dummy/liboladummyresponder.la \
                      $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_monitor_SOURCES = e133-monitor.cpp \
                       E133HealthCheckedConnection.cpp \
                       E133StreamSender.cpp \
                       SlpThread.cpp \
                       SlpUrlParser.cpp
e133_monitor_CXXFLAGS = @OLAD_DEFINES@ $(COMMON_CXXFLAGS)
e133_monitor_LDADD = $(openslp_LIBS) \
                     $(top_builddir)/common/libolacommon.la \
                     $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_controller_SOURCES = e133-controller.cpp \
                          SlpThread.cpp SlpUrlParser.cpp

# required for PID_DATA_FILE
e133_controller_CXXFLAGS = @OLAD_DEFINES@ $(COMMON_CXXFLAGS)
e133_controller_LDADD = $(openslp_LIBS) \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/e131/e131/libolae131core.la

slp_locate_SOURCES = slp-locate.cpp SlpThread.cpp
slp_locate_LDADD = $(openslp_LIBS) \
                   $(top_builddir)/common/libolacommon.la

slp_register_SOURCES = slp-register.cpp SlpThread.cpp
slp_register_LDADD = $(openslp_LIBS) \
                     $(top_builddir)/common/libolacommon.la

# Test Programs
if BUILD_TESTS
TESTS = SlpTester
endif
check_PROGRAMS = $(TESTS)
SlpTester_SOURCES = SlpTester.cpp SlpUrlParser.cpp SlpUrlParserTest.cpp
SlpTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SlpTester_LDADD = $(CPPUNIT_LIBS) $(top_builddir)/common/libolacommon.la

endif
