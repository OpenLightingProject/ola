include $(top_srcdir)/common.mk

libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArduinoRGBDevice.h ArduinoWidget.h BaseUsbProWidget.h \
             BaseRobeWidget.h CommonWidgetTest.h DmxTriDevice.h \
             DmxTriWidget.h DmxterDevice.h DmxterWidget.h \
             EnttecUsbProWidget.h GenericUsbProWidget.h MockEndpoint.h \
             RobeDevice.h RobeWidget.h RobeWidgetDetector.h \
             SerialWidgetInterface.h SerialWidgetInterface.h \
             UltraDMXProDevice.h UltraDMXProWidget.h \
             UsbSerialDevice.h UsbProDevice.h \
             UsbProWidgetDetector.h UsbSerialPlugin.h \
             WidgetDetectorInterface.h WidgetDetectorThread.h

# This is a library which doesn't contain any coupling to olad
noinst_LTLIBRARIES = libolausbprowidget.la
libolausbprowidget_la_SOURCES = ArduinoWidget.cpp \
                                DmxTriWidget.cpp \
                                BaseRobeWidget.cpp \
                                BaseUsbProWidget.cpp \
                                DmxterWidget.cpp \
                                GenericUsbProWidget.cpp \
                                RobeDevice.cpp \
                                RobeWidget.cpp \
                                RobeWidgetDetector.cpp \
                                EnttecUsbProWidget.cpp \
                                UltraDMXProDevice.cpp \
                                UltraDMXProWidget.cpp \
                                UsbProWidgetDetector.cpp \
                                WidgetDetectorThread.cpp
libolausbprowidget_la_LIBADD = ../../common/libolacommon.la

# The OLA USB Pro Plugin
lib_LTLIBRARIES = libolausbpro.la
libolausbpro_la_SOURCES = ArduinoRGBDevice.cpp DmxTriDevice.cpp \
                          DmxterDevice.cpp UsbProDevice.cpp \
                          UsbSerialPlugin.cpp
libolausbpro_la_LIBADD = ./libolausbprowidget.la \
                         messages/libolausbproconf.la


# Test programs
TESTS = RobeWidgetTester SerialUsbTester
check_PROGRAMS = $(TESTS)
SerialUsbTester_SOURCES = ArduinoWidgetTest.cpp \
                          BaseUsbProWidgetTest.cpp \
                          CommonWidgetTest.cpp \
                          DmxTriWidgetTest.cpp \
                          DmxterWidgetTest.cpp \
                          EnttecUsbProWidgetTest.cpp \
                          MockEndpoint.cpp \
                          SerialUsbTester.cpp \
                          UsbProWidgetDetectorTest.cpp \
                          UltraDMXProWidgetTest.cpp \
                          WidgetDetectorThreadTest.cpp

SerialUsbTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SerialUsbTester_LDADD = $(CPPUNIT_LIBS) ./libolausbprowidget.la

RobeWidgetTester_SOURCES = BaseRobeWidgetTest.cpp \
                           CommonWidgetTest.cpp \
                           MockEndpoint.cpp \
                           RobeWidgetDetectorTest.cpp \
                           RobeWidgetTest.cpp \
                           RobeWidgetTester.cpp
RobeWidgetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
RobeWidgetTester_LDADD = $(CPPUNIT_LIBS) ./libolausbprowidget.la
