include $(top_srcdir)/common.mk
libdir = $(plugindir)

EXTRA_DIST = DummyPlugin.h DummyDevice.h DummyPort.h DummyResponder.h DummyRDMDevice.h


if USE_DUMMY
lib_LTLIBRARIES = liboladummyresponder.la liboladummy.la
liboladummyresponder_la_SOURCES = DummyResponder.cpp DummyRDMDevice.cpp
liboladummyresponder_la_LIBADD = ../../common/libolacommon.la

liboladummy_la_SOURCES = DummyPlugin.cpp DummyDevice.cpp DummyPort.cpp
liboladummy_la_LIBADD = ./liboladummyresponder.la

# Test programs
if BUILD_TESTS
TESTS = DummyPluginTester
endif
check_PROGRAMS = $(TESTS)

DummyPluginTester_SOURCES = DummyPortTest.cpp
DummyPluginTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
# it's unclear to me why liboladummyresponder has to be included here
# but if it isn't, the test breaks with gcc 4.6.1
DummyPluginTester_LDADD = $(COMMON_TEST_LDADD) \
                          $(top_builddir)/olad/libolaserver.la \
                          $(top_builddir)/common/testing/libtestmain.la \
                          ./liboladummy.la \
                          ./liboladummyresponder.la \
                          ../../common/libolacommon.la
endif
