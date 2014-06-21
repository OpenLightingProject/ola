include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_LIBUSB
plugin_LTLIBRARIES = libolausbdmx.la
libolausbdmx_la_SOURCES = AnymaDevice.cpp AnymaDevice.h AnymaOutputPort.cpp \
                          AnymaOutputPort.h EuroliteProDevice.cpp \
                          EuroliteProDevice.h EuroliteProOutputPort.cpp \
                          EuroliteProOutputPort.h FirmwareLoader.h \
                          SunliteDevice.cpp SunliteDevice.h SunliteFirmware.h \
                          SunliteFirmwareLoader.cpp SunliteFirmwareLoader.h \
                          SunliteOutputPort.cpp SunliteOutputPort.h \
                          UsbDevice.h UsbDmxPlugin.cpp UsbDmxPlugin.h \
                          VellemanDevice.cpp VellemanDevice.h \
                          VellemanOutputPort.cpp VellemanOutputPort.h
libolausbdmx_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
libolausbdmx_la_LIBADD = $(libusb_LIBS) \
                         ../../common/libolacommon.la
endif
