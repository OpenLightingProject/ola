# pkg-config
##################################################
if INSTALL_E133
pkgconfig_DATA += \
    tools/e133/libolae133common.pc \
    tools/e133/libolae133controller.pc
endif

# LIBRARIES
##################################################
E133_LIBS = \
    tools/e133/libolae133common.la \
    tools/e133/libolae133controller.la \
    tools/e133/libolae133device.la

if INSTALL_E133
lib_LTLIBRARIES += $(E133_LIBS)
else
noinst_LTLIBRARIES += $(E133_LIBS)
endif

# libolae133common
# Code required by both the controller and device.
tools_e133_libolae133common_la_SOURCES = \
    tools/e133/E133HealthCheckedConnection.cpp \
    tools/e133/E133HealthCheckedConnection.h \
    tools/e133/E133Helper.cpp \
    tools/e133/E133Receiver.cpp \
    tools/e133/E133StatusHelper.cpp \
    tools/e133/MessageBuilder.cpp
tools_e133_libolae133common_la_LIBADD = libs/acn/libolae131core.la

# libolae133controller
# Controller side.
tools_e133_libolae133controller_la_SOURCES = \
    tools/e133/DeviceManager.cpp \
    tools/e133/DeviceManagerImpl.cpp \
    tools/e133/DeviceManagerImpl.h
tools_e133_libolae133controller_la_LIBADD = \
    common/libolacommon.la \
    libs/acn/libolae131core.la \
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
    libs/acn/libolae131core.la \
    tools/e133/libolae133common.la


# PROGRAMS
##################################################
noinst_PROGRAMS += \
    tools/e133/basic_controller \
    tools/e133/basic_device \
    tools/e133/e133_controller \
    tools/e133/e133_monitor \
    tools/e133/e133_receiver \
    tools/e133/llrp_manager \
    tools/e133/llrp_target

tools_e133_e133_receiver_SOURCES = tools/e133/e133-receiver.cpp
tools_e133_e133_receiver_LDADD = common/libolacommon.la \
                                 libs/acn/libolaacn.la \
                                 plugins/usbpro/libolausbprowidget.la \
                                 tools/e133/libolae133device.la

if USE_SPI
tools_e133_e133_receiver_LDADD += plugins/spi/libolaspicore.la
endif

tools_e133_e133_monitor_SOURCES = tools/e133/e133-monitor.cpp
tools_e133_e133_monitor_LDADD = common/libolacommon.la \
                                libs/acn/libolaacn.la \
                                tools/e133/libolae133common.la \
                                tools/e133/libolae133controller.la

tools_e133_e133_controller_SOURCES = tools/e133/e133-controller.cpp
# required for PID_DATA_FILE
tools_e133_e133_controller_LDADD = common/libolacommon.la \
                                   libs/acn/libolae131core.la \
                                   tools/e133/libolae133common.la \
                                   tools/e133/libolae133controller.la

tools_e133_basic_controller_SOURCES = tools/e133/basic-controller.cpp
tools_e133_basic_controller_LDADD = common/libolacommon.la \
                                    libs/acn/libolaacn.la \
                                    tools/e133/libolae133common.la

tools_e133_basic_device_SOURCES = tools/e133/basic-device.cpp
tools_e133_basic_device_LDADD = common/libolacommon.la \
                                libs/acn/libolaacn.la \
                                tools/e133/libolae133common.la

tools_e133_llrp_manager_SOURCES = tools/e133/llrp-manager.cpp
tools_e133_llrp_manager_LDADD = common/libolacommon.la \
                                libs/acn/libolaacn.la \
                                tools/e133/libolae133common.la

tools_e133_llrp_target_SOURCES = tools/e133/llrp-target.cpp
tools_e133_llrp_target_LDADD = common/libolacommon.la \
                               libs/acn/libolaacn.la \
                               tools/e133/libolae133common.la
