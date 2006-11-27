
libdir = $(plugindir)

lib_LTLIBRARIES = libllausbpro.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CFLAGS = -Wall -Werror -Wformat -W


libllausbpro_la_SOURCES = usbproplugin.cpp usbproplugin.h usbprodevice.cpp\
						 UsbProConfParser.cpp UsbProConfParser.h UsbProConfMessage.h\
						 UsbProWidget.h UsbProWidget.cpp\
						 usbprodevice.h	usbproport.cpp usbproport.h
						 

pkginclude_HEADERS = usbpro_conf_messages.h
pkgincludedir = $(includedir)/lla


