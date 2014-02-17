include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_DUMMY
plugin_LTLIBRARIES = liboladummy.la

liboladummy_la_SOURCES = DummyDevice.cpp DummyDevice.h DummyPlugin.cpp \
                         DummyPlugin.h DummyPort.cpp DummyPort.h
liboladummy_la_LIBADD = ../../common/libolacommon.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = DummyPluginTester
endif
check_PROGRAMS = $(TESTS)

DummyPluginTester_SOURCES = DummyPortTest.cpp
DummyPluginTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
# it's unclear to me why liboladummyresponder has to be included here
# but if it isn't, the test breaks with gcc 4.6.1
DummyPluginTester_LDADD = $(COMMON_TESTING_LIBS) \
                          $(top_builddir)/olad/libolaserver.la \
                          ./liboladummy.la \
                          ../../common/libolacommon.la

endif
