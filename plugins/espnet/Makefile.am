include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_ESPNET
plugin_LTLIBRARIES = libolaespnet.la
libolaespnet_la_SOURCES = EspNetDevice.cpp EspNetDevice.h EspNetNode.cpp \
                          EspNetNode.h EspNetPackets.h EspNetPlugin.cpp \
                          EspNetPlugin.h EspNetPluginCommon.h EspNetPort.cpp \
                          EspNetPort.h RunLengthDecoder.cpp RunLengthDecoder.h
libolaespnet_la_LIBADD = ../../common/libolacommon.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = EspNetTester
endif
check_PROGRAMS = $(TESTS)
EspNetTester_SOURCES = RunLengthDecoderTest.cpp \
                       RunLengthDecoder.cpp
EspNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
EspNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                     ../../common/libolacommon.la
endif
