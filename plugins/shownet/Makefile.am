
libdir = $(plugindir)

lib_LTLIBRARIES = libllashownet.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

libllashownet_la_SOURCES = ShowNetPlugin.cpp ShowNetDevice.cpp ShowNetPort.cpp\
                           ShowNetNode.cpp RunLengthEncoder.cpp
libllashownet_la_LIBADD = @LIBS@

EXTRA_DIST = ShowNetPlugin.h ShowNetDevice.h ShowNetPort.h ShowNetPackets.h \
             ShowNetNode.h RunLengthEncoder.h

# Test Programs
TESTS = ShowNetTester
check_PROGRAMS = $(TESTS)
ShowNetTester_SOURCES = ShowNetTester.cpp RunLengthEncoderTest.cpp \
                        RunLengthEncoder.cpp
ShowNetTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
ShowNetTester_LDFLAGS = $(CPPUNIT_LIBS)
ShowNetTester_LDADD = @LIBS@ @LIBLTDL@  -lprotobuf ../../common/libllacommon.la
