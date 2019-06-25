include plugins/usbpro/messages/Makefile.mk

# LIBRARIES
##################################################
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/usbpro/libolausbprowidget.la
plugins_usbpro_libolausbprowidget_la_SOURCES = \
    plugins/usbpro/ArduinoWidget.cpp \
    plugins/usbpro/ArduinoWidget.h \
    plugins/usbpro/BaseRobeWidget.cpp \
    plugins/usbpro/BaseRobeWidget.h \
    plugins/usbpro/BaseUsbProWidget.cpp \
    plugins/usbpro/BaseUsbProWidget.h \
    plugins/usbpro/DmxTriWidget.cpp \
    plugins/usbpro/DmxTriWidget.h \
    plugins/usbpro/DmxterWidget.cpp \
    plugins/usbpro/DmxterWidget.h \
    plugins/usbpro/DMXUSBWidget.cpp \
    plugins/usbpro/DMXUSBWidget.h \
    plugins/usbpro/EnttecUsbProWidget.cpp \
    plugins/usbpro/EnttecUsbProWidget.h \
    plugins/usbpro/EnttecUsbProWidgetImpl.h \
    plugins/usbpro/GenericUsbProWidget.cpp \
    plugins/usbpro/GenericUsbProWidget.h \
    plugins/usbpro/RobeWidget.cpp \
    plugins/usbpro/RobeWidget.h \
    plugins/usbpro/RobeWidgetDetector.cpp \
    plugins/usbpro/RobeWidgetDetector.h \
    plugins/usbpro/SerialWidgetInterface.h \
    plugins/usbpro/SerialWidgetInterface.h \
    plugins/usbpro/UltraDMXProWidget.cpp \
    plugins/usbpro/UltraDMXProWidget.h \
    plugins/usbpro/UsbProWidgetDetector.cpp \
    plugins/usbpro/UsbProWidgetDetector.h \
    plugins/usbpro/WidgetDetectorInterface.h \
    plugins/usbpro/WidgetDetectorThread.cpp \
    plugins/usbpro/WidgetDetectorThread.h
plugins_usbpro_libolausbprowidget_la_LIBADD = common/libolacommon.la

if USE_USBPRO
# The OLA USB Pro Plugin
lib_LTLIBRARIES += plugins/usbpro/libolausbpro.la

# Plugin description is generated from README.md
built_sources += plugins/usbpro/UsbSerialPluginDescription.h
nodist_plugins_usbpro_libolausbpro_la_SOURCES = \
    plugins/usbpro/UsbSerialPluginDescription.h
plugins/usbpro/UsbSerialPluginDescription.h: plugins/usbpro/README.md plugins/usbpro/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/usbpro $(top_builddir)/plugins/usbpro/UsbSerialPluginDescription.h

plugins_usbpro_libolausbpro_la_SOURCES = \
    plugins/usbpro/ArduinoRGBDevice.cpp \
    plugins/usbpro/ArduinoRGBDevice.h \
    plugins/usbpro/DmxTriDevice.cpp \
    plugins/usbpro/DmxTriDevice.h \
    plugins/usbpro/DmxterDevice.cpp \
    plugins/usbpro/DmxterDevice.h \
    plugins/usbpro/DMXUSBDevice.cpp \
    plugins/usbpro/DMXUSBDevice.h \
    plugins/usbpro/RobeDevice.cpp \
    plugins/usbpro/RobeDevice.h \
    plugins/usbpro/UltraDMXProDevice.cpp \
    plugins/usbpro/UltraDMXProDevice.h \
    plugins/usbpro/UsbProDevice.cpp \
    plugins/usbpro/UsbProDevice.h \
    plugins/usbpro/UsbSerialDevice.h \
    plugins/usbpro/UsbSerialPlugin.cpp \
    plugins/usbpro/UsbSerialPlugin.h
plugins_usbpro_libolausbpro_la_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
plugins_usbpro_libolausbpro_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/usbpro/libolausbprowidget.la \
    plugins/usbpro/messages/libolausbproconf.la

# TESTS
##################################################
test_programs += \
    plugins/usbpro/ArduinoWidgetTester \
    plugins/usbpro/BaseRobeWidgetTester \
    plugins/usbpro/BaseUsbProWidgetTester \
    plugins/usbpro/DmxTriWidgetTester \
    plugins/usbpro/DmxterWidgetTester \
    plugins/usbpro/DMXUSBWidgetTester \
    plugins/usbpro/EnttecUsbProWidgetTester \
    plugins/usbpro/RobeWidgetDetectorTester \
    plugins/usbpro/RobeWidgetTester \
    plugins/usbpro/UltraDMXProWidgetTester \
    plugins/usbpro/UsbProWidgetDetectorTester \
    plugins/usbpro/WidgetDetectorThreadTester

COMMON_USBPRO_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    plugins/usbpro/libolausbprowidget.la

common_test_sources = \
    plugins/usbpro/CommonWidgetTest.cpp \
    plugins/usbpro/CommonWidgetTest.h \
    plugins/usbpro/MockEndpoint.cpp \
    plugins/usbpro/MockEndpoint.h

plugins_usbpro_ArduinoWidgetTester_SOURCES = \
    plugins/usbpro/ArduinoWidgetTest.cpp \
    $(common_test_sources)
plugins_usbpro_ArduinoWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_ArduinoWidgetTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_BaseRobeWidgetTester_SOURCES = \
    plugins/usbpro/BaseRobeWidgetTest.cpp \
    $(common_test_sources)
plugins_usbpro_BaseRobeWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_BaseRobeWidgetTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_BaseUsbProWidgetTester_SOURCES = \
    plugins/usbpro/BaseUsbProWidgetTest.cpp \
    $(common_test_sources)
plugins_usbpro_BaseUsbProWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_BaseUsbProWidgetTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_DmxTriWidgetTester_SOURCES = \
    plugins/usbpro/DmxTriWidgetTest.cpp \
    $(common_test_sources)
plugins_usbpro_DmxTriWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_DmxTriWidgetTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_DmxterWidgetTester_SOURCES = \
    plugins/usbpro/DmxterWidgetTest.cpp \
    $(common_test_sources)
plugins_usbpro_DmxterWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_DmxterWidgetTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_DMXUSBWidgetTester_SOURCES = \
    plugins/usbpro/DMXUSBWidgetTest.cpp \
    $(common_test_sources)
plugins_usbpro_DMXUSBWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_DMXUSBWidgetTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_EnttecUsbProWidgetTester_SOURCES = \
    plugins/usbpro/EnttecUsbProWidgetTest.cpp \
    $(common_test_sources)
plugins_usbpro_EnttecUsbProWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_EnttecUsbProWidgetTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_RobeWidgetDetectorTester_SOURCES = \
    plugins/usbpro/RobeWidgetDetectorTest.cpp \
    $(common_test_sources)
plugins_usbpro_RobeWidgetDetectorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_RobeWidgetDetectorTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_RobeWidgetTester_SOURCES = \
    plugins/usbpro/RobeWidgetTest.cpp \
    $(common_test_sources)
plugins_usbpro_RobeWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_RobeWidgetTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_UltraDMXProWidgetTester_SOURCES = \
    plugins/usbpro/UltraDMXProWidgetTest.cpp \
    $(common_test_sources)
plugins_usbpro_UltraDMXProWidgetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_UltraDMXProWidgetTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_UsbProWidgetDetectorTester_SOURCES = \
    plugins/usbpro/UsbProWidgetDetectorTest.cpp \
    $(common_test_sources)
plugins_usbpro_UsbProWidgetDetectorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_UsbProWidgetDetectorTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)

plugins_usbpro_WidgetDetectorThreadTester_SOURCES = \
    plugins/usbpro/WidgetDetectorThreadTest.cpp \
    $(common_test_sources)
plugins_usbpro_WidgetDetectorThreadTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_usbpro_WidgetDetectorThreadTester_LDADD = $(COMMON_USBPRO_TEST_LDADD)
endif

EXTRA_DIST += plugins/usbpro/README.md
