built_sources += \
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
    common/rpc/RpcSession.h \
    common/rpc/RpcController.cpp \
    common/rpc/RpcController.h \
    common/rpc/RpcHeader.h \
    common/rpc/RpcPeer.h \
    common/rpc/RpcServer.cpp \
    common/rpc/RpcServer.h \
    common/rpc/RpcService.h
nodist_common_libolacommon_la_SOURCES += common/rpc/Rpc.pb.cc
common_libolacommon_la_LIBADD += $(libprotobuf_LIBS)

EXTRA_DIST += \
    common/rpc/Rpc.proto \
    common/rpc/TestService.proto

common/rpc/Rpc.pb.cc common/rpc/Rpc.pb.h: common/rpc/Makefile.mk common/rpc/Rpc.proto
	$(PROTOC) --cpp_out $(top_builddir)/common/rpc --proto_path $(srcdir)/common/rpc $(srcdir)/common/rpc/Rpc.proto

common/rpc/TestService.pb.cc common/rpc/TestService.pb.h: common/rpc/Makefile.mk common/rpc/TestService.proto
	$(PROTOC) --cpp_out $(top_builddir)/common/rpc --proto_path $(srcdir)/common/rpc $(srcdir)/common/rpc/TestService.proto

common/rpc/TestServiceService.pb.cpp common/rpc/TestServiceService.pb.h: common/rpc/Makefile.mk common/rpc/TestService.proto protoc/ola_protoc_plugin$(EXEEXT)
	$(OLA_PROTOC) --cppservice_out $(top_builddir)/common/rpc --proto_path $(srcdir)/common/rpc $(srcdir)/common/rpc/TestService.proto

# TESTS
##################################################
test_programs += common/rpc/RpcTester common/rpc/RpcServerTester

common_rpc_TEST_SOURCES = \
    common/rpc/TestService.h \
    common/rpc/TestService.cpp

common_rpc_RpcTester_SOURCES = \
    common/rpc/RpcControllerTest.cpp \
    common/rpc/RpcChannelTest.cpp \
    common/rpc/RpcHeaderTest.cpp \
    $(common_rpc_TEST_SOURCES)
nodist_common_rpc_RpcTester_SOURCES = \
    common/rpc/TestService.pb.cc \
    common/rpc/TestServiceService.pb.cpp
# required, otherwise we get build errors
common_rpc_RpcTester_CXXFLAGS = $(COMMON_TESTING_FLAGS_ONLY_WARNINGS)
common_rpc_RpcTester_LDADD = $(COMMON_TESTING_LIBS) \
                             $(libprotobuf_LIBS)

common_rpc_RpcServerTester_SOURCES = \
    common/rpc/RpcServerTest.cpp \
    $(common_rpc_TEST_SOURCES)
nodist_common_rpc_RpcServerTester_SOURCES = \
    common/rpc/TestService.pb.cc \
    common/rpc/TestServiceService.pb.cpp
# required, otherwise we get build errors
common_rpc_RpcServerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS_ONLY_WARNINGS)
common_rpc_RpcServerTester_LDADD = $(COMMON_TESTING_LIBS) \
                                   $(libprotobuf_LIBS)
