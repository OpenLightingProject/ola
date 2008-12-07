
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

noinst_LTLIBRARIES = libstreamrpcchannel.la
libstreamrpcchannel_la_SOURCES = StreamRpcChannel.cpp SimpleRpcController.cpp
nodist_libstreamrpcchannel_la_SOURCES = Rpc.pb.cc

EXTRA_DIST = Rpc.proto TestService.proto SimpleRpcController.h \
             StreamRpcChannel.h TestServiceImpl.h

BUILT_SOURCES = Rpc.pb.cc Rpc.pb.h TestService.pb.cc TestService.pb.h

Rpc.pb.cc Rpc.pb.h: Rpc.proto
	protoc --cpp_out ./ Rpc.proto

TestService.pb.cc TestService.pb.h: TestService.proto
	protoc --cpp_out ./ TestService.proto

TESTS = RpcTester
check_PROGRAMS = $(TESTS)
RpcTester_SOURCES = RpcTester.cpp RpcControllerTest.cpp\
                    StreamRpcChannelTest.cpp StreamRpcHeaderTest.cpp\
                    TestServiceImpl.cpp
nodist_RpcTester_SOURCES = TestService.pb.cc
RpcTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
RpcTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl -lprotobuf
RpcTester_LDADD = ./libstreamrpcchannel.la ../select_server/libselectserver.la

clean-local:
	rm *.pb.{h,cc}

# this has the problem that it doesn't get rebuilt if a dependency changes.
../select_server/libselectserver.la:
	cd ../select_server && $(MAKE) $(AM_MAKEFLAGS) libselectserver.la
