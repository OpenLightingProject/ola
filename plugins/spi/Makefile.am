include $(top_srcdir)/common.mk

libdir = $(plugindir)
EXTRA_DIST = FakeSPIWriter.h SPIBackend.h SPIOutput.h SPIPlugin.h \
             SPIDevice.h SPIPort.h SPIWriter.h

if USE_SPI
lib_LTLIBRARIES = libolaspicore.la libolaspi.la
libolaspicore_la_SOURCES = SPIBackend.cpp SPIOutput.cpp SPIWriter.cpp
libolaspicore_la_LIBADD = ../../common/libolacommon.la

libolaspi_la_SOURCES = SPIPlugin.cpp SPIDevice.cpp SPIPort.cpp
libolaspi_la_LIBADD = ../../common/libolacommon.la \
                      libolaspicore.la

# Test programs
if BUILD_TESTS
TESTS = SPITester
endif
check_PROGRAMS = $(TESTS)
SPITester_SOURCES = SPIBackendTest.cpp SPIOutputTest.cpp \
                    FakeSPIWriter.cpp
SPITester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SPITester_LDADD = $(COMMON_TESTING_LIBS) \
                  libolaspicore.la \
                  ../../common/libolacommon.la

endif
