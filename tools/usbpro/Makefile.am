include $(top_srcdir)/common.mk

EXTRA_DIST = download_firmware.sh

noinst_PROGRAMS = usbpro_firmware
usbpro_firmware_SOURCES = usbpro-firmware.cpp
usbpro_firmware_LDADD = $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/usbpro/libolausbprowidget.la
