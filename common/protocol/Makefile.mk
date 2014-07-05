EXTRA_DIST += common/protocol/Ola.proto

BUILT_SOURCES += \
    common/protocol/Ola.pb.cc \
    common/protocol/Ola.pb.h \
    common/protocol/OlaService.pb.h \
    common/protocol/OlaService.pb.cpp

# LIBRARIES
##################################################
libolacommon_la_SOURCES += \
    common/protocol/Ola.pb.cc \
    common/protocol/OlaService.pb.cpp
libolacommon_la_LIBADD += $(libprotobuf_LIBS)

# required, otherwise we get build errors
# TODO(simon): fix me
#libolaproto_la_CXXFLAGS = $(WARNING_CXXFLAGS)

common/protocol/Ola.pb.cc common/protocol/Ola.pb.h: common/protocol/Ola.proto
	$(PROTOC) --cpp_out common/protocol --proto_path $(srcdir)/common/protocol $(srcdir)/common/protocol/Ola.proto

common/protocol/OlaService.pb.cpp common/protocol/OlaService.pb.h: common/protocol/Ola.proto protoc/ola_protoc
	$(OLA_PROTOC)  --cppservice_out common/protocol --proto_path $(srcdir)/common/protocol $(srcdir)/common/protocol/Ola.proto

CLEANFILES += common/protocol/*.pb.{h,cc,cpp}
