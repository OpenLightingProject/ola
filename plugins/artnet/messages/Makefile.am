include $(top_srcdir)/common.mk

pkgincludedir = $(includedir)/ola/artnet
lib_LTLIBRARIES = libolaartnetconf.la
nodist_pkginclude_HEADERS = ArtnetConfigMessages.pb.h
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libolaartnetconf.pc

BUILT_SOURCES = ArtnetConfigMessages.pb.cc ArtnetConfigMessages.pb.h

nodist_libolaartnetconf_la_SOURCES = ArtnetConfigMessages.pb.cc

EXTRA_DIST = ArtnetConfigMessages.proto

ArtnetConfigMessages.pb.cc ArtnetConfigMessages.pb.h: ArtnetConfigMessages.proto
	protoc --cpp_out ./ ArtnetConfigMessages.proto

clean-local:
	rm -f *.pb.{h,cc}
