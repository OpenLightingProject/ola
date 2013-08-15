include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = KaratePlugin.h KarateDevice.h KaratePort.h KarateThread.h KarateLight.h

if USE_KARATE
lib_LTLIBRARIES = libolakarate.la
libolakarate_la_SOURCES =  KaratePlugin.cpp KarateDevice.cpp \
                           KarateThread.cpp KarateLight.cpp
libolakarate_la_LIBADD = ../../common/libolacommon.la

endif
