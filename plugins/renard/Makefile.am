include $(top_srcdir)/common.mk

# LIBRARIES
##################################################
if USE_RENARD
plugin_LTLIBRARIES = libolarenard.la
libolarenard_la_SOURCES = RenardDevice.cpp RenardDevice.h RenardPlugin.cpp \
                          RenardPlugin.h RenardPort.cpp RenardPort.h \
                          RenardWidget.cpp RenardWidget.h
libolarenard_la_LIBADD = ../../common/libolacommon.la
endif
