if BUILD_JA_RULE
if !USING_WIN32
bin_PROGRAMS += tools/ja-rule/ja-rule
endif
endif

tools_ja_rule_ja_rule_SOURCES = \
    tools/ja-rule/OpenLightingDevice.h \
    tools/ja-rule/OpenLightingDevice.cpp \
    tools/ja-rule/USBDeviceManager.cpp \
    tools/ja-rule/USBDeviceManager.h \
    tools/ja-rule/ja-rule.cpp
tools_ja_rule_ja_rule_CXXFLAGS = $(COMMON_CXXFLAGS) $(libusb_CFLAGS)
tools_ja_rule_ja_rule_LDADD = $(libusb_CFLAGS) \
                              common/libolacommon.la \
                              plugins/usbdmx/libolausbdmxwidget.la
