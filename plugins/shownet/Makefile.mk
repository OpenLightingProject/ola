# LIBRARIES
##################################################
if USE_SHOWNET
lib_LTLIBRARIES += plugins/shownet/libolashownet.la
plugins_shownet_libolashownet_la_SOURCES = \
    plugins/shownet/ShowNetPlugin.cpp \
    plugins/shownet/ShowNetDevice.cpp \
    plugins/shownet/ShowNetPort.cpp \
    plugins/shownet/ShowNetNode.cpp \
    plugins/shownet/ShowNetPlugin.h \
    plugins/shownet/ShowNetDevice.h \
    plugins/shownet/ShowNetPort.h \
    plugins/shownet/ShowNetPackets.h \
    plugins/shownet/ShowNetNode.h
plugins_shownet_libolashownet_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la

# TESTS
##################################################
test_programs += plugins/shownet/ShowNetTester

plugins_shownet_ShowNetTester_SOURCES = \
    plugins/shownet/ShowNetNode.cpp \
    plugins/shownet/ShowNetNodeTest.cpp
plugins_shownet_ShowNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_shownet_ShowNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                                      common/libolacommon.la
endif

EXTRA_DIST += \
    plugins/shownet/README.md
