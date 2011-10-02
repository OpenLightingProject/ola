include $(top_srcdir)/common.mk
libdir = $(plugindir)

EXTRA_DIST = EspNetPlugin.h EspNetDevice.h EspNetPort.h EspNetPackets.h \
             EspNetNode.h EspNetPluginCommon.h RunLengthDecoder.h

lib_LTLIBRARIES = libolaespnet.la
libolaespnet_la_SOURCES = EspNetPlugin.cpp EspNetDevice.cpp EspNetPort.cpp \
                          EspNetNode.cpp RunLengthDecoder.cpp
libolaespnet_la_LIBADD = ../../common/libolacommon.la

# Test Programs
TESTS = EspNetTester
check_PROGRAMS = $(TESTS)
EspNetTester_SOURCES = EspNetTester.cpp \
                       RunLengthDecoderTest.cpp \
                       RunLengthDecoder.cpp
EspNetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
EspNetTester_LDADD = $(CPPUNIT_LIBS) \
                     ../../common/libolacommon.la
