
libdir = $(plugindir)

lib_LTLIBRARIES = libllausbpro.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include -I./conf_msgs
AM_CXXFLAGS = -Wall -Wformat -W

libllausbpro_la_SOURCES = usbproplugin.cpp usbproplugin.h usbprodevice.cpp\
						 usbprodevice.h UsbProWidget.h UsbProWidget.cpp\
						 usbproport.cpp usbproport.h

libllausbpro_la_LIBADD = -L./conf_msgs/.libs -lllausbproconf


SUBDIRS = conf_msgs

