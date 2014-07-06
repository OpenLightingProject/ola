# The ArtNet plugin config messages. This needs to be available to client
# programs.
EXTRA_DIST += plugins/artnet/messages/ArtnetConfigMessages.proto

# pkg-config
##################################################
pkgconfig_DATA += plugins/artnet/messages/libolaartnetconf.pc

# LIBRARIES
##################################################
if USE_ARTNET
lib_LTLIBRARIES += plugins/artnet/messages/libolaartnetconf.la
artnetincludedir = $(includedir)/ola/artnet
nodist_artnetinclude_HEADERS = \
    plugins/artnet/messages/ArtnetConfigMessages.pb.h


built_sources += plugins/artnet/messages/ArtnetConfigMessages.pb.cc \
                 plugins/artnet/messages/ArtnetConfigMessages.pb.h

nodist_plugins_artnet_messages_libolaartnetconf_la_SOURCES = \
    plugins/artnet/messages/ArtnetConfigMessages.pb.cc
plugins_artnet_messages_libolaartnetconf_la_LIBADD = $(libprotobuf_LIBS)

plugins/artnet/messages/ArtnetConfigMessages.pb.cc plugins/artnet/messages/ArtnetConfigMessages.pb.h: plugins/artnet/messages/ArtnetConfigMessages.proto
	$(PROTOC) --cpp_out plugins/artnet/messages/ --proto_path $(srcdir)/plugins/artnet/messages $(srcdir)/plugins/artnet/messages/ArtnetConfigMessages.proto

endif

