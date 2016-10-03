# LIBRARIES
##################################################
if USE_LIBUSB
noinst_LTLIBRARIES += plugins/usbdmx/libolausbdmxwidget.la

plugins_usbdmx_libolausbdmxwidget_la_SOURCES = \
    plugins/usbdmx/AnymauDMX.cpp \
    plugins/usbdmx/AnymauDMX.h \
    plugins/usbdmx/AnymauDMXFactory.cpp \
    plugins/usbdmx/AnymauDMXFactory.h \
    plugins/usbdmx/AVLdiyD512.cpp \
    plugins/usbdmx/AVLdiyD512.h \
    plugins/usbdmx/AVLdiyD512Factory.cpp \
    plugins/usbdmx/AVLdiyD512Factory.h \
    plugins/usbdmx/AsyncUsbReceiver.cpp \
    plugins/usbdmx/AsyncUsbReceiver.h \
    plugins/usbdmx/AsyncUsbSender.cpp \
    plugins/usbdmx/AsyncUsbSender.h \
    plugins/usbdmx/AsyncUsbTransceiverBase.cpp \
    plugins/usbdmx/AsyncUsbTransceiverBase.h \
    plugins/usbdmx/DMXCProjectsNodleU1.cpp \
    plugins/usbdmx/DMXCProjectsNodleU1.h \
    plugins/usbdmx/DMXCProjectsNodleU1Factory.cpp \
    plugins/usbdmx/DMXCProjectsNodleU1Factory.h \
    plugins/usbdmx/EurolitePro.cpp \
    plugins/usbdmx/EurolitePro.h \
    plugins/usbdmx/EuroliteProFactory.cpp \
    plugins/usbdmx/EuroliteProFactory.h \
    plugins/usbdmx/FirmwareLoader.h \
    plugins/usbdmx/Flags.cpp \
    plugins/usbdmx/JaRuleFactory.cpp \
    plugins/usbdmx/JaRuleFactory.h \
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
    plugins/usbdmx/ThreadedUsbReceiver.cpp \
    plugins/usbdmx/ThreadedUsbReceiver.h \
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
    common/libolacommon.la \
    libs/usb/libolausb.la

lib_LTLIBRARIES += plugins/usbdmx/libolausbdmx.la
plugins_usbdmx_libolausbdmx_la_SOURCES = \
    plugins/usbdmx/AsyncPluginImpl.cpp \
    plugins/usbdmx/AsyncPluginImpl.h \
    plugins/usbdmx/DMXCProjectsNodleU1Device.cpp \
    plugins/usbdmx/DMXCProjectsNodleU1Device.h \
    plugins/usbdmx/DMXCProjectsNodleU1Port.cpp \
    plugins/usbdmx/DMXCProjectsNodleU1Port.h \
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
endif

EXTRA_DIST += plugins/usbdmx/README.md
