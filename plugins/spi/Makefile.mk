# LIBRARIES
##################################################
if USE_SPI
# This is a library which isn't coupled to olad
lib_LTLIBRARIES += plugins/spi/libolaspicore.la plugins/spi/libolaspi.la
plugins_spi_libolaspicore_la_SOURCES = \
    plugins/spi/SPIBackend.cpp \
    plugins/spi/SPIBackend.h \
    plugins/spi/SPIOutput.cpp \
    plugins/spi/SPIOutput.h \
    plugins/spi/SPIWriter.cpp \
    plugins/spi/SPIWriter.h
plugins_spi_libolaspicore_la_LIBADD = common/libolacommon.la

# Plugin description is generated from README.md
built_sources += plugins/spi/SPIPluginDescription.h
nodist_plugins_spi_libolaspi_la_SOURCES = \
    plugins/spi/SPIPluginDescription.h
plugins/spi/SPIPluginDescription.h: plugins/spi/README.md plugins/spi/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/spi $(top_builddir)/plugins/spi/SPIPluginDescription.h

plugins_spi_libolaspi_la_SOURCES = \
    plugins/spi/SPIDevice.cpp \
    plugins/spi/SPIDevice.h \
    plugins/spi/SPIPlugin.cpp \
    plugins/spi/SPIPlugin.h \
    plugins/spi/SPIPort.cpp \
    plugins/spi/SPIPort.h
plugins_spi_libolaspi_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/spi/libolaspicore.la

# TESTS
##################################################
test_programs += plugins/spi/SPITester

plugins_spi_SPITester_SOURCES = \
    plugins/spi/SPIBackendTest.cpp \
    plugins/spi/SPIOutputTest.cpp \
    plugins/spi/FakeSPIWriter.cpp \
    plugins/spi/FakeSPIWriter.h
plugins_spi_SPITester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_spi_SPITester_LDADD = $(COMMON_TESTING_LIBS) \
                              plugins/spi/libolaspicore.la \
                              common/libolacommon.la

endif

EXTRA_DIST += plugins/spi/README.md
