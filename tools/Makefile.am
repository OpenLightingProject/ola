INCLUDES = -I$(top_builddir)/common -I$(top_builddir)/include \
           -I$(top_builddir)/plugins @LLAD_DEFINES@

bin_PROGRAMS = usbpro_firmware
usbpro_firmware_SOURCES = usbpro-firmware.cpp
usbpro_firmware_CXXFLAGS = -Wall -Wformat -W
usbpro_firmware_LDFLAGS =
usbpro_firmware_LDADD = @LIBS@ -lprotobuf \
                        $(top_builddir)/common/libllacommon.la \
                        $(top_builddir)/plugins/usbpro/libllausbprowidget.la
