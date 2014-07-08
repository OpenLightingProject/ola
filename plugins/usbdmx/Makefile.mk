# LIBRARIES
##################################################
if USE_LIBUSB
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
    plugins/usbdmx/FirmwareLoader.h \
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
plugins_usbdmx_libolausbdmx_la_LIBADD = $(libusb_LIBS) \
                                        common/libolacommon.la
endif
