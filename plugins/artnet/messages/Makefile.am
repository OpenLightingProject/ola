include $(top_srcdir)/common.mk

# The ArtNet plugin config messages. This needs to be available to client
# programs.
EXTRA_DIST = ArtnetConfigMessages.proto
CLEANFILES = *.pb.{h,cc}

# pkg-config
##################################################
pkgincludedir = $(includedir)/ola/artnet
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libolaartnetconf.pc

# LIBRARIES
##################################################
if USE_ARTNET
lib_LTLIBRARIES = libolaartnetconf.la
nodist_pkginclude_HEADERS = ArtnetConfigMessages.pb.h

BUILT_SOURCES = ArtnetConfigMessages.pb.cc ArtnetConfigMessages.pb.h

nodist_libolaartnetconf_la_SOURCES = ArtnetConfigMessages.pb.cc
libolaartnetconf_la_LIBADD = $(libprotobuf_LIBS)

ArtnetConfigMessages.pb.cc ArtnetConfigMessages.pb.h: ArtnetConfigMessages.proto
	$(PROTOC) --cpp_out ./ --proto_path $(srcdir) $(srcdir)/ArtnetConfigMessages.proto

endif

