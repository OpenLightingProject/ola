# LIBRARIES
##################################################
if USE_LIBUSB
lib_LTLIBRARIES += plugins/usbdmx/libolausbdmx.la
plugins_usbdmx_libolausbdmx_la_SOURCES = \
    plugins/usbdmx/AnymaDevice.cpp \
    plugins/usbdmx/AnymaDevice.h \
    plugins/usbdmx/AnymaDeviceManager.cpp \
    plugins/usbdmx/AnymaDeviceManager.h \
    plugins/usbdmx/AnymaOutputPort.cpp \
    plugins/usbdmx/AnymaOutputPort.h \
    plugins/usbdmx/AnymaWidget.cpp \
    plugins/usbdmx/AnymaWidget.h \
    plugins/usbdmx/AsyncPluginImpl.cpp \
    plugins/usbdmx/AsyncPluginImpl.h \
    plugins/usbdmx/EuroliteProDevice.cpp \
    plugins/usbdmx/EuroliteProDevice.h \
    plugins/usbdmx/EuroliteProWidget.h \
    plugins/usbdmx/EuroliteProWidget.cpp \
    plugins/usbdmx/EuroliteProDeviceManager.h \
    plugins/usbdmx/EuroliteProDeviceManager.cpp \
    plugins/usbdmx/EuroliteProOutputPort.cpp \
    plugins/usbdmx/EuroliteProOutputPort.h \
    plugins/usbdmx/FirmwareLoader.h \
    plugins/usbdmx/LibUsbAdaptor.cpp \
    plugins/usbdmx/LibUsbAdaptor.h \
    plugins/usbdmx/LibUsbHelper.cpp \
    plugins/usbdmx/LibUsbHelper.h \
    plugins/usbdmx/PluginImplInterface.h \
    plugins/usbdmx/SunliteDevice.cpp \
    plugins/usbdmx/SunliteDevice.h \
    plugins/usbdmx/SunliteDeviceManager.cpp \
    plugins/usbdmx/SunliteDeviceManager.h \
    plugins/usbdmx/SunliteFirmware.h \
    plugins/usbdmx/SunliteFirmwareLoader.cpp \
    plugins/usbdmx/SunliteFirmwareLoader.h \
    plugins/usbdmx/SunliteOutputPort.cpp \
    plugins/usbdmx/SunliteOutputPort.h \
    plugins/usbdmx/SunliteWidget.cpp \
    plugins/usbdmx/SunliteWidget.h \
    plugins/usbdmx/SyncPluginImpl.cpp \
    plugins/usbdmx/SyncPluginImpl.h \
    plugins/usbdmx/ThreadedUsbSender.cpp \
    plugins/usbdmx/ThreadedUsbSender.h \
    plugins/usbdmx/UsbDevice.h \
    plugins/usbdmx/UsbDeviceManagerInterface.h \
    plugins/usbdmx/UsbDmxPlugin.cpp \
    plugins/usbdmx/UsbDmxPlugin.h \
    plugins/usbdmx/VellemanDevice.cpp \
    plugins/usbdmx/VellemanDevice.h \
    plugins/usbdmx/VellemanOutputPort.cpp \
    plugins/usbdmx/VellemanOutputPort.h

plugins_usbdmx_libolausbdmx_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
plugins_usbdmx_libolausbdmx_la_LIBADD = $(libusb_LIBS) \
                                        common/libolacommon.la
endif
