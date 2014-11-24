# LIBRARIES
##################################################
if USE_LIBUSB
lib_LTLIBRARIES += plugins/usbdmx/libolausbdmx.la
plugins_usbdmx_libolausbdmx_la_SOURCES = \
    plugins/usbdmx/AnymaWidget.cpp \
    plugins/usbdmx/AnymaWidget.h \
    plugins/usbdmx/AnymaWidgetFactory.cpp \
    plugins/usbdmx/AnymaWidgetFactory.h \
    plugins/usbdmx/AsyncPluginImpl.cpp \
    plugins/usbdmx/AsyncPluginImpl.h \
    plugins/usbdmx/AsyncUsbSender.cpp \
    plugins/usbdmx/AsyncUsbSender.h \
    plugins/usbdmx/EuroliteProWidget.cpp \
    plugins/usbdmx/EuroliteProWidget.h \
    plugins/usbdmx/EuroliteProWidgetFactory.cpp \
    plugins/usbdmx/EuroliteProWidgetFactory.h \
    plugins/usbdmx/FadecandyWidget.cpp \
    plugins/usbdmx/FadecandyWidget.h \
    plugins/usbdmx/FadecandyWidgetFactory.cpp \
    plugins/usbdmx/FadecandyWidgetFactory.h \
    plugins/usbdmx/FirmwareLoader.h \
    plugins/usbdmx/GenericDevice.cpp \
    plugins/usbdmx/GenericDevice.h \
    plugins/usbdmx/GenericOutputPort.cpp \
    plugins/usbdmx/GenericOutputPort.h \
    plugins/usbdmx/LibUsbAdaptor.cpp \
    plugins/usbdmx/LibUsbAdaptor.h \
    plugins/usbdmx/LibUsbThread.cpp \
    plugins/usbdmx/LibUsbThread.h \
    plugins/usbdmx/PluginImplInterface.h \
    plugins/usbdmx/SunliteFirmware.h \
    plugins/usbdmx/SunliteFirmwareLoader.cpp \
    plugins/usbdmx/SunliteFirmwareLoader.h \
    plugins/usbdmx/SunliteWidget.cpp \
    plugins/usbdmx/SunliteWidget.h \
    plugins/usbdmx/SunliteWidgetFactory.cpp \
    plugins/usbdmx/SunliteWidgetFactory.h \
    plugins/usbdmx/SyncPluginImpl.cpp \
    plugins/usbdmx/SyncPluginImpl.h \
    plugins/usbdmx/SyncronizedWidgetObserver.cpp \
    plugins/usbdmx/SyncronizedWidgetObserver.h \
    plugins/usbdmx/ThreadedUsbSender.cpp \
    plugins/usbdmx/ThreadedUsbSender.h \
    plugins/usbdmx/UsbDmxPlugin.cpp \
    plugins/usbdmx/UsbDmxPlugin.h \
    plugins/usbdmx/VellemanWidget.cpp \
    plugins/usbdmx/VellemanWidget.h \
    plugins/usbdmx/VellemanWidgetFactory.cpp \
    plugins/usbdmx/VellemanWidgetFactory.h \
    plugins/usbdmx/Widget.h \
    plugins/usbdmx/WidgetFactory.h

plugins_usbdmx_libolausbdmx_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
plugins_usbdmx_libolausbdmx_la_LIBADD = $(libusb_LIBS) \
                                        common/libolacommon.la
endif
