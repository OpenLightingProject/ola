include $(top_srcdir)/common.mk

libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArduinoRGBDevice.h ArduinoWidget.h DmxTriDevice.h DmxTriWidget.h \
             DmxterDevice.h DmxterWidget.h MockUsbWidget.h UsbDevice.h \
             UsbProDevice.h UsbProPlugin.h UsbProWidget.h UsbWidget.h \
             WidgetDetector.h WidgetDetectorThread.h

# This is a library which doesn't contain any coupling to olad
noinst_LTLIBRARIES = libolausbprowidget.la
libolausbprowidget_la_SOURCES = ArduinoWidget.cpp DmxTriWidget.cpp \
                                DmxterWidget.cpp UsbProWidget.cpp \
                                UsbWidget.cpp WidgetDetector.cpp \
                                WidgetDetectorThread.cpp
libolausbprowidget_la_LIBADD = ../../common/libolacommon.la

# The OLA USB Pro Plugin
lib_LTLIBRARIES = libolausbpro.la
libolausbpro_la_SOURCES = ArduinoRGBDevice.cpp DmxTriDevice.cpp \
                          DmxterDevice.cpp UsbProDevice.cpp \
                          UsbProPlugin.cpp
libolausbpro_la_LIBADD = ./libolausbprowidget.la \
                         messages/libolausbproconf.la


# Test programs
check_PROGRAMS = UsbProTester
UsbProTester_SOURCES = ArduinoWidgetTest.cpp \
                       DmxTriWidgetTest.cpp \
                       DmxterWidgetTest.cpp \
                       MockUsbWidget.cpp \
                       UsbProTester.cpp \
                       UsbProWidgetTest.cpp \
                       UsbWidgetTest.cpp \
                       WidgetDetectorTest.cpp
UsbProTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
UsbProTester_LDADD = $(CPPUNIT_LIBS) \
                     ./libolausbprowidget.la
