# LIBRARIES
##################################################
if USE_ESPNET
lib_LTLIBRARIES += plugins/espnet/libolaespnet.la
plugins_espnet_libolaespnet_la_SOURCES = \
    plugins/espnet/EspNetDevice.cpp \
    plugins/espnet/EspNetDevice.h \
    plugins/espnet/EspNetNode.cpp \
    plugins/espnet/EspNetNode.h \
    plugins/espnet/EspNetPackets.h \
    plugins/espnet/EspNetPlugin.cpp \
    plugins/espnet/EspNetPlugin.h \
    plugins/espnet/EspNetPluginCommon.h \
    plugins/espnet/EspNetPort.cpp \
    plugins/espnet/EspNetPort.h \
    plugins/espnet/RunLengthDecoder.cpp \
    plugins/espnet/RunLengthDecoder.h
plugins_espnet_libolaespnet_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la

# TESTS
##################################################
test_programs += plugins/espnet/EspNetTester

plugins_espnet_EspNetTester_SOURCES = \
    plugins/espnet/RunLengthDecoderTest.cpp \
    plugins/espnet/RunLengthDecoder.cpp
plugins_espnet_EspNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_espnet_EspNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                                    common/libolacommon.la
endif
