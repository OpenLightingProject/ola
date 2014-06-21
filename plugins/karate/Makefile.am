include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_KARATE
plugin_LTLIBRARIES = libolakarate.la
libolakarate_la_SOURCES = KaratePlugin.cpp KarateDevice.cpp KarateThread.cpp \
                          KarateLight.cpp KaratePlugin.h KarateDevice.h \
                          KaratePort.h KarateThread.h KarateLight.h
libolakarate_la_LIBADD = ../../common/libolacommon.la
endif

EXTRA_DIST = README.protocol
