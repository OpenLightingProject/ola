include $(top_srcdir)/common.mk
libdir = $(plugindir)

EXTRA_DIST = EspNetPlugin.h EspNetDevice.h EspNetPort.h EspNetPackets.h \
             EspNetNode.h EspNetPluginCommon.h RunLengthDecoder.h

if USE_ESPNET
lib_LTLIBRARIES = libolaespnet.la
libolaespnet_la_SOURCES = EspNetPlugin.cpp EspNetDevice.cpp EspNetPort.cpp \
                          EspNetNode.cpp RunLengthDecoder.cpp
libolaespnet_la_LIBADD = ../../common/libolacommon.la

# Test Programs
if BUILD_TESTS
TESTS = EspNetTester
endif
check_PROGRAMS = $(TESTS)
EspNetTester_SOURCES = RunLengthDecoderTest.cpp \
                       RunLengthDecoder.cpp
EspNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
EspNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                     $(top_builddir)/common/testing/libtestmain.la \
                     ../../common/libolacommon.la
endif
