include $(top_srcdir)/common.mk

bin_PROGRAMS = usbpro_firmware
usbpro_firmware_SOURCES = usbpro-firmware.cpp
usbpro_firmware_LDFLAGS =
usbpro_firmware_LDADD = @LIBS@ -lprotobuf \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/usbpro/libolausbprowidget.la
