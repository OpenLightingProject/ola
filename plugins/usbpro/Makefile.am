
libdir = $(plugindir)

lib_LTLIBRARIES = libllausbpro.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir)
AM_CFLAGS = -Wall -Werror -Wformat -W


libllausbpro_la_SOURCES = usbproplugin.cpp usbproplugin.h usbprodevice.cpp\
						 UsbProConfParser.cpp UsbProConfParser.h UsbProConfMessage.h\
						 usbprodevice.h	usbproport.cpp usbproport.h

pkginclude_HEADERS = usbpro_messages.h
pkgincludedir = $(includedir)/lla


