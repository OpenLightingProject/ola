dist_noinst_SCRIPTS += tools/usbpro/download_firmware.sh

bin_PROGRAMS += tools/usbpro/usbpro_firmware
tools_usbpro_usbpro_firmware_SOURCES = tools/usbpro/usbpro-firmware.cpp
tools_usbpro_usbpro_firmware_LDADD = common/libolacommon.la \
                                     plugins/usbpro/libolausbprowidget.la
