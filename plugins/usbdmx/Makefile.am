libdir = $(plugindir)
EXTRA_DIST = AnymaDevice.h AnymaOutputPort.h EuroliteProDevice.h \
             EuroliteProOutputPort.h FirmwareLoader.h SunliteDevice.h \
             SunliteFirmware.h SunliteFirmwareLoader.h SunliteOutputPort.h \
             UsbDmxPlugin.h UsbDevice.h VellemanDevice.h VellemanOutputPort.h

if HAVE_LIBUSB
  lib_LTLIBRARIES = libolausbdmx.la
  libolausbdmx_la_SOURCES = AnymaDevice.cpp AnymaOutputPort.cpp \
                            EuroliteProDevice.cpp EuroliteProOutputPort.cpp \
                            SunliteDevice.cpp SunliteFirmwareLoader.cpp \
                            SunliteOutputPort.cpp \
                            UsbDmxPlugin.cpp VellemanDevice.cpp \
                            VellemanOutputPort.cpp
  libolausbdmx_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
  libolausbdmx_la_LIBADD = $(libusb_LIBS) \
                           ../../common/libolacommon.la
endif
