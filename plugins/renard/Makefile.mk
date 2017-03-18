# LIBRARIES
##################################################
if USE_RENARD
lib_LTLIBRARIES += plugins/renard/libolarenard.la

# Plugin description is generated from README.md
built_sources += plugins/renard/RenardPluginDescription.h
nodist_plugins_renard_libolarenard_la_SOURCES = \
    plugins/renard/RenardPluginDescription.h
plugins/renard/RenardPluginDescription.h: plugins/renard/README.md plugins/renard/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/renard $(top_builddir)/plugins/renard/RenardPluginDescription.h

plugins_renard_libolarenard_la_SOURCES = \
    plugins/renard/RenardDevice.cpp \
    plugins/renard/RenardDevice.h \
    plugins/renard/RenardPlugin.cpp \
    plugins/renard/RenardPlugin.h \
    plugins/renard/RenardPort.cpp \
    plugins/renard/RenardPort.h \
    plugins/renard/RenardWidget.cpp \
    plugins/renard/RenardWidget.h
plugins_renard_libolarenard_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += plugins/renard/README.md
