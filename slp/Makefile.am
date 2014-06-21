include $(top_srcdir)/common.mk

CLEANFILES = *.pb.{h,cc,cpp} *.gcno *.gcda

EXTRA_DIST = SLP.proto
noinst_HEADERS = URLListVerifier.h SLPServerTestHelper.h

# pkg-config
##################################################
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libolaslpclient.pc libolaslpserver.pc

# LIBRARIES
##################################################
noinst_LTLIBRARIES = libolaslpproto.la
lib_LTLIBRARIES = libolaslpcore.la \
                  libolaslpserver.la \
                  libolaslpclient.la

# libolaslpproto
nodist_libolaslpproto_la_SOURCES = SLP.pb.cc SLPService.pb.cpp
libolaslpproto_la_LIBADD = $(libprotobuf_LIBS)
# Required, otherwise we get build errors
libolaslpproto_la_CXXFLAGS = $(WARNING_CXXFLAGS)

BUILT_SOURCES = SLP.pb.cc SLP.pb.h SLPService.pb.h SLPService.pb.h

SLP.pb.cc SLP.pb.h: Makefile.am SLP.proto
	$(PROTOC) --cpp_out ./ --proto_path $(srcdir) $(srcdir)/SLP.proto

SLPService.pb.cpp SLPService.pb.h: Makefile.am SLP.proto
	$(OLA_PROTOC)  --cppservice_out ./ --proto_path $(srcdir) $(srcdir)/SLP.proto

# libolaslpcore
libolaslpcore_la_SOURCES = SLPUtil.cpp SLPUtil.h URLEntry.cpp

# libolaslpserver
# This is an SLP server, without the RPC interface.
libolaslpserver_la_SOURCES = DATracker.cpp DATracker.h \
                             RegistrationFileParser.cpp \
                             RegistrationFileParser.h SLPPacketBuilder.cpp \
                             SLPPacketBuilder.h SLPPacketConstants.h \
                             SLPPacketParser.cpp SLPPacketParser.h \
                             SLPPendingOperations.cpp SLPPendingOperations.h \
                             SLPServer.cpp SLPServer.h SLPStore.cpp \
                             SLPStore.h SLPStrings.cpp SLPStrings.h \
                             SLPUDPSender.cpp SLPUDPSender.h ScopeSet.cpp \
                             ScopeSet.h ServerCommon.h ServiceEntry.h \
                             XIDAllocator.h
libolaslpserver_la_LIBADD = ../common/libolacommon.la \
                            libolaslpcore.la

# libolaslpclient
libolaslpclient_la_SOURCES = SLPClient.cpp SLPClientCore.cpp SLPClientCore.h
libolaslpclient_la_LIBADD = ../common/libolacommon.la \
                            ../ola/libola.la \
                            libolaslpproto.la \
                            libolaslpcore.la

# PROGRAMS
##################################################
noinst_PROGRAMS = slp_client

if HAVE_LIBMICROHTTPD
noinst_PROGRAMS += slp_server
endif

slp_server_SOURCES = SLPDaemon.cpp SLPDaemon.h slp-server.cpp
slp_server_LDADD = ../common/libolacommon.la \
                   ../common/http/libolahttp.la \
                   libolaslpproto.la \
                   libolaslpserver.la

slp_client_SOURCES = slp-client.cpp
slp_client_LDADD = ../common/libolacommon.la \
                   ../ola/libola.la \
                   libolaslpclient.la \
                   libolaslpcore.la

if USING_WIN32
slp_server_LDADD += -lWs2_32 -lIphlpapi
slp_client_LDADD += -lWs2_32 -lIphlpapi
endif

# TESTS
##################################################
if BUILD_TESTS
TESTS = DATrackerTester PacketBuilderTester PacketParserTester \
        RegistrationFileParserTester SLPServerDATester SLPServerNetworkTester \
        SLPServerSATester SLPServerUATester SLPStoreTester SLPStringsTester \
        ScopeSetTester ServiceEntryTester XIDAllocatorTester
endif
check_PROGRAMS = $(TESTS)
COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    ../common/libolacommon.la \
                    libolaslpcore.la \
                    libolaslpserver.la

DATrackerTester_SOURCES = DATrackerTest.cpp
DATrackerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DATrackerTester_LDADD = $(COMMON_TEST_LDADD)

PacketBuilderTester_SOURCES = PacketBuilderTest.cpp
PacketBuilderTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PacketBuilderTester_LDADD = $(COMMON_TEST_LDADD)

PacketParserTester_SOURCES = PacketParserTest.cpp
PacketParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PacketParserTester_LDADD = $(COMMON_TEST_LDADD)

RegistrationFileParserTester_SOURCES = RegistrationFileParserTest.cpp
RegistrationFileParserTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RegistrationFileParserTester_LDADD = $(COMMON_TEST_LDADD)

SLPServerDATester_SOURCES = SLPServerTestHelper.cpp \
                            SLPServerDATest.cpp
SLPServerDATester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SLPServerDATester_LDADD = $(COMMON_TEST_LDADD)

SLPServerSATester_SOURCES = SLPServerTestHelper.cpp \
                            SLPServerSATest.cpp
SLPServerSATester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SLPServerSATester_LDADD = $(COMMON_TEST_LDADD)

SLPServerUATester_SOURCES = SLPServerTestHelper.cpp \
                            SLPServerUATest.cpp
SLPServerUATester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SLPServerUATester_LDADD = $(COMMON_TEST_LDADD)

SLPServerNetworkTester_SOURCES = SLPServerTestHelper.cpp \
                                 SLPServerNetworkTest.cpp
SLPServerNetworkTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SLPServerNetworkTester_LDADD = $(COMMON_TEST_LDADD)

SLPStoreTester_SOURCES = SLPStoreTest.cpp
SLPStoreTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SLPStoreTester_LDADD = $(COMMON_TEST_LDADD)

SLPStringsTester_SOURCES = SLPStringsTest.cpp
SLPStringsTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SLPStringsTester_LDADD = $(COMMON_TEST_LDADD)

ScopeSetTester_SOURCES = ScopeSetTest.cpp
ScopeSetTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ScopeSetTester_LDADD = $(COMMON_TEST_LDADD)

ServiceEntryTester_SOURCES = ServiceEntryTest.cpp \
                             URLEntryTest.cpp
ServiceEntryTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ServiceEntryTester_LDADD = $(COMMON_TEST_LDADD)

XIDAllocatorTester_SOURCES = XIDAllocatorTest.cpp
XIDAllocatorTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
XIDAllocatorTester_LDADD = $(COMMON_TEST_LDADD)
