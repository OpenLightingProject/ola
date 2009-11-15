libdir = $(plugindir)

EXTRA_DIST = ShowNetPlugin.h ShowNetDevice.h ShowNetPort.h ShowNetPackets.h \
             ShowNetNode.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

lib_LTLIBRARIES = libolashownet.la
libolashownet_la_SOURCES = ShowNetPlugin.cpp ShowNetDevice.cpp ShowNetPort.cpp\
                           ShowNetNode.cpp
libolashownet_la_LIBADD = @LIBS@

# Test Programs
TESTS = ShowNetTester
check_PROGRAMS = $(TESTS)
ShowNetTester_SOURCES = ShowNetTester.cpp \
                        ShowNetNode.cpp ShowNetNodeTest.cpp
ShowNetTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
ShowNetTester_LDFLAGS = $(CPPUNIT_LIBS)
ShowNetTester_LDADD = @LIBS@ @LIBLTDL@  -lprotobuf ../../common/libolacommon.la
