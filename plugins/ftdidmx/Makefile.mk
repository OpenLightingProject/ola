# LIBRARIES
##################################################
if USE_FTDI
lib_LTLIBRARIES += plugins/ftdidmx/libolaftdidmx.la
plugins_ftdidmx_libolaftdidmx_la_SOURCES = \
    plugins/ftdidmx/FtdiDmxDevice.cpp \
    plugins/ftdidmx/FtdiDmxDevice.h \
    plugins/ftdidmx/FtdiDmxPlugin.cpp \
    plugins/ftdidmx/FtdiDmxPlugin.h \
    plugins/ftdidmx/FtdiDmxPort.h \
    plugins/ftdidmx/FtdiDmxThread.cpp \
    plugins/ftdidmx/FtdiDmxThread.h \
    plugins/ftdidmx/FtdiWidget-libftdi.cpp \
    plugins/ftdidmx/FtdiWidget.h
plugins_ftdidmx_libolaftdidmx_la_LIBADD = $(libftdi_LIBS) \
                                          common/libolacommon.la
endif

# This isn't used yet.
#if HAVE_LIBFTD2XX
#  lib_LTLIBRARIES = plugins/ftdidmx/libolaftdidmx.la
#  libolaftdidmx_la_SOURCES = FtdiDmxDevice.cpp FtdiDmxPlugin.cpp \
#                             FtdiDmxThread.cpp FtdiWidget-ftd2xx.cpp
#  libolaftdidmx_la_LIBADD = -lftd2xx \
#                            ../../common/libolacommon.la
#endif
