# LIBRARIES
##################################################
if USE_UART
lib_LTLIBRARIES += plugins/uartdmx/libolauartdmx.la
plugins_uartdmx_libolauartdmx_la_SOURCES = \
    plugins/uartdmx/UartDmxDevice.cpp \
    plugins/uartdmx/UartDmxDevice.h \
    plugins/uartdmx/UartDmxPlugin.cpp \
    plugins/uartdmx/UartDmxPlugin.h \
    plugins/uartdmx/UartDmxPort.h \
    plugins/uartdmx/UartDmxThread.cpp \
    plugins/uartdmx/UartDmxThread.h \
    plugins/uartdmx/UartWidget.cpp \
    plugins/uartdmx/UartWidget.h
plugins_uartdmx_libolauartdmx_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

