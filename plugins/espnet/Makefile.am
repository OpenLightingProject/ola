libdir = $(plugindir)

EXTRA_DIST = EspNetPlugin.h EspNetDevice.h EspNetPort.h EspNetPackets.h \
             EspNetNode.h EspNetPluginCommon.h RunLengthDecoder.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat

lib_LTLIBRARIES = libolaespnet.la
libolaespnet_la_SOURCES = EspNetPlugin.cpp EspNetDevice.cpp EspNetPort.cpp \
                          EspNetNode.cpp RunLengthDecoder.cpp
libolaespnet_la_LIBADD = @LIBS@

# Test Programs
TESTS = EspNetTester
check_PROGRAMS = $(TESTS)
EspNetTester_SOURCES = EspNetTester.cpp RunLengthDecoderTest.cpp \
                       RunLengthDecoder.cpp
EspNetTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
EspNetTester_LDFLAGS = $(CPPUNIT_LIBS)
EspNetTester_LDADD = @LIBS@ @LIBLTDL@  -lprotobuf ../../common/libolacommon.la
