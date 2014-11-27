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
    plugins/usbdmx/Flags.cpp \
    plugins/usbdmx/LibUsbAdaptor.cpp \
    plugins/usbdmx/LibUsbAdaptor.h \
    plugins/usbdmx/LibUsbThread.cpp \
    plugins/usbdmx/LibUsbThread.h \
    plugins/usbdmx/ScanlimeFadecandy.cpp \
    plugins/usbdmx/ScanlimeFadecandy.h \
    plugins/usbdmx/ScanlimeFadecandyFactory.cpp \
    plugins/usbdmx/ScanlimeFadecandyFactory.h \
    plugins/usbdmx/SyncronizedWidgetObserver.cpp \
    plugins/usbdmx/SyncronizedWidgetObserver.h \
    plugins/usbdmx/ThreadedUsbSender.cpp \
    plugins/usbdmx/ThreadedUsbSender.h \
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
    plugins/usbdmx/AnymaDevice.cpp \
    plugins/usbdmx/AnymaDevice.h \
    plugins/usbdmx/AnymaOutputPort.cpp \
    plugins/usbdmx/AnymaOutputPort.h \
    plugins/usbdmx/EuroliteProDevice.cpp \
    plugins/usbdmx/EuroliteProDevice.h \
    plugins/usbdmx/EuroliteProOutputPort.cpp \
    plugins/usbdmx/EuroliteProOutputPort.h \
    plugins/usbdmx/LibUsbUtils.cpp \
    plugins/usbdmx/LibUsbUtils.h \
    plugins/usbdmx/FirmwareLoader.h \
    plugins/usbdmx/ScanlimeDevice.cpp \
    plugins/usbdmx/ScanlimeDevice.h \
    plugins/usbdmx/ScanlimeOutputPort.cpp \
    plugins/usbdmx/ScanlimeOutputPort.h \
    plugins/usbdmx/SunliteDevice.cpp \
    plugins/usbdmx/SunliteDevice.h \
    plugins/usbdmx/SunliteFirmware.h \
    plugins/usbdmx/SunliteFirmwareLoader.cpp \
    plugins/usbdmx/SunliteFirmwareLoader.h \
    plugins/usbdmx/SunliteOutputPort.cpp \
    plugins/usbdmx/SunliteOutputPort.h \
    plugins/usbdmx/UsbDevice.h \
    plugins/usbdmx/UsbDmxPlugin.cpp \
    plugins/usbdmx/UsbDmxPlugin.h \
    plugins/usbdmx/VellemanDevice.cpp \
    plugins/usbdmx/VellemanDevice.h \
    plugins/usbdmx/VellemanOutputPort.cpp \
    plugins/usbdmx/VellemanOutputPort.h
plugins_usbdmx_libolausbdmx_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
plugins_usbdmx_libolausbdmx_la_LIBADD = \
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
