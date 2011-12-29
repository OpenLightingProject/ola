include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = FtdiDmxDevice.h FtdiDmxPlugin.h FtdiUsbDevice.h FtdiDmxDevice.h FtdiDmxPort.h FtdiDmxThread.h

if HAVE_LIBFTDI
  lib_LTLIBRARIES = libolaftdidmx.la

  libolaftdidmx_la_SOURCES =  FtdiUsbDevice-libftdi.cpp FtdiDmxPlugin.cpp FtdiDmxDevice.cpp FtdiDmxThread.cpp
  libolaftdidmx_la_LIBADD = -lftdi -lusb \
                            ../../common/libolacommon.la
endif