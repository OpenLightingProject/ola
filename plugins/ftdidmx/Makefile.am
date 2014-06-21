include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_FTDI
plugin_LTLIBRARIES = libolaftdidmx.la
libolaftdidmx_la_SOURCES = FtdiDmxDevice.cpp FtdiDmxDevice.h \
                           FtdiDmxPlugin.cpp FtdiDmxPlugin.h FtdiDmxPort.h \
                           FtdiDmxThread.cpp FtdiDmxThread.h \
                           FtdiWidget-libftdi.cpp FtdiWidget.h
libolaftdidmx_la_LIBADD = $(libftdi_LIBS) \
                          ../../common/libolacommon.la
endif

# This isn't used yet.
#if HAVE_LIBFTD2XX
#  lib_LTLIBRARIES = libolaftdidmx.la
#  libolaftdidmx_la_SOURCES = FtdiDmxDevice.cpp FtdiDmxPlugin.cpp \
#                             FtdiDmxThread.cpp FtdiWidget-ftd2xx.cpp
#  libolaftdidmx_la_LIBADD = -lftd2xx \
#                            ../../common/libolacommon.la
#endif
