

lib_LTLIBRARIES = 
include ./Makefile_Conf_Msg.am

AM_CXXFLAGS = -Wall -Wformat -W

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

pkginclude_HEADERS = $(USBPRO_CONF_MSG_HEADERS)
pkgincludedir = $(includedir)/lla/artnet

pkgconfig_DATA = libllaartnetconf.pc
pkgconfigdir = $(libdir)/pkgconfig
