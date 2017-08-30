# LIBRARIES
##################################################
if USE_SPI
# This is a library which isn't coupled to olad
lib_LTLIBRARIES += plugins/spi/libolaspicore.la plugins/spi/libolaspi.la
plugins_spi_libolaspicore_la_SOURCES = \
    plugins/spi/SpiBackend.cpp \
    plugins/spi/SpiBackend.h \
    plugins/spi/SpiOutput.cpp \
    plugins/spi/SpiOutput.h \
    plugins/spi/SpiWriter.cpp \
    plugins/spi/SpiWriter.h
plugins_spi_libolaspicore_la_LIBADD = common/libolacommon.la

# Plugin description is generated from README.md
built_sources += plugins/spi/SpiPluginDescription.h
nodist_plugins_spi_libolaspi_la_SOURCES = \
    plugins/spi/SpiPluginDescription.h
plugins/spi/SpiPluginDescription.h: plugins/spi/README.md plugins/spi/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/spi $(top_builddir)/plugins/spi/SpiPluginDescription.h

plugins_spi_libolaspi_la_SOURCES = \
    plugins/spi/SpiDevice.cpp \
    plugins/spi/SpiDevice.h \
    plugins/spi/SpiPlugin.cpp \
    plugins/spi/SpiPlugin.h \
    plugins/spi/SpiPort.cpp \
    plugins/spi/SpiPort.h
plugins_spi_libolaspi_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/spi/libolaspicore.la

# TESTS
##################################################
test_programs += plugins/spi/SpiTester

plugins_spi_SpiTester_SOURCES = \
    plugins/spi/SpiBackendTest.cpp \
    plugins/spi/SpiOutputTest.cpp \
    plugins/spi/FakeSpiWriter.cpp \
    plugins/spi/FakeSpiWriter.h
plugins_spi_SpiTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_spi_SpiTester_LDADD = $(COMMON_TESTING_LIBS) \
                              plugins/spi/libolaspicore.la \
                              common/libolacommon.la

endif

EXTRA_DIST += plugins/spi/README.md
