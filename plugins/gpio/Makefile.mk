# LIBRARIES
##################################################
if USE_GPIO
lib_LTLIBRARIES += plugins/gpio/libolagpiocore.la \
                   plugins/gpio/libolagpio.la

# This is a library which isn't coupled to olad
plugins_gpio_libolagpiocore_la_SOURCES = \
    plugins/gpio/GpioDriver.cpp \
    plugins/gpio/GpioDriver.h
plugins_gpio_libolagpiocore_la_LIBADD = common/libolacommon.la

# Plugin description is generated from README.md
built_sources += plugins/gpio/GpioPluginDescription.h
nodist_plugins_gpio_libolagpio_la_SOURCES = \
    plugins/gpio/GpioPluginDescription.h
plugins/gpio/GpioPluginDescription.h: plugins/gpio/README.md plugins/gpio/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/gpio $(top_builddir)/plugins/gpio/GpioPluginDescription.h

plugins_gpio_libolagpio_la_SOURCES = \
    plugins/gpio/GpioDevice.cpp \
    plugins/gpio/GpioDevice.h \
    plugins/gpio/GpioPlugin.cpp \
    plugins/gpio/GpioPlugin.h \
    plugins/gpio/GpioPort.cpp \
    plugins/gpio/GpioPort.h
plugins_gpio_libolagpio_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/gpio/libolagpiocore.la
endif

EXTRA_DIST += plugins/gpio/README.md
