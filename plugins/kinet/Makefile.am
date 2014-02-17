include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_KINET
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES = libolakinetnode.la
libolakinetnode_la_SOURCES = KiNetNode.cpp KiNetNode.h
libolakinetnode_la_LIBADD = ../../common/libolacommon.la

plugin_LTLIBRARIES = libolakinet.la
libolakinet_la_SOURCES = KiNetPlugin.cpp KiNetPlugin.h KiNetDevice.cpp \
                         KiNetDevice.h KiNetPort.h
libolakinet_la_LIBADD = libolakinetnode.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = KiNetTester
endif
check_PROGRAMS = $(TESTS)

KiNetTester_SOURCES = KiNetNodeTest.cpp
KiNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
KiNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                     libolakinetnode.la
endif
