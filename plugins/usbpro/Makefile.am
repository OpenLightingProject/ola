libdir = $(plugindir)

SUBDIRS = messages
lib_LTLIBRARIES = libolausbpro.la
noinst_LTLIBRARIES = libolausbprowidget.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

libolausbpro_la_SOURCES = UsbProPlugin.cpp UsbProDevice.cpp UsbProPort.cpp
libolausbpro_la_LIBADD = ./libolausbprowidget.la messages/libolausbproconf.la

libolausbprowidget_la_SOURCES = UsbProWidget.cpp UsbProWidget.h \
                                UsbProWidgetListener.h

EXTRA_DIST = UsbProPlugin.h UsbProDevice.h UsbProPort.h
