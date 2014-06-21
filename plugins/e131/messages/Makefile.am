include $(top_srcdir)/common.mk

# The E1.31 plugin config messages. This needs to be available to client
# programs.
EXTRA_DIST = E131ConfigMessages.proto
CLEANFILES = *.pb.{h,cc}

# pkg-config
##################################################
pkgincludedir = $(includedir)/ola/e131
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libolae131conf.pc

# LIBRARIES
##################################################
lib_LTLIBRARIES = libolae131conf.la
nodist_pkginclude_HEADERS = E131ConfigMessages.pb.h

BUILT_SOURCES = E131ConfigMessages.pb.cc E131ConfigMessages.pb.h

nodist_libolae131conf_la_SOURCES = E131ConfigMessages.pb.cc
libolae131conf_la_LIBADD = $(libprotobuf_LIBS)

E131ConfigMessages.pb.cc E131ConfigMessages.pb.h: E131ConfigMessages.proto
	$(PROTOC) --cpp_out ./ --proto_path $(srcdir) $(srcdir)/E131ConfigMessages.proto

