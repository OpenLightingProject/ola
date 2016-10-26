# LIBRARIES
##################################################
if USE_OSC
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/osc/libolaoscnode.la
plugins_osc_libolaoscnode_la_SOURCES = \
    plugins/osc/OSCAddressTemplate.cpp \
    plugins/osc/OSCAddressTemplate.h \
    plugins/osc/OSCNode.cpp \
    plugins/osc/OSCNode.h \
    plugins/osc/OSCTarget.h
plugins_osc_libolaoscnode_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(liblo_CFLAGS)
plugins_osc_libolaoscnode_la_LIBADD = $(liblo_LIBS)

lib_LTLIBRARIES += plugins/osc/libolaosc.la
plugins_osc_libolaosc_la_SOURCES = \
    plugins/osc/OSCDevice.cpp \
    plugins/osc/OSCDevice.h \
    plugins/osc/OSCPlugin.cpp \
    plugins/osc/OSCPlugin.h \
    plugins/osc/OSCPort.cpp \
    plugins/osc/OSCPort.h
plugins_osc_libolaosc_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(liblo_CFLAGS)
plugins_osc_libolaosc_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/osc/libolaoscnode.la

# TESTS
##################################################
test_programs += plugins/osc/OSCTester

plugins_osc_OSCTester_SOURCES = \
    plugins/osc/OSCAddressTemplateTest.cpp \
    plugins/osc/OSCNodeTest.cpp
plugins_osc_OSCTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_osc_OSCTester_LDADD = $(COMMON_TESTING_LIBS) \
                  plugins/osc/libolaoscnode.la \
                  common/libolacommon.la
endif

EXTRA_DIST += \
    plugins/osc/README.md \
    plugins/osc/README.developer
