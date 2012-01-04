include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = FtdiDmxDevice.h FtdiDmxPlugin.h FtdiDmxPort.h FtdiDmxThread.h \
             FtdiWidget.h

if HAVE_LIBFTDI
  lib_LTLIBRARIES = libolaftdidmx.la
  libolaftdidmx_la_SOURCES = FtdiDmxDevice.cpp FtdiDmxPlugin.cpp \
                             FtdiDmxThread.cpp FtdiWidget-libftdi.cpp
  libolaftdidmx_la_LIBADD = -lftdi -lusb \
                            ../../common/libolacommon.la
endif

if HAVE_LIBFTD2XX
  lib_LTLIBRARIES = libolaftdidmx.la
  libolaftdidmx_la_SOURCES = FtdiDmxDevice.cpp FtdiDmxPlugin.cpp \
                             FtdiDmxThread.cpp FtdiWidget-ftd2xx.cpp
  libolaftdidmx_la_LIBADD = -lftd2xx \
                            ../../common/libolacommon.la
endif
