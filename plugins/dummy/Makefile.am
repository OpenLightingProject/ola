include $(top_srcdir)/common.mk
libdir = $(plugindir)

EXTRA_DIST = DummyPlugin.h DummyDevice.h DummyPort.h DummyResponder.h

lib_LTLIBRARIES = liboladummy.la
liboladummy_la_SOURCES =  DummyPlugin.cpp DummyDevice.cpp DummyPort.cpp \
                          DummyResponder.cpp
liboladummy_la_LIBADD = @LIBS@

TESTS = DummyPluginTester
check_PROGRAMS = $(TESTS)
DummyPluginTester_SOURCES = DummyPluginTester.cpp DummyPortTest.cpp
DummyPluginTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
DummyPluginTester_LDFLAGS = $(CPPUNIT_LIBS) $(PROTOBUF_LIBS)
DummyPluginTester_LDADD = @LIBS@ \
                          ./liboladummy.la \
                          $(top_builddir)/common/libolacommon.la \
                          $(top_builddir)/olad/libolaserver.la
