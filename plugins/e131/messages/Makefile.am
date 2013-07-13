include $(top_srcdir)/common.mk

pkgincludedir = $(includedir)/ola/e131
lib_LTLIBRARIES = libolae131conf.la
nodist_pkginclude_HEADERS = E131ConfigMessages.pb.h
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libolae131conf.pc

BUILT_SOURCES = E131ConfigMessages.pb.cc E131ConfigMessages.pb.h

nodist_libolae131conf_la_SOURCES = E131ConfigMessages.pb.cc
libolae131conf_la_LIBADD = $(libprotobuf_LIBS)

EXTRA_DIST = E131ConfigMessages.proto

E131ConfigMessages.pb.cc E131ConfigMessages.pb.h: E131ConfigMessages.proto
	$(PROTOC) --cpp_out ./ E131ConfigMessages.proto

clean-local:
	rm -f *.pb.{h,cc}
