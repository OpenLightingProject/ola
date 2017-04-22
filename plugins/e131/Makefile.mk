include plugins/e131/messages/Makefile.mk

# LIBRARIES
##################################################

if USE_E131
lib_LTLIBRARIES += plugins/e131/libolae131.la

# Plugin description is generated from README.md
built_sources += plugins/e131/E131PluginDescription.h
nodist_plugins_e131_libolae131_la_SOURCES = \
    plugins/e131/E131PluginDescription.h
plugins/e131/E131PluginDescription.h: plugins/e131/README.md plugins/e131/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/e131 $(top_builddir)/plugins/e131/E131PluginDescription.h

plugins_e131_libolae131_la_SOURCES = \
    plugins/e131/E131Device.cpp \
    plugins/e131/E131Device.h \
    plugins/e131/E131Plugin.cpp \
    plugins/e131/E131Plugin.h \
    plugins/e131/E131Port.cpp \
    plugins/e131/E131Port.h
plugins_e131_libolae131_la_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
plugins_e131_libolae131_la_LIBADD = \
    olad/plugin_api/libolaserverplugininterface.la \
    plugins/e131/messages/libolae131conf.la \
    libs/acn/libolae131core.la
endif

EXTRA_DIST += plugins/e131/README.md
