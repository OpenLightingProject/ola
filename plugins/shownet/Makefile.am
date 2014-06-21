include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_SHOWNET
plugin_LTLIBRARIES = libolashownet.la
libolashownet_la_SOURCES = ShowNetPlugin.cpp ShowNetDevice.cpp \
                           ShowNetPort.cpp ShowNetNode.cpp ShowNetPlugin.h \
                           ShowNetDevice.h ShowNetPort.h ShowNetPackets.h \
                           ShowNetNode.h
libolashownet_la_LIBADD = ../../common/libolacommon.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = ShowNetTester
endif
check_PROGRAMS = $(TESTS)

ShowNetTester_SOURCES = ShowNetNode.cpp \
                        ShowNetNodeTest.cpp
ShowNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ShowNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                      ../../common/libolacommon.la
endif
