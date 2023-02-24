EXTRA_DIST += common/protocol/Ola.proto

# The .h files are included elsewhere so we have to put them in BUILT_SOURCES
built_sources += \
    common/protocol/Ola.pb.cc \
    common/protocol/Ola.pb.h \
    common/protocol/OlaService.pb.h \
    common/protocol/OlaService.pb.cpp

# LIBRARIES
##################################################

# We build the .cc files as a separate unit because they aren't warning-clean.
noinst_LTLIBRARIES += common/protocol/libolaproto.la
nodist_common_protocol_libolaproto_la_SOURCES = \
    common/protocol/Ola.pb.cc \
    common/protocol/OlaService.pb.cpp
common_protocol_libolaproto_la_LIBADD = $(libprotobuf_LIBS)
# required, otherwise we get build errors
common_protocol_libolaproto_la_CXXFLAGS = $(COMMON_CXXFLAGS_ONLY_WARNINGS)

common_libolacommon_la_LIBADD += common/protocol/libolaproto.la

common/protocol/Ola.pb.cc common/protocol/Ola.pb.h: common/protocol/Makefile.mk common/protocol/Ola.proto
	$(PROTOC) --cpp_out $(top_builddir)/common/protocol --proto_path $(srcdir)/common/protocol $(srcdir)/common/protocol/Ola.proto

common/protocol/OlaService.pb.cpp common/protocol/OlaService.pb.h: common/protocol/Makefile.mk common/protocol/Ola.proto protoc/ola_protoc_plugin$(EXEEXT)
	$(OLA_PROTOC) --cppservice_out $(top_builddir)/common/protocol --proto_path $(srcdir)/common/protocol $(srcdir)/common/protocol/Ola.proto
