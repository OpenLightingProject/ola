# LIBRARIES
##################################################
if USE_GPIO
lib_LTLIBRARIES += plugins/gpio/libolagpiocore.la \
                   plugins/gpio/libolagpio.la

# This is a library which isn't coupled to olad
plugins_gpio_libolagpiocore_la_SOURCES = \
    plugins/gpio/GPIODriver.cpp \
    plugins/gpio/GPIODriver.h
plugins_gpio_libolagpiocore_la_LIBADD = common/libolacommon.la

plugins_gpio_libolagpio_la_SOURCES = \
    plugins/gpio/GPIODevice.cpp \
    plugins/gpio/GPIODevice.h \
    plugins/gpio/GPIOPlugin.cpp \
    plugins/gpio/GPIOPlugin.h \
    plugins/gpio/GPIOPort.cpp \
    plugins/gpio/GPIOPort.h
plugins_gpio_libolagpio_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/gpio/libolagpiocore.la
endif
