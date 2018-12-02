# LIBRARIES
##################################################
if USE_FTDI
lib_LTLIBRARIES += plugins/ftdidmx/libolaftdidmx.la

# Plugin description is generated from README.md
built_sources += plugins/ftdidmx/FtdiDmxPluginDescription.h
nodist_plugins_ftdidmx_libolaftdidmx_la_SOURCES = \
    plugins/ftdidmx/FtdiDmxPluginDescription.h
plugins/ftdidmx/FtdiDmxPluginDescription.h: plugins/ftdidmx/README.md plugins/ftdidmx/Makefile.mk plugins/convert_README_to_header.sh
	sh $(top_srcdir)/plugins/convert_README_to_header.sh $(top_srcdir)/plugins/ftdidmx $(top_builddir)/plugins/ftdidmx/FtdiDmxPluginDescription.h

plugins_ftdidmx_libolaftdidmx_la_SOURCES = \
    plugins/ftdidmx/FtdiDmxDevice.cpp \
    plugins/ftdidmx/FtdiDmxDevice.h \
    plugins/ftdidmx/FtdiDmxPlugin.cpp \
    plugins/ftdidmx/FtdiDmxPlugin.h \
    plugins/ftdidmx/FtdiDmxPort.h \
    plugins/ftdidmx/FtdiDmxThread.cpp \
    plugins/ftdidmx/FtdiDmxThread.h \
    plugins/ftdidmx/FtdiWidget.cpp \
    plugins/ftdidmx/FtdiWidget.h
plugins_ftdidmx_libolaftdidmx_la_LIBADD = \
    common/libolacommon.la \
    olad/plugin_api/libolaserverplugininterface.la
if HAVE_LIBFTDI1
plugins_ftdidmx_libolaftdidmx_la_LIBADD += $(libftdi1_LIBS)
else
plugins_ftdidmx_libolaftdidmx_la_LIBADD += $(libftdi0_LIBS)
endif

endif

EXTRA_DIST += plugins/ftdidmx/README.md
