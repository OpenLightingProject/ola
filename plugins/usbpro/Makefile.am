

libdir = $(plugindir)

lib_LTLIBRARIES = libllausbpro.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

libllausbpro_la_SOURCES = usbproplugin.cpp usbproplugin.h usbprodevice.cpp\
						 usbprodevice.h UsbProWidget.h UsbProWidget.cpp\
						 usbproport.cpp usbproport.h

libllausbpro_la_LIBADD = -lllausbproconf


SUBDIRS = conf_msgs

