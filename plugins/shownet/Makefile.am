include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = ShowNetPlugin.h ShowNetDevice.h ShowNetPort.h ShowNetPackets.h \
             ShowNetNode.h

if USE_SHOWNET
lib_LTLIBRARIES = libolashownet.la
libolashownet_la_SOURCES = ShowNetPlugin.cpp ShowNetDevice.cpp ShowNetPort.cpp\
                           ShowNetNode.cpp
libolashownet_la_LIBADD = ../../common/libolacommon.la

# Test Programs
if BUILD_TESTS
TESTS = ShowNetTester
endif
check_PROGRAMS = $(TESTS)
ShowNetTester_SOURCES = ShowNetNode.cpp \
                        ShowNetNodeTest.cpp
ShowNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ShowNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                      $(top_builddir)/common/testing/libtestmain.la \
                      ../../common/libolacommon.la
endif
