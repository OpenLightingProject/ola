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
TESTS = ArduinoWidgetTester BaseRobeWidgetTester BaseUsbProWidgetTester \
        DmxTriWidgetTester DmxterWidgetTester EnttecUsbProWidgetTester \
        RobeWidgetDetectorTester RobeWidgetTester UltraDMXProWidgetTester \
        UsbProWidgetDetectorTester WidgetDetectorThreadTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ./libolausbprowidget.la

common_test_sources = CommonWidgetTest.cpp \
                      MockEndpoint.cpp

ArduinoWidgetTester_SOURCES = ArduinoWidgetTest.cpp \
                              $(common_test_sources)
ArduinoWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ArduinoWidgetTester_LDADD = $(COMMON_TEST_LDADD)

BaseRobeWidgetTester_SOURCES = BaseRobeWidgetTest.cpp \
                               $(common_test_sources)
BaseRobeWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
BaseRobeWidgetTester_LDADD = $(COMMON_TEST_LDADD)

BaseUsbProWidgetTester_SOURCES = BaseUsbProWidgetTest.cpp \
                                 $(common_test_sources)
BaseUsbProWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
BaseUsbProWidgetTester_LDADD = $(COMMON_TEST_LDADD)

DmxTriWidgetTester_SOURCES = DmxTriWidgetTest.cpp \
                             $(common_test_sources)
DmxTriWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DmxTriWidgetTester_LDADD = $(COMMON_TEST_LDADD)

DmxterWidgetTester_SOURCES = DmxterWidgetTest.cpp \
                             $(common_test_sources)
DmxterWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DmxterWidgetTester_LDADD = $(COMMON_TEST_LDADD)

EnttecUsbProWidgetTester_SOURCES = EnttecUsbProWidgetTest.cpp \
                                   $(common_test_sources)
EnttecUsbProWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
EnttecUsbProWidgetTester_LDADD = $(COMMON_TEST_LDADD)

RobeWidgetDetectorTester_SOURCES = RobeWidgetDetectorTest.cpp \
                                   $(common_test_sources)
RobeWidgetDetectorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RobeWidgetDetectorTester_LDADD = $(COMMON_TEST_LDADD)

RobeWidgetTester_SOURCES = RobeWidgetTest.cpp \
                           $(common_test_sources)
RobeWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RobeWidgetTester_LDADD = $(COMMON_TEST_LDADD)

UltraDMXProWidgetTester_SOURCES = UltraDMXProWidgetTest.cpp \
                                  $(common_test_sources)
UltraDMXProWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
UltraDMXProWidgetTester_LDADD = $(COMMON_TEST_LDADD)

UsbProWidgetDetectorTester_SOURCES = UsbProWidgetDetectorTest.cpp \
                                     $(common_test_sources)
UsbProWidgetDetectorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
UsbProWidgetDetectorTester_LDADD = $(COMMON_TEST_LDADD)

WidgetDetectorThreadTester_SOURCES = WidgetDetectorThreadTest.cpp \
                                     $(common_test_sources)
WidgetDetectorThreadTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
WidgetDetectorThreadTester_LDADD = $(COMMON_TEST_LDADD)
endif
