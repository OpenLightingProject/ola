include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = MilInstPlugin.h MilInstDevice.h MilInstPort.h MilInstWidget.h \
             MilInstWidget1463.h  MilInstWidget1553.h

if USE_MILINST
lib_LTLIBRARIES = libolamilinst.la
libolamilinst_la_SOURCES = MilInstPlugin.cpp MilInstDevice.cpp MilInstPort.cpp \
                           MilInstWidget.cpp MilInstWidget1463.cpp \
                           MilInstWidget1553.cpp
libolamilinst_la_LIBADD = ../../common/libolacommon.la
endif
