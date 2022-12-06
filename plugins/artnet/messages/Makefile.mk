# The Art-Net plugin config messages. This needs to be available to client
# programs.
EXTRA_DIST += plugins/artnet/messages/ArtNetConfigMessages.proto

# pkg-config
##################################################
pkgconfig_DATA += plugins/artnet/messages/libolaartnetconf.pc

# LIBRARIES
##################################################
if USE_ARTNET
lib_LTLIBRARIES += plugins/artnet/messages/libolaartnetconf.la
artnetincludedir = $(includedir)/ola/artnet
nodist_artnetinclude_HEADERS = \
    plugins/artnet/messages/ArtNetConfigMessages.pb.h


built_sources += plugins/artnet/messages/ArtNetConfigMessages.pb.cc \
                 plugins/artnet/messages/ArtNetConfigMessages.pb.h

nodist_plugins_artnet_messages_libolaartnetconf_la_SOURCES = \
    plugins/artnet/messages/ArtNetConfigMessages.pb.cc
plugins_artnet_messages_libolaartnetconf_la_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
plugins_artnet_messages_libolaartnetconf_la_LIBADD = $(libprotobuf_LIBS)

plugins/artnet/messages/ArtNetConfigMessages.pb.cc plugins/artnet/messages/ArtNetConfigMessages.pb.h: plugins/artnet/messages/Makefile.mk plugins/artnet/messages/ArtNetConfigMessages.proto
	$(PROTOC) --cpp_out $(top_builddir)/plugins/artnet/messages/ --proto_path $(srcdir)/plugins/artnet/messages $(srcdir)/plugins/artnet/messages/ArtNetConfigMessages.proto

endif

