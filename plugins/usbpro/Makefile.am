include $(top_srcdir)/common.mk

libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArduinoRGBDevice.h DmxTriDevice.h DmxTriWidget.h DmxterDevice.h \
             DmxterWidget.h MockUsbWidget.h UsbDevice.h UsbProDevice.h \
             UsbProPlugin.h UsbProWidget.h UsbWidget.h WidgetDetector.h

# This is a library which doesn't contain any coupling to olad
noinst_LTLIBRARIES = libolausbprowidget.la
libolausbprowidget_la_SOURCES = DmxTriWidget.cpp DmxterWidget.cpp \
                                UsbWidget.cpp UsbProWidget.cpp \
                                WidgetDetector.cpp

lib_LTLIBRARIES = libolausbpro.la
libolausbpro_la_SOURCES = UsbProPlugin.cpp ArduinoRGBDevice.cpp \
                          DmxTriDevice.cpp UsbProDevice.cpp DmxterDevice.cpp
libolausbpro_la_LIBADD = ./libolausbprowidget.la messages/libolausbproconf.la


# Test programs
TESTS = UsbProTester
check_PROGRAMS = $(TESTS)
UsbProTester_SOURCES = DmxTriWidgetTest.cpp \
                       DmxterWidgetTest.cpp \
                       MockUsbWidget.cpp \
                       UsbProTester.cpp \
                       UsbProWidgetTest.cpp \
                       UsbWidgetTest.cpp \
                       WidgetDetectorTest.cpp

UsbProTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
UsbProTester_LDFLAGS = $(CPPUNIT_LIBS)
UsbProTester_LDADD = -lprotobuf \
                     ./libolausbprowidget.la \
                     ../../common/libolacommon.la
