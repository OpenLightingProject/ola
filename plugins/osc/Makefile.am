include $(top_srcdir)/common.mk

libdir = $(plugindir)
EXTRA_DIST = OSCAddressTemplate.h OSCDevice.h OSCNode.h OSCPlugin.h OSCPort.h \
             OSCTarget.h

if HAVE_LIBLO
  noinst_LTLIBRARIES = libolaoscnode.la
  libolaoscnode_la_SOURCES = OSCAddressTemplate.cpp OSCNode.cpp
  libolaoscnode_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(liblo_CFLAGS)
  libolaoscnode_la_LIBADD = $(liblo_LIBS)

  lib_LTLIBRARIES = libolaosc.la
  libolaosc_la_SOURCES = OSCDevice.cpp OSCPlugin.cpp
  libolaosc_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(liblo_CFLAGS)
  libolaosc_la_LIBADD = libolaoscnode.la

if BUILD_TESTS
TESTS = OSCTester
endif
check_PROGRAMS = $(TESTS)

OSCTester_SOURCES = OSCAddressTemplateTest.cpp \
                    OSCNodeTest.cpp
OSCTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
OSCTester_LDADD = $(COMMON_TESTING_LIBS) \
                  $(top_builddir)/common/testing/libtestmain.la \
                  libolaoscnode.la \
                  ../../common/libolacommon.la
endif
