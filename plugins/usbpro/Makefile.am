include $(top_srcdir)/common.mk

libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArduinoRGBDevice.h ArduinoWidget.h DmxTriDevice.h DmxTriWidget.h \
             BaseUsbProUsbWidget.h DmxterDevice.h DmxterWidget.h \
             MockUsbWidget.h RobeDevice.h RobeWidget.h RobeWidgetDetector.h \
             UsbDevice.h UsbProDevice.h UsbProPlugin.h EnttecUsbProWidget.h \
             UsbProWidgetDetector.h UsbWidgetInterface.h \
             WidgetDetectorInterface.h WidgetDetectorThread.h

# This is a library which doesn't contain any coupling to olad
noinst_LTLIBRARIES = libolausbprowidget.la
libolausbprowidget_la_SOURCES = ArduinoWidget.cpp \
                                DmxTriWidget.cpp \
                                BaseUsbProWidget.cpp \
                                DmxterWidget.cpp \
                                RobeDevice.cpp \
                                RobeWidget.cpp \
                                RobeWidgetDetector.cpp \
                                EnttecUsbProWidget.cpp \
                                UsbProWidgetDetector.cpp \
                                WidgetDetectorThread.cpp

lib_LTLIBRARIES = libolausbpro.la
libolausbpro_la_SOURCES = ArduinoRGBDevice.cpp DmxTriDevice.cpp \
                          DmxterDevice.cpp UsbProDevice.cpp \
                          UsbProPlugin.cpp
libolausbpro_la_LIBADD = ./libolausbprowidget.la messages/libolausbproconf.la


# Test programs
TESTS = UsbProTester
check_PROGRAMS = $(TESTS)
UsbProTester_SOURCES = ArduinoWidgetTest.cpp \
                       DmxTriWidgetTest.cpp \
                       DmxterWidgetTest.cpp \
                       MockUsbWidget.cpp \
                       RobeWidgetTest.cpp \
                       UsbProTester.cpp \
                       UsbProWidgetDetectorTest.cpp \
                       UsbProWidgetTest.cpp \
                       UsbWidgetTest.cpp

UsbProTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
UsbProTester_LDFLAGS = $(CPPUNIT_LIBS) $(PROTOBUF_LIBS)
UsbProTester_LDADD = ./libolausbprowidget.la \
                     ../../common/libolacommon.la
