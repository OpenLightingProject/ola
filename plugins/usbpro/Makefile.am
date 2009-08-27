libdir = $(plugindir)
SUBDIRS = messages
EXTRA_DIST = UsbProPlugin.h UsbProDevice.h UsbProPort.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

lib_LTLIBRARIES = libolausbpro.la
libolausbpro_la_SOURCES = UsbProPlugin.cpp UsbProDevice.cpp UsbProPort.cpp
libolausbpro_la_LIBADD = ./libolausbprowidget.la messages/libolausbproconf.la

noinst_LTLIBRARIES = libolausbprowidget.la
libolausbprowidget_la_SOURCES = UsbProWidget.cpp UsbProWidget.h \
                                UsbProWidgetListener.h
