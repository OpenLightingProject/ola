include $(top_srcdir)/common.mk

SUBDIRS = e131 messages

# LIBRARIES
##################################################

if USE_E131
plugin_LTLIBRARIES = libolae131.la
libolae131_la_SOURCES = E131Device.cpp E131Device.h E131Plugin.cpp \
                        E131Plugin.h E131Port.cpp E131Port.h
libolae131_la_LIBADD = messages/libolae131conf.la \
                       e131/libolae131core.la
endif
