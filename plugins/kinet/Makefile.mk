# LIBRARIES
##################################################
if USE_KINET
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/kinet/libolakinetnode.la
plugins_kinet_libolakinetnode_la_SOURCES = plugins/kinet/KiNetNode.cpp \
                                           plugins/kinet/KiNetNode.h
plugins_kinet_libolakinetnode_la_LIBADD = common/libolacommon.la

lib_LTLIBRARIES += plugins/kinet/libolakinet.la

# Plugin description is generated from README.md
built_sources += plugins/kinet/KiNetPluginDescription.h
nodist_plugins_kinet_libolakinet_la_SOURCES = \
    plugins/kinet/KiNetPluginDescription.h
plugins/kinet/KiNetPluginDescription.h: plugins/kinet/README.md plugins/kinet/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/kinet $(top_builddir)/plugins/kinet/KiNetPluginDescription.h

plugins_kinet_libolakinet_la_SOURCES = \
    plugins/kinet/KiNetPlugin.cpp \
    plugins/kinet/KiNetPlugin.h \
    plugins/kinet/KiNetDevice.cpp \
    plugins/kinet/KiNetDevice.h \
    plugins/kinet/KiNetPort.h
plugins_kinet_libolakinet_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/kinet/libolakinetnode.la

# PROGRAMS
##################################################
noinst_PROGRAMS += plugins/kinet/kinet

plugins_kinet_kinet_SOURCES = plugins/kinet/kinet.cpp
plugins_kinet_kinet_LDADD = plugins/kinet/libolakinetnode.la

# TESTS
##################################################
test_programs += plugins/kinet/KiNetTester

plugins_kinet_KiNetTester_SOURCES = plugins/kinet/KiNetNodeTest.cpp
plugins_kinet_KiNetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_kinet_KiNetTester_LDADD = $(COMMON_TESTING_LIBS) \
                                  plugins/kinet/libolakinetnode.la
endif

EXTRA_DIST += plugins/kinet/README.md
