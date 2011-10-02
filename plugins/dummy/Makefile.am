include $(top_srcdir)/common.mk
libdir = $(plugindir)

EXTRA_DIST = DummyPlugin.h DummyDevice.h DummyPort.h DummyResponder.h

lib_LTLIBRARIES = liboladummyresponder.la liboladummy.la
liboladummyresponder_la_SOURCES = DummyResponder.cpp
liboladummyresponder_la_LIBADD = ../../common/libolacommon.la

liboladummy_la_SOURCES = DummyPlugin.cpp DummyDevice.cpp DummyPort.cpp
liboladummy_la_LIBADD = ./liboladummyresponder.la

# Test programs
TESTS = DummyPluginTester
check_PROGRAMS = $(TESTS)
DummyPluginTester_SOURCES = DummyPluginTester.cpp DummyPortTest.cpp
DummyPluginTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DummyPluginTester_LDADD = $(CPPUNIT_LIBS) \
                          ./liboladummy.la \
                          $(top_builddir)/olad/libolaserver.la
