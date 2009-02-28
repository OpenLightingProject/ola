libdir = $(plugindir)

SUBDIRS = messages
lib_LTLIBRARIES = libllausbpro.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

libllausbpro_la_SOURCES = UsbProPlugin.cpp UsbProDevice.cpp\
                          UsbProWidget.cpp UsbProPort.cpp
libllausbpro_la_LIBADD = messages/libllausbproconf.la

EXTRA_DIST = UsbProWidgetListener.h UsbProWidget.h \
             UsbProPlugin.h UsbProDevice.h UsbProPort.h
