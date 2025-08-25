# LIBRARIES
##################################################

if USE_E133
lib_LTLIBRARIES += plugins/e133/libolae133.la

# Plugin description is generated from README.md
built_sources += plugins/e133/E133PluginDescription.h
nodist_plugins_e133_libolae133_la_SOURCES = \
    plugins/e133/E133PluginDescription.h
plugins/e133/E133PluginDescription.h: plugins/e133/README.md plugins/e133/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/e133 $(top_builddir)/plugins/e133/E133PluginDescription.h

plugins_e133_libolae133_la_SOURCES = \
    plugins/e133/E133Device.cpp \
    plugins/e133/E133Device.h \
    plugins/e133/E133Plugin.cpp \
    plugins/e133/E133Plugin.h \
    plugins/e133/E133Port.cpp \
    plugins/e133/E133Port.h \
    plugins/e133/E133PortImpl.cpp \
    plugins/e133/E133PortImpl.h
plugins_e133_libolae133_la_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS) $(uuid_CFLAGS)

#    plugins/e133/messages/libolae133conf.la
plugins_e133_libolae133_la_LIBADD = \
    $(uuid_LIBS) \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la \
    libs/acn/libolaacn.la \
    libs/acn/libolae131core.la \
    libs/acn/libolae133core.la
endif

EXTRA_DIST += plugins/e133/README.md
