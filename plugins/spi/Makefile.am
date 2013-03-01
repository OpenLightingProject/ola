include $(top_srcdir)/common.mk

libdir = $(plugindir)
EXTRA_DIST = SPIPlugin.h SPIDevice.h SPIPort.h

if USE_SPI
lib_LTLIBRARIES = libolaspi.la
libolaspi_la_SOURCES = SPIPlugin.cpp SPIDevice.cpp SPIPort.cpp
libolaspi_la_LIBADD = ../../common/libolacommon.la
endif
