# LIBRARIES
##################################################
if USE_MILINST
lib_LTLIBRARIES += plugins/milinst/libolamilinst.la

# Plugin description is generated from README.md
built_sources += plugins/milinst/MilInstPluginDescription.h
nodist_plugins_milinst_libolamilinst_la_SOURCES = \
    plugins/milinst/MilInstPluginDescription.h
plugins/milinst/MilInstPluginDescription.h: plugins/milinst/README.md plugins/milinst/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/milinst $(top_builddir)/plugins/milinst/MilInstPluginDescription.h

plugins_milinst_libolamilinst_la_SOURCES = \
    plugins/milinst/MilInstDevice.cpp \
    plugins/milinst/MilInstDevice.h \
    plugins/milinst/MilInstPlugin.cpp \
    plugins/milinst/MilInstPlugin.h \
    plugins/milinst/MilInstPort.cpp \
    plugins/milinst/MilInstPort.h \
    plugins/milinst/MilInstWidget.cpp \
    plugins/milinst/MilInstWidget.h \
    plugins/milinst/MilInstWidget1463.cpp \
    plugins/milinst/MilInstWidget1463.h \
    plugins/milinst/MilInstWidget1553.cpp \
    plugins/milinst/MilInstWidget1553.h
plugins_milinst_libolamilinst_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
endif

EXTRA_DIST += plugins/milinst/README.md
