include $(top_srcdir)/common.mk

libdir = $(plugindir)
SUBDIRS = e131 messages
EXTRA_DIST = E131Plugin.h E131Device.h E131Port.h

lib_LTLIBRARIES = libolae131.la
libolae131_la_SOURCES = E131Plugin.cpp E131Device.cpp E131Port.cpp
libolae131_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(UUID_CFLAGS) $(OSSP_UUID_CFLAGS)
libolae131_la_LIBADD = $(UUID_LIBS) $(OSSP_UUID_LIBS) \
                       messages/libolae131conf.la \
                       e131/libolae131core.la
