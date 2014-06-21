include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_MILINST
plugin_LTLIBRARIES = libolamilinst.la
libolamilinst_la_SOURCES = MilInstDevice.cpp MilInstDevice.h \
                           MilInstPlugin.cpp MilInstPlugin.h \
                           MilInstPort.cpp MilInstPort.h MilInstWidget.cpp \
                           MilInstWidget.h MilInstWidget1463.cpp \
                           MilInstWidget1463.h MilInstWidget1553.cpp \
                           MilInstWidget1553.h
libolamilinst_la_LIBADD = ../../common/libolacommon.la
endif
