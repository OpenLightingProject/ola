include $(top_srcdir)/common.mk

libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArduinoRGBDevice.h ArduinoWidget.h BaseUsbProWidget.h \
             BaseRobeWidget.h CommonWidgetTest.h DmxTriDevice.h \
             DmxTriWidget.h DmxterDevice.h DmxterWidget.h \
             EnttecUsbProWidget.h EnttecUsbProWidgetImpl.h \
             GenericUsbProWidget.h MockEndpoint.h \
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
                      MockEndpoint.cpp

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    $(top_builddir)/common/testing/libtestmain.la

ArduinoWidgetTester_SOURCES = ArduinoWidgetTest.cpp \
                              $(common_test_sources)
ArduinoWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ArduinoWidgetTester_LDADD = $(COMMON_TEST_LDADD) ./libolausbprowidget.la

DmxTriWidgetTester_SOURCES = DmxTriWidgetTest.cpp \
                             $(common_test_sources)
DmxTriWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DmxTriWidgetTester_LDADD = $(COMMON_TEST_LDADD) ./libolausbprowidget.la

DmxterWidgetTester_SOURCES = DmxterWidgetTest.cpp \
                             $(common_test_sources)
DmxterWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DmxterWidgetTester_LDADD = $(COMMON_TEST_LDADD) ./libolausbprowidget.la

EnttecUsbProWidgetTester_SOURCES = EnttecUsbProWidgetTest.cpp \
                                   $(common_test_sources)
EnttecUsbProWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
EnttecUsbProWidgetTester_LDADD = $(COMMON_TEST_LDADD) ./libolausbprowidget.la

RobeWidgetTester_SOURCES = BaseRobeWidgetTest.cpp \
                           RobeWidgetDetectorTest.cpp \
                           RobeWidgetTest.cpp \
                           $(common_test_sources)
RobeWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RobeWidgetTester_LDADD = $(COMMON_TEST_LDADD) ./libolausbprowidget.la

UltraDMXProWidgetTester_SOURCES = UltraDMXProWidgetTest.cpp \
                                  $(common_test_sources)
UltraDMXProWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
UltraDMXProWidgetTester_LDADD = $(COMMON_TEST_LDADD) ./libolausbprowidget.la

SerialUsbTester_SOURCES = BaseUsbProWidgetTest.cpp \
                          UsbProWidgetDetectorTest.cpp \
                          WidgetDetectorThreadTest.cpp \
                          $(common_test_sources)
SerialUsbTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SerialUsbTester_LDADD = $(COMMON_TEST_LDADD) ./libolausbprowidget.la
endif
