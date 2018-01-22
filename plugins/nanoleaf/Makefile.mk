# LIBRARIES
##################################################
if USE_NANOLEAF
# This is a library which isn't coupled to olad
noinst_LTLIBRARIES += plugins/nanoleaf/libolananoleafnode.la
plugins_nanoleaf_libolananoleafnode_la_SOURCES = plugins/nanoleaf/NanoleafNode.cpp \
                                                 plugins/nanoleaf/NanoleafNode.h
plugins_nanoleaf_libolananoleafnode_la_LIBADD = common/libolacommon.la

lib_LTLIBRARIES += plugins/nanoleaf/libolananoleaf.la

# Plugin description is generated from README.md
built_sources += plugins/nanoleaf/NanoleafPluginDescription.h
nodist_plugins_nanoleaf_libolananoleaf_la_SOURCES = \
    plugins/nanoleaf/NanoleafPluginDescription.h
plugins/nanoleaf/NanoleafPluginDescription.h: plugins/nanoleaf/README.md plugins/nanoleaf/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/nanoleaf $(top_builddir)/plugins/nanoleaf/NanoleafPluginDescription.h

plugins_nanoleaf_libolananoleaf_la_SOURCES = \
    plugins/nanoleaf/NanoleafPlugin.cpp \
    plugins/nanoleaf/NanoleafPlugin.h \
    plugins/nanoleaf/NanoleafDevice.cpp \
    plugins/nanoleaf/NanoleafDevice.h \
    plugins/nanoleaf/NanoleafPort.h
plugins_nanoleaf_libolananoleaf_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/nanoleaf/libolananoleafnode.la

# TESTS
##################################################
test_programs += plugins/nanoleaf/NanoleafTester

plugins_nanoleaf_NanoleafTester_SOURCES = plugins/nanoleaf/NanoleafNodeTest.cpp
plugins_nanoleaf_NanoleafTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
plugins_nanoleaf_NanoleafTester_LDADD = $(COMMON_TESTING_LIBS) \
                                        plugins/nanoleaf/libolananoleafnode.la
endif

EXTRA_DIST += plugins/nanoleaf/README.md
