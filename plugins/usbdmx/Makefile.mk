# LIBRARIES
##################################################
if USE_LIBUSB
noinst_LTLIBRARIES += plugins/usbdmx/libolausbdmxwidget.la

plugins_usbdmx_libolausbdmxwidget_la_SOURCES = \
    plugins/usbdmx/AnymauDMX.cpp \
    plugins/usbdmx/AnymauDMX.h \
    plugins/usbdmx/AnymauDMXFactory.cpp \
    plugins/usbdmx/AnymauDMXFactory.h \
    plugins/usbdmx/AsyncUsbSender.cpp \
    plugins/usbdmx/AsyncUsbSender.h \
    plugins/usbdmx/AsyncUsbSender.h \
    plugins/usbdmx/EurolitePro.cpp \
    plugins/usbdmx/EurolitePro.h \
    plugins/usbdmx/EuroliteProFactory.cpp \
    plugins/usbdmx/EuroliteProFactory.h \
    plugins/usbdmx/FirmwareLoader.h \
    plugins/usbdmx/Flags.cpp \
    plugins/usbdmx/JaRuleEndpoint.cpp \
    plugins/usbdmx/JaRuleEndpoint.h \
    plugins/usbdmx/JaRuleFactory.cpp \
    plugins/usbdmx/JaRuleFactory.h \
    plugins/usbdmx/JaRuleWidget.cpp \
    plugins/usbdmx/JaRuleWidget.h \
    plugins/usbdmx/JaRuleWidgetImpl.cpp \
    plugins/usbdmx/JaRuleWidgetImpl.h \
    plugins/usbdmx/LibUsbAdaptor.cpp \
    plugins/usbdmx/LibUsbAdaptor.h \
    plugins/usbdmx/LibUsbThread.cpp \
    plugins/usbdmx/LibUsbThread.h \
    plugins/usbdmx/ScanlimeFadecandy.cpp \
    plugins/usbdmx/ScanlimeFadecandy.h \
    plugins/usbdmx/ScanlimeFadecandyFactory.cpp \
    plugins/usbdmx/ScanlimeFadecandyFactory.h \
    plugins/usbdmx/Sunlite.cpp \
    plugins/usbdmx/Sunlite.h \
    plugins/usbdmx/SunliteFactory.cpp \
    plugins/usbdmx/SunliteFactory.h \
    plugins/usbdmx/SunliteFirmware.h \
    plugins/usbdmx/SunliteFirmwareLoader.cpp \
    plugins/usbdmx/SunliteFirmwareLoader.h \
    plugins/usbdmx/SyncronizedWidgetObserver.cpp \
    plugins/usbdmx/SyncronizedWidgetObserver.h \
    plugins/usbdmx/ThreadedUsbSender.cpp \
    plugins/usbdmx/ThreadedUsbSender.h \
    plugins/usbdmx/VellemanK8062.cpp \
    plugins/usbdmx/VellemanK8062.h \
    plugins/usbdmx/VellemanK8062Factory.cpp \
    plugins/usbdmx/VellemanK8062Factory.h \
    plugins/usbdmx/Widget.h \
    plugins/usbdmx/WidgetFactory.h
plugins_usbdmx_libolausbdmxwidget_la_CXXFLAGS = \
    $(COMMON_CXXFLAGS) \
    $(libusb_CFLAGS)
plugins_usbdmx_libolausbdmxwidget_la_LIBADD = \
    $(libusb_LIBS) \
    common/libolacommon.la

lib_LTLIBRARIES += plugins/usbdmx/libolausbdmx.la
plugins_usbdmx_libolausbdmx_la_SOURCES = \
    plugins/usbdmx/AsyncPluginImpl.cpp \
    plugins/usbdmx/AsyncPluginImpl.h \
    plugins/usbdmx/GenericDevice.cpp \
    plugins/usbdmx/GenericDevice.h \
    plugins/usbdmx/GenericOutputPort.cpp \
    plugins/usbdmx/GenericOutputPort.h \
    plugins/usbdmx/JaRuleDevice.cpp \
    plugins/usbdmx/JaRuleDevice.h \
    plugins/usbdmx/JaRuleOutputPort.cpp \
    plugins/usbdmx/JaRuleOutputPort.h \
    plugins/usbdmx/PluginImplInterface.h \
    plugins/usbdmx/SyncPluginImpl.cpp \
    plugins/usbdmx/SyncPluginImpl.h \
    plugins/usbdmx/UsbDmxPlugin.cpp \
    plugins/usbdmx/UsbDmxPlugin.h
plugins_usbdmx_libolausbdmx_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
plugins_usbdmx_libolausbdmx_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/usbdmx/libolausbdmxwidget.la

# TESTS
##################################################
test_programs += \
    plugins/usbdmx/LibUsbThreadTester

COMMON_USBDMX_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                           $(libusb_LIBS) \
                           plugins/usbdmx/libolausbdmxwidget.la

plugins_usbdmx_LibUsbThreadTester_SOURCES = \
    plugins/usbdmx/LibUsbThreadTest.cpp
plugins_usbdmx_LibUsbThreadTester_CXXFLAGS = $(COMMON_TESTING_FLAGS) \
                                             $(libusb_CFLAGS)
plugins_usbdmx_LibUsbThreadTester_LDADD = $(COMMON_USBDMX_TEST_LDADD)
endif

EXTRA_DIST += plugins/usbdmx/README.md
