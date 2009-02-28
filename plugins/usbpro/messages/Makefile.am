
pkgincludedir = $(includedir)/lla/usbpro
lib_LTLIBRARIES = libllausbproconf.la
pkginclude_HEADERS = UsbProConfigMessages.pb.h
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libllausbproconf.pc

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

BUILT_SOURCES = UsbProConfigMessages.pb.cc UsbProConfigMessages.pb.h

libllausbproconf_la_SOURCES = UsbProConfigMessages.pb.cc

EXTRA_DIST = UsbProConfigMessages.proto

UsbProConfigMessages.pb.cc UsbProConfigMessages.pb.h: UsbProConfigMessages.proto
	protoc --cpp_out ./ UsbProConfigMessages.proto

clean-local:
	rm -f *.pb.{h,cc}
