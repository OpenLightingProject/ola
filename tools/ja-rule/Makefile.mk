if BUILD_JA_RULE
bin_PROGRAMS += tools/ja-rule/ja-rule \
                tools/ja-rule/ja-rule-controller
endif

tools_ja_rule_ja_rule_SOURCES = \
    tools/ja-rule/USBDeviceManager.cpp \
    tools/ja-rule/USBDeviceManager.h \
    tools/ja-rule/ja-rule.cpp
tools_ja_rule_ja_rule_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
tools_ja_rule_ja_rule_LDADD = $(libusb_LIBS) \
                              common/libolacommon.la \
                              plugins/usbdmx/libolausbdmxwidget.la \
                              libs/usb/libolausb.la

tools_ja_rule_ja_rule_controller_SOURCES = \
    tools/ja-rule/USBDeviceManager.cpp \
    tools/ja-rule/USBDeviceManager.h \
    tools/ja-rule/ja-rule-controller.cpp
tools_ja_rule_ja_rule_controller_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
tools_ja_rule_ja_rule_controller_LDADD = $(libusb_LIBS) \
                                         common/libolacommon.la \
                                         plugins/usbdmx/libolausbdmxwidget.la \
                                         libs/usb/libolausb.la
