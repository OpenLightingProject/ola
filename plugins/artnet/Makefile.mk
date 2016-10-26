include plugins/artnet/messages/Makefile.mk

# LIBRARIES
##################################################
if USE_ARTNET
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/artnet/libolaartnetnode.la
plugins_artnet_libolaartnetnode_la_SOURCES = \
    plugins/artnet/ArtNetPackets.h \
    plugins/artnet/ArtNetNode.cpp \
    plugins/artnet/ArtNetNode.h
plugins_artnet_libolaartnetnode_la_LIBADD = common/libolacommon.la

# The OLA artnet plugin
BUILT_SOURCES += plugins/artnet/PluginDescription.cpp
lib_LTLIBRARIES += plugins/artnet/libolaartnet.la
plugins_artnet_libolaartnet_la_SOURCES = \
    plugins/artnet/PluginDescription.cpp \
    plugins/artnet/ArtNetPlugin.cpp \
    plugins/artnet/ArtNetPlugin.h \
    plugins/artnet/ArtNetDevice.cpp \
    plugins/artnet/ArtNetDevice.h \
    plugins/artnet/ArtNetPort.cpp \
    plugins/artnet/ArtNetPort.h
plugins_artnet_libolaartnet_la_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
plugins_artnet_libolaartnet_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/artnet/libolaartnetnode.la \
    plugins/artnet/messages/libolaartnetconf.la

# PROGRAMS
##################################################
noinst_PROGRAMS += plugins/artnet/artnet_loadtest

plugins_artnet_artnet_loadtest_SOURCES = plugins/artnet/artnet_loadtest.cpp
plugins_artnet_artnet_loadtest_LDADD = plugins/artnet/libolaartnetnode.la

# TESTS
##################################################
test_programs += plugins/artnet/ArtNetTester

plugins_artnet_ArtNetTester_SOURCES = plugins/artnet/ArtNetNodeTest.cpp
plugins_artnet_ArtNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_artnet_ArtNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                                    plugins/artnet/libolaartnetnode.la
endif

EXTRA_DIST += \
    plugins/artnet/README.md
