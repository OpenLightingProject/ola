include $(top_srcdir)/common.mk

SUBDIRS = messages

# LIBRARIES
##################################################
if USE_ARTNET
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES = libolaartnetnode.la
libolaartnetnode_la_SOURCES = ArtNetPackets.h ArtNetNode.cpp ArtNetNode.h
libolaartnetnode_la_LIBADD = ../../common/libolacommon.la

# The OLA artnet plugin
plugin_LTLIBRARIES = libolaartnet.la
libolaartnet_la_SOURCES = ArtNetPlugin.cpp ArtNetPlugin.h ArtNetDevice.cpp \
                          ArtNetDevice.h ArtNetPort.cpp ArtNetPort.h
libolaartnet_la_LIBADD = libolaartnetnode.la \
                         messages/libolaartnetconf.la

# PROGRAMS
##################################################
noinst_PROGRAMS = artnet_loadtest

artnet_loadtest_SOURCES = artnet_loadtest.cpp
artnet_loadtest_LDADD = libolaartnetnode.la

if USING_WIN32
artnet_loadtest_LDADD += -lWs2_32 -lIphlpapi
endif

# TESTS
##################################################
if BUILD_TESTS
TESTS = ArtNetTester
endif
check_PROGRAMS = $(TESTS)

ArtNetTester_SOURCES = ArtNetNodeTest.cpp
ArtNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ArtNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                     libolaartnetnode.la

if USING_WIN32
ArtNetTester_LDADD += -lWs2_32 -lIphlpapi
endif

endif
