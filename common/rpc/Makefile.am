include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libstreamrpcchannel.la
libstreamrpcchannel_la_SOURCES = RpcChannel.cpp RpcController.cpp
nodist_libstreamrpcchannel_la_SOURCES = Rpc.pb.cc
libstreamrpcchannel_la_LIBADD = $(libprotobuf_LIBS)

EXTRA_DIST = Rpc.proto RpcChannel.h RpcController.h RpcHeader.h \
             RpcPeer.h RpcService.h TestService.proto

BUILT_SOURCES = Rpc.pb.cc Rpc.pb.h TestService.pb.cc TestService.pb.h \
                TestServiceService.pb.h TestServiceService.pb.cpp

Rpc.pb.cc Rpc.pb.h: Rpc.proto
	$(PROTOC) --cpp_out ./ Rpc.proto

TestService.pb.cc TestService.pb.h: TestService.proto
	$(PROTOC) --cpp_out ./ TestService.proto

TestServiceService.pb.cpp TestServiceService.pb.h: TestService.proto
	$(OLA_PROTOC) --cppservice_out ./ TestService.proto

if BUILD_TESTS
TESTS = RpcTester
endif
check_PROGRAMS = $(TESTS)

RpcTester_SOURCES = RpcControllerTest.cpp \
                    RpcChannelTest.cpp \
                    RpcHeaderTest.cpp
nodist_RpcTester_SOURCES = TestService.pb.cc \
                           TestServiceService.pb.cpp
RpcTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RpcTester_LDADD = $(COMMON_TESTING_LIBS) \
                  $(libprotobuf_LIBS) \
                  ../libolacommon.la

clean-local:
	rm -f *.pb.{h,cc,cpp}

# this has the problem that it doesn't get rebuilt if a dependency changes.
../network/libnetwork.la:
	cd ../network && $(MAKE) $(AM_MAKEFLAGS) libnetwork.la
