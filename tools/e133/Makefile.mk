# pkg-config
##################################################
if INSTALL_E133
pkgconfig_DATA += \
    tools/e133/libolae133slp.pc \
    tools/e133/libolae133common.pc \
    tools/e133/libolae133controller.pc
endif

# LIBRARIES
##################################################
# olae133device depends on olaee133slp. On Ubunutu (and maybe others)
# the order the libs are installed in matters otherwise libtool fails with
# cannot find -lolae133slp
E133_LIBS = \
    tools/e133/libolae133common.la \
    tools/e133/libolae133controller.la \
    tools/e133/libolae133slp.la \
    tools/e133/libolae133device.la

if INSTALL_E133
lib_LTLIBRARIES += $(E133_LIBS)
else
noinst_LTLIBRARIES += $(E133_LIBS)
endif

# libolae133slp
# Everything required for SLP discovery, either via openslp or by
# communicating with the OLA SLP server.
tools_e133_libolae133slp_la_SOURCES = \
    tools/e133/OLASLPThread.cpp \
    tools/e133/E133URLParser.cpp \
    tools/e133/SLPThread.cpp \
    tools/e133/SLPConstants.h
tools_e133_libolae133slp_la_LIBADD = slp/libolaslpclient.la \
                                     common/libolacommon.la

if HAVE_SLP
tools_e133_libolae133slp_la_SOURCES += tools/e133/OpenSLPThread.cpp
tools_e133_libolae133slp_la_LIBADD += $(openslp_LIBS)
endif

# libolae133common
# Code required by both the controller and device.
tools_e133_libolae133common_la_SOURCES = \
    tools/e133/E133HealthCheckedConnection.cpp \
    tools/e133/E133HealthCheckedConnection.h \
    tools/e133/E133Receiver.cpp \
    tools/e133/E133StatusHelper.cpp \
    tools/e133/MessageQueue.cpp \
    tools/e133/MessageQueue.h \
    tools/e133/MessageBuilder.cpp
tools_e133_libolae133common_la_LIBADD = plugins/e131/e131/libolae131core.la

# libolae133controller
# Controller side.
tools_e133_libolae133controller_la_SOURCES = \
    tools/e133/DeviceManager.cpp \
    tools/e133/DeviceManagerImpl.cpp \
    tools/e133/DeviceManagerImpl.h
tools_e133_libolae133controller_la_LIBADD = \
    common/libolacommon.la \
    plugins/e131/e131/libolae131core.la \
    slp/libolaslpclient.la \
    tools/e133/libolae133common.la

# libolae133device
# Device side.
tools_e133_libolae133device_la_SOURCES = \
    tools/e133/DesignatedControllerConnection.cpp \
    tools/e133/DesignatedControllerConnection.h \
    tools/e133/E133Device.cpp \
    tools/e133/E133Device.h \
    tools/e133/E133Endpoint.cpp \
    tools/e133/E133Endpoint.h \
    tools/e133/EndpointManager.cpp \
    tools/e133/EndpointManager.h \
    tools/e133/ManagementEndpoint.cpp \
    tools/e133/ManagementEndpoint.h \
    tools/e133/SimpleE133Node.cpp \
    tools/e133/SimpleE133Node.h \
    tools/e133/TCPConnectionStats.h

tools_e133_libolae133device_la_LIBADD = \
    common/libolacommon.la \
    plugins/e131/e131/libolae131core.la \
    slp/libolaslpclient.la \
    tools/e133/libolae133slp.la \
    tools/e133/libolae133common.la


# PROGRAMS
##################################################
noinst_PROGRAMS += \
    tools/e133/basic_controller \
    tools/e133/basic_device \
    tools/e133/e133_controller \
    tools/e133/e133_monitor \
    tools/e133/e133_receiver \
    tools/e133/slp_locate \
    tools/e133/slp_register \
    tools/e133/slp_sa_test

tools_e133_e133_receiver_SOURCES = tools/e133/e133-receiver.cpp
tools_e133_e133_receiver_LDADD = common/libolacommon.la \
                                 plugins/e131/e131/libolaacn.la \
                                 plugins/usbpro/libolausbprowidget.la \
                                 tools/e133/libolae133device.la

if USE_SPI
tools_e133_e133_receiver_LDADD += plugins/spi/libolaspicore.la
endif

tools_e133_e133_monitor_SOURCES = tools/e133/e133-monitor.cpp
tools_e133_e133_monitor_LDADD = common/libolacommon.la \
                                plugins/e131/e131/libolaacn.la \
                                tools/e133/libolae133slp.la \
                                tools/e133/libolae133common.la \
                                tools/e133/libolae133controller.la

tools_e133_e133_controller_SOURCES = tools/e133/e133-controller.cpp
# required for PID_DATA_FILE
tools_e133_e133_controller_LDADD = common/libolacommon.la \
                                   plugins/e131/e131/libolae131core.la \
                                   tools/e133/libolae133slp.la \
                                   tools/e133/libolae133common.la \
                                   tools/e133/libolae133controller.la

tools_e133_slp_locate_SOURCES = tools/e133/slp-locate.cpp
tools_e133_slp_locate_LDADD = common/libolacommon.la \
                              tools/e133/libolae133slp.la

tools_e133_slp_register_SOURCES = tools/e133/slp-register.cpp
tools_e133_slp_register_LDADD = common/libolacommon.la \
                                tools/e133/libolae133slp.la

tools_e133_slp_sa_test_SOURCES = \
    tools/e133/slp-sa-test.cpp \
    tools/e133/SLPSATestHelpers.cpp \
    tools/e133/SLPSATestHelpers.h \
    tools/e133/SLPSATestRunner.cpp \
    tools/e133/SLPSATestRunner.h \
    tools/e133/SLPSATest.cpp
tools_e133_slp_sa_test_LDADD = common/libolacommon.la \
                               slp/libolaslpserver.la \
                               tools/e133/libolae133slp.la

tools_e133_basic_controller_SOURCES = tools/e133/basic-controller.cpp
tools_e133_basic_controller_LDADD = common/libolacommon.la \
                                    plugins/e131/e131/libolaacn.la \
                                    tools/e133/libolae133common.la

tools_e133_basic_device_SOURCES = tools/e133/basic-device.cpp
tools_e133_basic_device_LDADD = common/libolacommon.la \
                                plugins/e131/e131/libolaacn.la \
                                tools/e133/libolae133common.la

# TESTS
##################################################
test_programs += tools/e133/E133SLPTester

tools_e133_E133SLPTester_SOURCES = tools/e133/E133URLParserTest.cpp
tools_e133_E133SLPTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
tools_e133_E133SLPTester_LDADD = $(COMMON_TESTING_LIBS) \
                                 tools/e133/libolae133slp.la
