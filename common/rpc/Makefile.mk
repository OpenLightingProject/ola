BUILT_SOURCES += \
    common/rpc/Rpc.pb.cc \
    common/rpc/Rpc.pb.h \
    common/rpc/TestService.pb.cc \
    common/rpc/TestService.pb.h \
    common/rpc/TestServiceService.pb.cpp \
    common/rpc/TestServiceService.pb.h

# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/rpc/RpcChannel.cpp \
    common/rpc/RpcChannel.h \
    common/rpc/RpcController.cpp \
    common/rpc/RpcController.h \
    common/rpc/RpcHeader.h \
    common/rpc/RpcPeer.h \
    common/rpc/RpcService.h
nodist_common_libolacommon_la_SOURCES += common/rpc/Rpc.pb.cc
common_libolacommon_la_LIBADD += $(libprotobuf_LIBS)

EXTRA_DIST += \
    common/rpc/Rpc.proto \
    common/rpc/TestService.proto

common/rpc/Rpc.pb.cc common/rpc/Rpc.pb.h: common/rpc/Rpc.proto
	$(PROTOC) --cpp_out common/rpc --proto_path $(srcdir)/common/rpc $(srcdir)/common/rpc/Rpc.proto

common/rpc/TestService.pb.cc common/rpc/TestService.pb.h: common/rpc/TestService.proto
	$(PROTOC) --cpp_out common/rpc --proto_path $(srcdir)/common/rpc $(srcdir)/common/rpc/TestService.proto

common/rpc/TestServiceService.pb.cpp common/rpc/TestServiceService.pb.h: common/rpc/TestService.proto protoc/ola_protoc
	$(OLA_PROTOC) --cppservice_out common/rpc --proto_path $(srcdir)/common/rpc $(srcdir)/common/rpc/TestService.proto

CLEANFILES += common/rpc/*.pb.{h,cc,cpp}

# TESTS
##################################################
test_programs += common/rpc/RpcTester

common_rpc_RpcTester_SOURCES = \
    common/rpc/RpcControllerTest.cpp \
    common/rpc/RpcChannelTest.cpp \
    common/rpc/RpcHeaderTest.cpp
nodist_common_rpc_RpcTester_SOURCES = \
    common/rpc/TestService.pb.cc \
    common/rpc/TestServiceService.pb.cpp
common_rpc_RpcTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_rpc_RpcTester_LDADD = $(COMMON_TESTING_LIBS) \
                             $(libprotobuf_LIBS)
