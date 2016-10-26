# LIBRARIES
##################################################
if USE_KINET
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/kinet/libolakinetnode.la
plugins_kinet_libolakinetnode_la_SOURCES = plugins/kinet/KiNetNode.cpp \
                                           plugins/kinet/KiNetNode.h
plugins_kinet_libolakinetnode_la_LIBADD = common/libolacommon.la

lib_LTLIBRARIES += plugins/kinet/libolakinet.la
plugins_kinet_libolakinet_la_SOURCES = \
    plugins/kinet/KiNetPlugin.cpp \
    plugins/kinet/KiNetPlugin.h \
    plugins/kinet/KiNetDevice.cpp \
    plugins/kinet/KiNetDevice.h \
    plugins/kinet/KiNetPort.h
plugins_kinet_libolakinet_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/kinet/libolakinetnode.la

# TESTS
##################################################
test_programs += plugins/kinet/KiNetTester

plugins_kinet_KiNetTester_SOURCES = plugins/kinet/KiNetNodeTest.cpp
plugins_kinet_KiNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_kinet_KiNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                                  plugins/kinet/libolakinetnode.la
endif

EXTRA_DIST += \
    plugins/kinet/README.md
