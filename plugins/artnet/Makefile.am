include $(top_srcdir)/common.mk
libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = ArtNetDevice.h ArtNetNode.h ArtNetPackets.h ArtNetPlugin.h \
             ArtNetPort.h MockUdpSocket.h

# This is a library which doesn't contain any coupling to olad
noinst_LTLIBRARIES = libolaartnetnode.la
libolaartnetnode_la_SOURCES = ArtNetNode.cpp
libolaartnetnode_la_LIBADD = ../../common/libolacommon.la

# The OLA artnet plugin
lib_LTLIBRARIES = libolaartnet.la
libolaartnet_la_SOURCES = ArtNetPlugin.cpp ArtNetDevice.cpp ArtNetPort.cpp
libolaartnet_la_LIBADD = ./libolaartnetnode.la \
                         messages/libolaartnetconf.la

# Test programs
check_PROGRAMS = ArtNetTester
ArtNetTester_SOURCES = ArtNetNodeTest.cpp \
                       ArtNetTester.cpp \
                       MockUdpSocket.cpp
ArtNetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ArtNetTester_LDADD = $(CPPUNIT_LIBS) \
                     ./libolaartnetnode.la
