include $(top_srcdir)/common.mk

libdir = $(plugindir)

EXTRA_DIST = KaratePlugin.h KarateDevice.h KaratePort.h KarateThread.h Kl.h

lib_LTLIBRARIES = libolakarate.la
libolakarate_la_SOURCES =  KaratePlugin.cpp KarateDevice.cpp \
                            KarateThread.cpp Kl.cpp
libolakarate_la_LIBADD = ../../common/libolacommon.la
