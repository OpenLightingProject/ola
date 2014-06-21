include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_OSC
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES = libolaoscnode.la
libolaoscnode_la_SOURCES = OSCAddressTemplate.cpp OSCAddressTemplate.h \
                           OSCNode.cpp OSCNode.h OSCTarget.h
libolaoscnode_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(liblo_CFLAGS)
libolaoscnode_la_LIBADD = $(liblo_LIBS)

plugin_LTLIBRARIES = libolaosc.la
libolaosc_la_SOURCES = OSCDevice.cpp OSCDevice.h OSCPlugin.cpp OSCPlugin.h \
                       OSCPort.cpp OSCPort.h
libolaosc_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(liblo_CFLAGS)
libolaosc_la_LIBADD = libolaoscnode.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = OSCTester
endif
check_PROGRAMS = $(TESTS)

OSCTester_SOURCES = OSCAddressTemplateTest.cpp \
                    OSCNodeTest.cpp
OSCTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
OSCTester_LDADD = $(COMMON_TESTING_LIBS) \
                  libolaoscnode.la \
                  ../../common/libolacommon.la
endif
