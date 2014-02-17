include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_SPI
# This is a library which isn't coupled to olad
plugin_LTLIBRARIES = libolaspicore.la libolaspi.la
libolaspicore_la_SOURCES = SPIBackend.cpp SPIBackend.h SPIOutput.cpp \
                           SPIOutput.h SPIWriter.cpp SPIWriter.h
libolaspicore_la_LIBADD = ../../common/libolacommon.la

libolaspi_la_SOURCES = SPIDevice.cpp SPIDevice.h SPIPlugin.cpp SPIPlugin.h \
                       SPIPort.cpp SPIPort.h
libolaspi_la_LIBADD = ../../common/libolacommon.la \
                      libolaspicore.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = SPITester
endif
check_PROGRAMS = $(TESTS)

SPITester_SOURCES = SPIBackendTest.cpp SPIOutputTest.cpp \
                    FakeSPIWriter.cpp FakeSPIWriter.h
SPITester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SPITester_LDADD = $(COMMON_TESTING_LIBS) \
                  libolaspicore.la \
                  ../../common/libolacommon.la

endif
