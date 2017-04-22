# LIBRARIES
##################################################
if USE_UART
lib_LTLIBRARIES += plugins/uartdmx/libolauartdmx.la

# Plugin description is generated from README.md
built_sources += plugins/uartdmx/UartDmxPluginDescription.h
nodist_plugins_uartdmx_libolauartdmx_la_SOURCES = \
    plugins/uartdmx/UartDmxPluginDescription.h
plugins/uartdmx/UartDmxPluginDescription.h: plugins/uartdmx/README.md plugins/uartdmx/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/uartdmx $(top_builddir)/plugins/uartdmx/UartDmxPluginDescription.h

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

EXTRA_DIST += plugins/uartdmx/README.md
