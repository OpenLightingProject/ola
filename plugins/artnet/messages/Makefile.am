
pkgincludedir = $(includedir)/ola/artnet
lib_LTLIBRARIES = libolaartnetconf.la
pkginclude_HEADERS = ArtnetConfigMessages.pb.h
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libolaartnetconf.pc

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

BUILT_SOURCES = ArtnetConfigMessages.pb.cc ArtnetConfigMessages.pb.h

libolaartnetconf_la_SOURCES = ArtnetConfigMessages.pb.cc

EXTRA_DIST = ArtnetConfigMessages.proto

ArtnetConfigMessages.pb.cc ArtnetConfigMessages.pb.h: ArtnetConfigMessages.proto
	protoc --cpp_out ./ ArtnetConfigMessages.proto

clean-local:
	rm -f *.pb.{h,cc}
