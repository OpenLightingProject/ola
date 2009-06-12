libdir = $(plugindir)

SUBDIRS = messages
lib_LTLIBRARIES = libllausbpro.la
noinst_LTLIBRARIES = libllausbprowidget.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

libllausbpro_la_SOURCES = UsbProPlugin.cpp UsbProDevice.cpp UsbProPort.cpp
libllausbpro_la_LIBADD = ./libllausbprowidget.la messages/libllausbproconf.la

libllausbprowidget_la_SOURCES = UsbProWidget.cpp UsbProWidget.h \
                                UsbProWidgetListener.h

EXTRA_DIST = UsbProPlugin.h UsbProDevice.h UsbProPort.h
