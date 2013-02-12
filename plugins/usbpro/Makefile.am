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

if USE_USBPRO
# The OLA USB Pro Plugin
lib_LTLIBRARIES = libolausbpro.la
libolausbpro_la_SOURCES = ArduinoRGBDevice.cpp DmxTriDevice.cpp \
                          DmxterDevice.cpp UsbProDevice.cpp \
                          UsbSerialPlugin.cpp
libolausbpro_la_LIBADD = libolausbprowidget.la \
                         messages/libolausbproconf.la


# Test programs
if BUILD_TESTS
TESTS = ArduinoWidgetTester DmxTriWidgetTester DmxterWidgetTester \
        EnttecUsbProWidgetTester RobeWidgetTester SerialUsbTester \
        UltraDMXProWidgetTester
endif
check_PROGRAMS = $(TESTS)

common_test_sources = CommonWidgetTest.cpp \
                      GenericTester.cpp \
                      MockEndpoint.cpp

ArduinoWidgetTester_SOURCES = ArduinoWidgetTest.cpp \
                              $(common_test_sources)
ArduinoWidgetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ArduinoWidgetTester_LDADD = $(CPPUNIT_LIBS) ./libolausbprowidget.la

DmxTriWidgetTester_SOURCES = DmxTriWidgetTest.cpp \
                             $(common_test_sources)
DmxTriWidgetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DmxTriWidgetTester_LDADD = $(CPPUNIT_LIBS) ./libolausbprowidget.la

DmxterWidgetTester_SOURCES = DmxterWidgetTest.cpp \
                             $(common_test_sources)
DmxterWidgetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DmxterWidgetTester_LDADD = $(CPPUNIT_LIBS) ./libolausbprowidget.la

EnttecUsbProWidgetTester_SOURCES = EnttecUsbProWidgetTest.cpp \
                                   $(common_test_sources)
EnttecUsbProWidgetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
EnttecUsbProWidgetTester_LDADD = $(CPPUNIT_LIBS) ./libolausbprowidget.la

RobeWidgetTester_SOURCES = BaseRobeWidgetTest.cpp \
                           RobeWidgetDetectorTest.cpp \
                           RobeWidgetTest.cpp \
                           $(common_test_sources)
RobeWidgetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
RobeWidgetTester_LDADD = $(CPPUNIT_LIBS) ./libolausbprowidget.la

UltraDMXProWidgetTester_SOURCES = UltraDMXProWidgetTest.cpp \
                                  $(common_test_sources)
UltraDMXProWidgetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
UltraDMXProWidgetTester_LDADD = $(CPPUNIT_LIBS) ./libolausbprowidget.la

SerialUsbTester_SOURCES = BaseUsbProWidgetTest.cpp \
                          UsbProWidgetDetectorTest.cpp \
                          WidgetDetectorThreadTest.cpp \
                          $(common_test_sources)
SerialUsbTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SerialUsbTester_LDADD = $(CPPUNIT_LIBS) ./libolausbprowidget.la
endif
