include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = MilInstPlugin.h MilInstDevice.h MilInstPort.h \
             MilInstWidget.h MilInstWidget1463.h

if USE_MILINST
lib_LTLIBRARIES = libolamilinst.la
libolamilinst_la_SOURCES = MilInstPlugin.cpp MilInstDevice.cpp \
                           MilInstPort.cpp MilInstWidget.cpp \
                           MilInstWidget1463.cpp
libolamilinst_la_LIBADD = ../../common/libolacommon.la
endif
