# LIBRARIES
##################################################
if USE_DUMMY
lib_LTLIBRARIES += plugins/dummy/liboladummy.la

plugins_dummy_liboladummy_la_SOURCES = \
    plugins/dummy/DummyDevice.cpp \
    plugins/dummy/DummyDevice.h \
    plugins/dummy/DummyPlugin.cpp \
    plugins/dummy/DummyPlugin.h \
    plugins/dummy/DummyPort.cpp \
    plugins/dummy/DummyPort.h
plugins_dummy_liboladummy_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la

# TESTS
##################################################
test_programs += plugins/dummy/DummyPluginTester

plugins_dummy_DummyPluginTester_SOURCES = plugins/dummy/DummyPortTest.cpp
plugins_dummy_DummyPluginTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
# it's unclear to me why liboladummyresponder has to be included here
# but if it isn't, the test breaks with gcc 4.6.1
plugins_dummy_DummyPluginTester_LDADD = \
    $(COMMON_TESTING_LIBS) \
    $(top_builddir)/olad/plugin_api/libolaserverplugininterface.la \
    $(top_builddir)/olad/libolaserver.la \
    plugins/dummy/liboladummy.la \
    common/libolacommon.la

endif
