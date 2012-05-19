include $(top_srcdir)/common.mk
libdir = $(plugindir)

EXTRA_DIST = DummyPlugin.h DummyDevice.h DummyPort.h DummyResponder.h DummyRDMDevice.h

lib_LTLIBRARIES = liboladummyrdmdevice.la liboladummyresponder.la liboladummy.la
liboladummyrdmdevice_la_SOURCES = DummyRDMDevice.cpp
liboladummyrdmdevice_la_LIBADD = ../../common/libolacommon.la

liboladummyresponder_la_SOURCES = DummyResponder.cpp
liboladummyresponder_la_LIBADD = ../../common/libolacommon.la ./liboladummyrdmdevice.la

liboladummy_la_SOURCES = DummyPlugin.cpp DummyDevice.cpp DummyPort.cpp
liboladummy_la_LIBADD = ./liboladummyresponder.la

# Test programs
TESTS = DummyPluginTester
check_PROGRAMS = $(TESTS)
DummyPluginTester_SOURCES = DummyPluginTester.cpp DummyPortTest.cpp
DummyPluginTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
# it's unclear to me why liboladummyresponder has to be included here
# but if it isn't, the test breaks with gcc 4.6.1
DummyPluginTester_LDADD = ./liboladummy.la \
                          ./liboladummyresponder.la \
                          ./liboladummyrdmdevice.la \
                          $(CPPUNIT_LIBS) \
                          $(top_builddir)/olad/libolaserver.la \
                          ../../common/libolacommon.la
