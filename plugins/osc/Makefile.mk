# LIBRARIES
##################################################
if USE_OSC
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/osc/libolaoscnode.la
plugins_osc_libolaoscnode_la_SOURCES = \
    plugins/osc/OscAddressTemplate.cpp \
    plugins/osc/OscAddressTemplate.h \
    plugins/osc/OscNode.cpp \
    plugins/osc/OscNode.h \
    plugins/osc/OscTarget.h
plugins_osc_libolaoscnode_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(liblo_CFLAGS)
plugins_osc_libolaoscnode_la_LIBADD = $(liblo_LIBS)

lib_LTLIBRARIES += plugins/osc/libolaosc.la

# Plugin description is generated from README.md
built_sources += plugins/osc/OscPluginDescription.h
nodist_plugins_osc_libolaosc_la_SOURCES = \
    plugins/osc/OscPluginDescription.h
plugins/osc/OscPluginDescription.h: plugins/osc/README.md plugins/osc/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/osc $(top_builddir)/plugins/osc/OscPluginDescription.h

plugins_osc_libolaosc_la_SOURCES = \
    plugins/osc/OscDevice.cpp \
    plugins/osc/OscDevice.h \
    plugins/osc/OscPlugin.cpp \
    plugins/osc/OscPlugin.h \
    plugins/osc/OscPort.cpp \
    plugins/osc/OscPort.h
plugins_osc_libolaosc_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(liblo_CFLAGS)
plugins_osc_libolaosc_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/osc/libolaoscnode.la

# TESTS
##################################################
test_programs += plugins/osc/OscTester

plugins_osc_OscTester_SOURCES = \
    plugins/osc/OscAddressTemplateTest.cpp \
    plugins/osc/OscNodeTest.cpp
plugins_osc_OscTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_osc_OscTester_LDADD = $(COMMON_TESTING_LIBS) \
                  plugins/osc/libolaoscnode.la \
                  common/libolacommon.la
endif

EXTRA_DIST += \
    plugins/osc/README.md \
    plugins/osc/README.developer.md
