include $(top_srcdir)/common.mk

libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArduinoRGBDevice.h DMXter4Device.h DmxTriDevice.h DmxTriWidget.h \
             UsbDevice.h UsbProDevice.h UsbProPlugin.h UsbWidget.h \
             WidgetDetector.h

lib_LTLIBRARIES = libolausbpro.la
libolausbpro_la_SOURCES = UsbProPlugin.cpp ArduinoRGBDevice.cpp \
                          DmxTriDevice.cpp UsbProDevice.cpp DMXter4Device.cpp
libolausbpro_la_LIBADD = ./libolausbprowidget.la messages/libolausbproconf.la

noinst_LTLIBRARIES = libolausbprowidget.la
libolausbprowidget_la_SOURCES = DmxTriWidget.cpp UsbWidget.cpp \
                                WidgetDetector.cpp
