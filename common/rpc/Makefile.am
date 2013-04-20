include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libstreamrpcchannel.la
libstreamrpcchannel_la_SOURCES = StreamRpcChannel.cpp SimpleRpcController.cpp
nodist_libstreamrpcchannel_la_SOURCES = Rpc.pb.cc
libstreamrpcchannel_la_LIBADD = $(libprotobuf_LIBS)

EXTRA_DIST = Rpc.proto TestService.proto SimpleRpcController.h \
             StreamRpcChannel.h

BUILT_SOURCES = Rpc.pb.cc Rpc.pb.h TestService.pb.cc TestService.pb.h

Rpc.pb.cc Rpc.pb.h: Rpc.proto
	$(PROTOC) --cpp_out ./ Rpc.proto

TestService.pb.cc TestService.pb.h: TestService.proto
	$(PROTOC) --cpp_out ./ TestService.proto

if BUILD_TESTS
TESTS = RpcTester
endif
check_PROGRAMS = $(TESTS)

RpcTester_SOURCES = RpcControllerTest.cpp \
                    StreamRpcChannelTest.cpp \
                    StreamRpcHeaderTest.cpp
nodist_RpcTester_SOURCES = TestService.pb.cc
RpcTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RpcTester_LDADD = $(COMMON_TESTING_LIBS) \
                  ../libolacommon.la \
                  ../testing/libtestmain.la

clean-local:
	rm -f *.pb.{h,cc}

# this has the problem that it doesn't get rebuilt if a dependency changes.
../network/libnetwork.la:
	cd ../network && $(MAKE) $(AM_MAKEFLAGS) libnetwork.la
