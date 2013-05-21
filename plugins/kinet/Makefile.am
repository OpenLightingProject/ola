include $(top_srcdir)/common.mk
libdir = $(plugindir)
EXTRA_DIST = KiNetPlugin.h KiNetDevice.h KiNetPort.h KiNetNode.h

if USE_KINET
# This is a library which doesn't contain any coupling to olad
noinst_LTLIBRARIES = libolakinetnode.la
libolakinetnode_la_SOURCES = KiNetNode.cpp
libolakinetnode_la_LIBADD = $(top_builddir)/common/libolacommon.la

# The OLA kinet plugin
lib_LTLIBRARIES = libolakinet.la
libolakinet_la_SOURCES = KiNetPlugin.cpp KiNetDevice.cpp
libolakinet_la_LIBADD = libolakinetnode.la

if BUILD_TESTS
TESTS = KiNetTester
endif
check_PROGRAMS = $(TESTS)
KiNetTester_SOURCES = KiNetNodeTest.cpp
KiNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
KiNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                     libolakinetnode.la
endif
