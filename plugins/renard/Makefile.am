include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = RenardPlugin.h RenardDevice.h RenardPort.h \
             RenardWidget.h RenardWidgetSS8.h

if USE_RENARD
lib_LTLIBRARIES = libolarenard.la
libolarenard_la_SOURCES = RenardPlugin.cpp RenardDevice.cpp \
                           RenardPort.cpp RenardWidget.cpp \
                           RenardWidgetSS8.cpp
libolarenard_la_LIBADD = ../../common/libolacommon.la
endif
