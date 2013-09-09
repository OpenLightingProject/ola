include $(top_srcdir)/common.mk

libdir = $(plugindir)
EXTRA_DIST = SPIBackend.h SPIOutput.h SPIPlugin.h SPIDevice.h SPIPort.h

if USE_SPI
lib_LTLIBRARIES = libolaspicore.la libolaspi.la
libolaspicore_la_SOURCES = SPIBackend.cpp SPIOutput.cpp
libolaspicore_la_LIBADD = ../../common/libolacommon.la

libolaspi_la_SOURCES = SPIPlugin.cpp SPIDevice.cpp SPIPort.cpp
libolaspi_la_LIBADD = ../../common/libolacommon.la \
                      libolaspicore.la
endif
