include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_UART
plugin_LTLIBRARIES = libolauartdmx.la
libolauartdmx_la_SOURCES = UartDmxDevice.cpp UartDmxDevice.h \
                           UartDmxPlugin.cpp UartDmxPlugin.h UartDmxPort.h \
                           UartDmxThread.cpp UartDmxThread.h \
                           UartWidget.cpp UartWidget.h UartLinuxHelper.h \
                           UartLinuxHelper.cpp
libolauartdmx_la_LIBADD = ../../common/libolacommon.la
endif

