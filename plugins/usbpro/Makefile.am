
libdir = $(plugindir)

lib_LTLIBRARIES = libllausbpro.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include -I./conf_msgs
AM_CXXFLAGS = -Wall -Wformat -W

libllausbpro_la_SOURCES = UsbProPlugin.cpp UsbProPlugin.h UsbProDevice.cpp\
						 UsbProDevice.h UsbProWidget.cpp\
						 UsbProPort.cpp UsbProPort.h

libllausbpro_la_LIBADD = ./conf_msgs/libllausbproconf.la

EXTRA_DIST = ConfMessages.xml UsbProWidgetListener.h UsbProWidget.h

SUBDIRS = conf_msgs

