include $(top_srcdir)/common.mk

# The UsbPro plugin config messages. This needs to be available to client
# programs.
EXTRA_DIST = UsbProConfigMessages.proto
CLEANFILES = *.pb.{h,cc}

# pkg-config
##################################################
pkgincludedir = $(includedir)/ola/usbpro
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libolausbproconf.pc

# LIBRARIES
##################################################
lib_LTLIBRARIES = libolausbproconf.la
nodist_pkginclude_HEADERS = UsbProConfigMessages.pb.h

BUILT_SOURCES = UsbProConfigMessages.pb.cc UsbProConfigMessages.pb.h

nodist_libolausbproconf_la_SOURCES = UsbProConfigMessages.pb.cc
libolausbproconf_la_LIBADD = $(libprotobuf_LIBS)

UsbProConfigMessages.pb.cc UsbProConfigMessages.pb.h: UsbProConfigMessages.proto
	$(PROTOC) --cpp_out ./ --proto_path $(srcdir) $(srcdir)/UsbProConfigMessages.proto
