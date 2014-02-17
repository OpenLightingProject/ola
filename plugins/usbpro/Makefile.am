include $(top_srcdir)/common.mk

SUBDIRS = messages

# LIBRARIES
##################################################
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES = libolausbprowidget.la
libolausbprowidget_la_SOURCES = ArduinoWidget.cpp ArduinoWidget.h \
                                BaseRobeWidget.cpp BaseRobeWidget.h \
                                BaseUsbProWidget.cpp BaseUsbProWidget.h \
                                DmxTriWidget.cpp DmxTriWidget.h \
                                DmxterWidget.cpp DmxterWidget.h \
                                EnttecUsbProWidget.cpp EnttecUsbProWidget.h \
                                EnttecUsbProWidgetImpl.h \
                                GenericUsbProWidget.cpp GenericUsbProWidget.h \
                                RobeWidget.cpp RobeWidget.h \
                                RobeWidgetDetector.cpp RobeWidgetDetector.h \
                                SerialWidgetInterface.h \
                                SerialWidgetInterface.h UltraDMXProWidget.cpp \
                                UltraDMXProWidget.h UsbProWidgetDetector.cpp \
                                UsbProWidgetDetector.h \
                                WidgetDetectorInterface.h \
                                WidgetDetectorThread.cpp WidgetDetectorThread.h
libolausbprowidget_la_LIBADD = ../../common/libolacommon.la

if USE_USBPRO
# The OLA USB Pro Plugin
plugin_LTLIBRARIES = libolausbpro.la
libolausbpro_la_SOURCES = ArduinoRGBDevice.cpp ArduinoRGBDevice.h \
                          DmxTriDevice.cpp DmxTriDevice.h DmxterDevice.cpp \
                          DmxterDevice.h RobeDevice.cpp RobeDevice.h \
                          UltraDMXProDevice.cpp UltraDMXProDevice.h \
                          UsbProDevice.cpp UsbProDevice.h UsbSerialDevice.h \
                          UsbSerialPlugin.cpp UsbSerialPlugin.h
libolausbpro_la_LIBADD = libolausbprowidget.la \
                         messages/libolausbproconf.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = ArduinoWidgetTester BaseRobeWidgetTester BaseUsbProWidgetTester \
        DmxTriWidgetTester DmxterWidgetTester EnttecUsbProWidgetTester \
        RobeWidgetDetectorTester RobeWidgetTester UltraDMXProWidgetTester \
        UsbProWidgetDetectorTester WidgetDetectorThreadTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ./libolausbprowidget.la

common_test_sources = CommonWidgetTest.cpp CommonWidgetTest.h \
                      MockEndpoint.cpp MockEndpoint.h

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
