include $(top_srcdir)/common.mk

bin_PROGRAMS = usbpro_rdm_sniffer usbpro_firmware
usbpro_firmware_SOURCES = usbpro-firmware.cpp
usbpro_firmware_LDFLAGS =
usbpro_firmware_LDADD = @LIBS@ -lprotobuf \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/usbpro/libolausbprowidget.la

usbpro_rdm_sniffer_SOURCES = usbpro-rdm.cpp
usbpro_rdm_sniffer_LDFLAGS =
usbpro_rdm_sniffer_LDADD = @LIBS@ -lprotobuf \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/usbpro/libolausbprowidget.la
