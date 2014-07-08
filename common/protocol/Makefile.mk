EXTRA_DIST += common/protocol/Ola.proto

# The .h files are included elsewhere so we have to put them in BUILT_SOURCES
built_sources += \
    common/protocol/Ola.pb.cc \
    common/protocol/Ola.pb.h \
    common/protocol/OlaService.pb.h \
    common/protocol/OlaService.pb.cpp

# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/protocol/Ola.pb.cc \
    common/protocol/OlaService.pb.cpp
common_libolacommon_la_LIBADD += $(libprotobuf_LIBS)
# required, otherwise we get build errors
# TODO(simonn): this disables warnings for the entire libolacommon. That sucks.
# Find a better way to do this.
# common_libolaproto_la_CXXFLAGS = $(COMMON_CXXFLAGS_ONLY_WARNINGS)

common/protocol/Ola.pb.cc common/protocol/Ola.pb.h: common/protocol/Ola.proto
	$(PROTOC) --cpp_out common/protocol --proto_path $(srcdir)/common/protocol $(srcdir)/common/protocol/Ola.proto

common/protocol/OlaService.pb.cpp common/protocol/OlaService.pb.h: common/protocol/Ola.proto protoc/ola_protoc$(EXEEXT)
	$(OLA_PROTOC)  --cppservice_out common/protocol --proto_path $(srcdir)/common/protocol $(srcdir)/common/protocol/Ola.proto
