include plugins/artnet/Makefile.mk
include plugins/dummy/Makefile.mk
include plugins/espnet/Makefile.mk
include plugins/ftdidmx/Makefile.mk
include plugins/gpio/Makefile.mk
include plugins/karate/Makefile.mk
include plugins/kinet/Makefile.mk
include plugins/milinst/Makefile.mk
include plugins/opendmx/Makefile.mk
include plugins/openpixelcontrol/Makefile.mk
include plugins/osc/Makefile.mk
include plugins/pathport/Makefile.mk
include plugins/renard/Makefile.mk
include plugins/sandnet/Makefile.mk
include plugins/shownet/Makefile.mk
include plugins/spi/Makefile.mk
include plugins/stageprofi/Makefile.mk
include plugins/usbdmx/Makefile.mk

if !USING_WIN32
include plugins/usbpro/Makefile.mk
include plugins/dmx4linux/Makefile.mk
include plugins/e131/Makefile.mk
include plugins/uartdmx/Makefile.mk
endif

dist_noinst_SCRIPTS += plugins/convert_README_to_header.sh
