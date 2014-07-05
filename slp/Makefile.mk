CLEANFILES += slp/*.pb.{h,cc,cpp} slp/*.gcno slp/*.gcda

EXTRA_DIST += slp/SLP.proto
noinst_HEADERS += slp/URLListVerifier.h slp/SLPServerTestHelper.h

# pkg-config
##################################################
pkgconfig_DATA += slp/libolaslpclient.pc slp/libolaslpserver.pc

# LIBRARIES
##################################################
noinst_LTLIBRARIES += slp/libolaslpproto.la
lib_LTLIBRARIES += slp/libolaslpcore.la \
                   slp/libolaslpserver.la \
                   slp/libolaslpclient.la

# libolaslpproto
nodist_slp_libolaslpproto_la_SOURCES = slp/SLP.pb.cc slp/SLPService.pb.cpp
slp_libolaslpproto_la_LIBADD = $(libprotobuf_LIBS)
# Required, otherwise we get build errors
slp_libolaslpproto_la_CXXFLAGS = $(WARNING_CXXFLAGS)

BUILT_SOURCES += \
    slp/SLP.pb.cc \
    slp/SLP.pb.h \
    slp/SLPService.pb.h \
    slp/SLPService.pb.h

slp/SLP.pb.cc slp/SLP.pb.h: slp/Makefile.mk slp/SLP.proto
	$(PROTOC) --cpp_out slp/ --proto_path $(srcdir)/slp $(srcdir)/slp/SLP.proto

slp/SLPService.pb.cpp slp/SLPService.pb.h: slp/Makefile.mk slp/SLP.proto protoc/ola_protoc
	$(OLA_PROTOC)  --cppservice_out slp/ --proto_path $(srcdir)/slp $(srcdir)/slp/SLP.proto

# libolaslpcore
slp_libolaslpcore_la_SOURCES = \
    slp/SLPUtil.cpp \
    slp/SLPUtil.h \
    slp/URLEntry.cpp

# libolaslpserver
# This is an SLP server, without the RPC interface.
slp_libolaslpserver_la_SOURCES = \
    slp/DATracker.cpp \
    slp/DATracker.h \
    slp/RegistrationFileParser.cpp \
    slp/RegistrationFileParser.h \
    slp/SLPPacketBuilder.cpp \
    slp/SLPPacketBuilder.h \
    slp/SLPPacketConstants.h \
    slp/SLPPacketParser.cpp \
    slp/SLPPacketParser.h \
    slp/SLPPendingOperations.cpp \
    slp/SLPPendingOperations.h \
    slp/SLPServer.cpp \
    slp/SLPServer.h \
    slp/SLPStore.cpp \
    slp/SLPStore.h \
    slp/SLPStrings.cpp \
    slp/SLPStrings.h \
    slp/SLPUDPSender.cpp \
    slp/SLPUDPSender.h \
    slp/ScopeSet.cpp \
    slp/ScopeSet.h \
    slp/ServerCommon.h \
    slp/ServiceEntry.h \
    slp/XIDAllocator.h
slp_libolaslpserver_la_LIBADD = common/libolacommon.la \
                                slp/libolaslpcore.la

# libolaslpclient
slp_libolaslpclient_la_SOURCES = \
    slp/SLPClient.cpp \
    slp/SLPClientCore.cpp \
    slp/SLPClientCore.h
slp_libolaslpclient_la_LIBADD = common/libolacommon.la \
                                ola/libola.la \
                                slp/libolaslpproto.la \
                                slp/libolaslpcore.la

# PROGRAMS
##################################################
noinst_PROGRAMS += slp/slp_client

if HAVE_LIBMICROHTTPD
noinst_PROGRAMS += slp/slp_server
endif

slp_slp_server_SOURCES = \
    slp/SLPDaemon.cpp \
    slp/SLPDaemon.h \
    slp/slp-server.cpp
slp_slp_server_LDADD = common/libolacommon.la \
                       common/http/libolahttp.la \
                       slp/libolaslpproto.la \
                       slp/libolaslpserver.la

slp_slp_client_SOURCES = slp/slp-client.cpp
slp_slp_client_LDADD = common/libolacommon.la \
                       ola/libola.la \
                       slp/libolaslpclient.la \
                       slp/libolaslpcore.la

# TESTS
##################################################
tests += \
    slp/DATrackerTester \
    slp/PacketBuilderTester \
    slp/PacketParserTester \
    slp/RegistrationFileParserTester \
    slp/SLPServerDATester \
    slp/SLPServerNetworkTester \
    slp/SLPServerSATester \
    slp/SLPServerUATester \
    slp/SLPStoreTester \
    slp/SLPStringsTester \
    slp/ScopeSetTester \
    slp/ServiceEntryTester \
    slp/XIDAllocatorTester

COMMON_SLP_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    common/libolacommon.la \
                    slp/libolaslpcore.la \
                    slp/libolaslpserver.la

slp_DATrackerTester_SOURCES = slp/DATrackerTest.cpp
slp_DATrackerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_DATrackerTester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_PacketBuilderTester_SOURCES = slp/PacketBuilderTest.cpp
slp_PacketBuilderTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_PacketBuilderTester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_PacketParserTester_SOURCES = slp/PacketParserTest.cpp
slp_PacketParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_PacketParserTester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_RegistrationFileParserTester_SOURCES = slp/RegistrationFileParserTest.cpp
slp_RegistrationFileParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_RegistrationFileParserTester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_SLPServerDATester_SOURCES = slp/SLPServerTestHelper.cpp \
                                slp/SLPServerDATest.cpp
slp_SLPServerDATester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_SLPServerDATester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_SLPServerSATester_SOURCES = slp/SLPServerTestHelper.cpp \
                                slp/SLPServerSATest.cpp
slp_SLPServerSATester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_SLPServerSATester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_SLPServerUATester_SOURCES = slp/SLPServerTestHelper.cpp \
                                slp/SLPServerUATest.cpp
slp_SLPServerUATester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_SLPServerUATester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_SLPServerNetworkTester_SOURCES = slp/SLPServerTestHelper.cpp \
                                     slp/SLPServerNetworkTest.cpp
slp_SLPServerNetworkTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_SLPServerNetworkTester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_SLPStoreTester_SOURCES = slp/SLPStoreTest.cpp
slp_SLPStoreTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_SLPStoreTester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_SLPStringsTester_SOURCES = slp/SLPStringsTest.cpp
slp_SLPStringsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_SLPStringsTester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_ScopeSetTester_SOURCES = slp/ScopeSetTest.cpp
slp_ScopeSetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_ScopeSetTester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_ServiceEntryTester_SOURCES = slp/ServiceEntryTest.cpp \
                                 slp/URLEntryTest.cpp
slp_ServiceEntryTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_ServiceEntryTester_LDADD = $(COMMON_SLP_TEST_LDADD)

slp_XIDAllocatorTester_SOURCES = slp/XIDAllocatorTest.cpp
slp_XIDAllocatorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
slp_XIDAllocatorTester_LDADD = $(COMMON_SLP_TEST_LDADD)
