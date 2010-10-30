include $(top_srcdir)/common.mk

libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = UsbProPlugin.h UsbDevice.h UsbWidget.h \
             WidgetDetector.h ArduinoRGBDevice.h DmxTriDevice.h \
             DmxTriWidget.h UsbProDevice.h

lib_LTLIBRARIES = libolausbpro.la
libolausbpro_la_SOURCES = UsbProPlugin.cpp ArduinoRGBDevice.cpp \
                          DmxTriDevice.cpp UsbProDevice.cpp
libolausbpro_la_LIBADD = ./libolausbprowidget.la messages/libolausbproconf.la

noinst_LTLIBRARIES = libolausbprowidget.la
libolausbprowidget_la_SOURCES = DmxTriWidget.cpp UsbWidget.cpp \
                                WidgetDetector.cpp
