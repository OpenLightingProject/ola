if BUILD_JA_RULE
if !USING_WIN32
if HAVE_LIBUSB_HOTPLUG_API
bin_PROGRAMS += tools/ja-rule/ja-rule \
                tools/ja-rule/ja-rule-controller
endif
endif
endif

tools_ja_rule_ja_rule_SOURCES = \
    tools/ja-rule/JaRuleEndpoint.h \
    tools/ja-rule/JaRuleEndpoint.cpp \
    tools/ja-rule/USBDeviceManager.cpp \
    tools/ja-rule/USBDeviceManager.h \
    tools/ja-rule/ja-rule.cpp
tools_ja_rule_ja_rule_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
tools_ja_rule_ja_rule_LDADD = $(libusb_LIBS) \
                              common/libolacommon.la \
                              plugins/usbdmx/libolausbdmxwidget.la

tools_ja_rule_ja_rule_controller_SOURCES = \
    tools/ja-rule/JaRuleEndpoint.cpp \
    tools/ja-rule/JaRuleEndpoint.h \
    tools/ja-rule/JaRuleWidget.cpp \
    tools/ja-rule/JaRuleWidget.h \
    tools/ja-rule/JaRuleWidgetImpl.cpp \
    tools/ja-rule/JaRuleWidgetImpl.h \
    tools/ja-rule/USBDeviceManager.cpp \
    tools/ja-rule/USBDeviceManager.h \
    tools/ja-rule/ja-rule-controller.cpp
tools_ja_rule_ja_rule_controller_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
tools_ja_rule_ja_rule_controller_LDADD = $(libusb_LIBS) \
                                         common/libolacommon.la \
                                         plugins/usbdmx/libolausbdmxwidget.la
