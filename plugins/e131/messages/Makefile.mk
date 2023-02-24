# The E1.31 plugin config messages. This needs to be available to client
# programs.
EXTRA_DIST += plugins/e131/messages/E131ConfigMessages.proto

# pkg-config
##################################################
pkgconfig_DATA += plugins/e131/messages/libolae131conf.pc

# LIBRARIES
##################################################
if USE_E131
lib_LTLIBRARIES += plugins/e131/messages/libolae131conf.la
e131includedir = $(includedir)/ola/e131
nodist_e131include_HEADERS = \
    plugins/e131/messages/E131ConfigMessages.pb.h

built_sources += plugins/e131/messages/E131ConfigMessages.pb.cc \
                 plugins/e131/messages/E131ConfigMessages.pb.h

nodist_plugins_e131_messages_libolae131conf_la_SOURCES = \
    plugins/e131/messages/E131ConfigMessages.pb.cc
plugins_e131_messages_libolae131conf_la_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
plugins_e131_messages_libolae131conf_la_LIBADD = $(libprotobuf_LIBS)

plugins/e131/messages/E131ConfigMessages.pb.cc plugins/e131/messages/E131ConfigMessages.pb.h: plugins/e131/messages/Makefile.mk plugins/e131/messages/E131ConfigMessages.proto
	$(PROTOC) --cpp_out $(top_builddir)/plugins/e131/messages/ --proto_path $(srcdir)/plugins/e131/messages/ $(srcdir)/plugins/e131/messages/E131ConfigMessages.proto

endif
