# The UsbPro plugin config messages. This needs to be available to client
# programs.
EXTRA_DIST += plugins/usbpro/messages/UsbProConfigMessages.proto

# pkg-config
##################################################
pkgconfig_DATA += plugins/usbpro/messages/libolausbproconf.pc

# LIBRARIES
##################################################
lib_LTLIBRARIES += plugins/usbpro/messages/libolausbproconf.la
usbproincludedir = $(includedir)/ola/usbpro
nodist_usbproinclude_HEADERS = \
    plugins/usbpro/messages/UsbProConfigMessages.pb.h

built_sources += plugins/usbpro/messages/UsbProConfigMessages.pb.cc \
                 plugins/usbpro/messages/UsbProConfigMessages.pb.h

nodist_plugins_usbpro_messages_libolausbproconf_la_SOURCES = \
    plugins/usbpro/messages/UsbProConfigMessages.pb.cc
plugins_usbpro_messages_libolausbproconf_la_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
plugins_usbpro_messages_libolausbproconf_la_LIBADD = $(libprotobuf_LIBS)

plugins/usbpro/messages/UsbProConfigMessages.pb.cc plugins/usbpro/messages/UsbProConfigMessages.pb.h: plugins/usbpro/messages/Makefile.mk plugins/usbpro/messages/UsbProConfigMessages.proto
	$(PROTOC) --cpp_out $(top_builddir)/plugins/usbpro/messages/ --proto_path $(srcdir)/plugins/usbpro/messages/ $(srcdir)/plugins/usbpro/messages/UsbProConfigMessages.proto
