include $(top_srcdir)/common.mk

# pkg-config
##################################################
pkgconfigdir = $(libdir)/pkgconfig
if INSTALL_E133
pkgconfig_DATA = libolae133slp.pc libolae133common.pc libolae133controller.pc
endif

# LIBRARIES
##################################################
# olae133device depends on olaee133slp. On Ubunutu (and maybe others)
# the order the libs are installed in matters otherwise libtool fails with
# cannot find -lolae133slp
E133_LIBS = libolae133common.la \
            libolae133controller.la \
            libolae133slp.la \
            libolae133device.la

noinst_LTLIBRARIES =
lib_LTLIBRARIES =

if INSTALL_E133
lib_LTLIBRARIES += $(E133_LIBS)
else
noinst_LTLIBRARIES += $(E133_LIBS)
endif

# libolae133slp
# Everything required for SLP discovery, either via openslp or by
# communicating with the OLA SLP server.
libolae133slp_la_SOURCES = OLASLPThread.cpp \
                           E133URLParser.cpp \
                           SLPThread.cpp \
                           SLPConstants.h
libolae133slp_la_LIBADD = $(top_builddir)/slp/libolaslpclient.la \
                          $(top_builddir)/common/libolacommon.la

if HAVE_SLP
libolae133slp_la_SOURCES += OpenSLPThread.cpp
libolae133slp_la_LIBADD += $(openslp_LIBS)
endif

# libolae133common
# Code required by both the controller and device.
libolae133common_la_SOURCES = E133HealthCheckedConnection.cpp \
                              E133HealthCheckedConnection.h \
                              E133Receiver.cpp \
                              E133StatusHelper.cpp \
                              MessageQueue.cpp \
                              MessageQueue.h \
                              MessageBuilder.cpp
libolae133common_la_LIBADD = ../../plugins/e131/e131/libolae131core.la

# libolae133controller
# Controller side.
libolae133controller_la_SOURCES = DeviceManager.cpp \
                                  DeviceManagerImpl.cpp DeviceManagerImpl.h
libolae133controller_la_LIBADD = \
    $(top_builddir)/common/libolacommon.la \
    $(top_builddir)/plugins/e131/e131/libolae131core.la \
    $(top_builddir)/slp/libolaslpclient.la \
    libolae133common.la

# libolae133device
# Device side.
libolae133device_la_SOURCES = DesignatedControllerConnection.cpp \
                              DesignatedControllerConnection.h \
                              E133Device.cpp \
                              E133Device.h \
                              E133Endpoint.cpp \
                              E133Endpoint.h \
                              EndpointManager.cpp \
                              EndpointManager.h \
                              ManagementEndpoint.cpp \
                              ManagementEndpoint.h \
                              SimpleE133Node.cpp \
                              SimpleE133Node.h \
                              TCPConnectionStats.h

libolae133device_la_LIBADD = \
    ../../common/libolacommon.la \
    ../../plugins/e131/e131/libolae131core.la \
    ../../slp/libolaslpclient.la \
    libolae133slp.la \
    libolae133common.la


# PROGRAMS
##################################################
noinst_PROGRAMS = basic_device e133_controller e133_monitor e133_receiver \
                  slp_locate slp_register slp_sa_test

e133_receiver_SOURCES = e133-receiver.cpp
e133_receiver_LDADD = ../../common/libolacommon.la \
                      ../../plugins/e131/e131/libolaacn.la \
                      ../../plugins/usbpro/libolausbprowidget.la \
                      libolae133device.la

if USE_SPI
e133_receiver_LDADD += ../../plugins/spi/libolaspicore.la
endif

e133_monitor_SOURCES = e133-monitor.cpp
e133_monitor_LDADD = ../../common/libolacommon.la \
                     ../../plugins/e131/e131/libolaacn.la \
                     libolae133slp.la \
                     libolae133common.la \
                     libolae133controller.la

e133_controller_SOURCES = e133-controller.cpp
# required for PID_DATA_FILE
e133_controller_LDADD = ../../common/libolacommon.la \
                        ../../plugins/e131/e131/libolae131core.la \
                        libolae133slp.la \
                        libolae133common.la \
                        libolae133controller.la

slp_locate_SOURCES = slp-locate.cpp
slp_locate_LDADD = ../../common/libolacommon.la \
                   libolae133slp.la

basic_device_SOURCES = basic-device.cpp
basic_device_LDADD = ../../common/libolacommon.la \
                     ../../plugins/e131/e131/libolaacn.la \
                     ./libolae133common.la

slp_register_SOURCES = slp-register.cpp
slp_register_LDADD = ../../common/libolacommon.la \
                     libolae133slp.la

slp_sa_test_SOURCES = slp-sa-test.cpp \
                      SLPSATestHelpers.cpp \
                      SLPSATestHelpers.h \
                      SLPSATestRunner.cpp \
                      SLPSATestRunner.h \
                      SLPSATest.cpp
slp_sa_test_LDADD = ../../common/libolacommon.la \
                    ../../slp/libolaslpserver.la \
                    libolae133slp.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = E133SLPTester
endif
check_PROGRAMS = $(TESTS)

E133SLPTester_SOURCES = E133URLParserTest.cpp
E133SLPTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
E133SLPTester_LDADD = $(COMMON_TESTING_LIBS) \
                      ../../common/libolacommon.la \
                      libolae133slp.la
