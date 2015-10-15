# LIBRARIES
##################################################
if USE_LIBUSB
noinst_LTLIBRARIES += libs/usb/libolausb.la

libs_usb_libolausb_la_SOURCES = \
    libs/usb/HotplugAgent.cpp \
    libs/usb/HotplugAgent.h \
    libs/usb/JaRuleConstants.h \
    libs/usb/JaRuleConstants.cpp \
    libs/usb/JaRulePortHandle.cpp \
    libs/usb/JaRulePortHandle.h \
    libs/usb/JaRulePortHandleImpl.cpp \
    libs/usb/JaRulePortHandleImpl.h \
    libs/usb/JaRuleWidget.cpp \
    libs/usb/JaRuleWidget.h \
    libs/usb/JaRuleWidgetPort.cpp \
    libs/usb/JaRuleWidgetPort.h \
    libs/usb/LibUsbAdaptor.cpp \
    libs/usb/LibUsbAdaptor.h \
    libs/usb/LibUsbThread.cpp \
    libs/usb/LibUsbThread.h \
    libs/usb/Types.cpp \
    libs/usb/Types.h
libs_usb_libolausb_la_CXXFLAGS = $(COMMON_CXXFLAGS) \
                                 $(libusb_CFLAGS)
libs_usb_libolausb_la_LIBADD = $(libusb_LIBS) \
                               common/libolacommon.la

# TESTS
##################################################
test_programs += libs/usb/LibUsbThreadTester

LIBS_USB_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                      $(libusb_LIBS) \
                      libs/usb/libolausb.la

libs_usb_LibUsbThreadTester_SOURCES = \
    libs/usb/LibUsbThreadTest.cpp
libs_usb_LibUsbThreadTester_CXXFLAGS = $(COMMON_TESTING_FLAGS) \
                                       $(libusb_CFLAGS)
libs_usb_LibUsbThreadTester_LDADD = $(LIBS_USB_TEST_LDADD)
endif
