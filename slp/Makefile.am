include $(top_srcdir)/common.mk

EXTRA_DIST = DATracker.h \
             RegistrationFileParser.h \
             SLP.proto \
             SLPClientCore.h \
             SLPDaemon.h \
             SLPPacketBuilder.h \
             SLPPacketConstants.h \
             SLPPacketParser.h \
             SLPPendingOperations.h \
             SLPServer.h \
             SLPServerTestHelper.h \
             SLPStore.h \
             SLPStrings.h \
             SLPUDPSender.h \
             SLPUtil.h \
             ScopeSet.h \
             ServerCommon.h \
             ServiceEntry.h \
             URLListVerifier.h \
             XIDAllocator.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libolaslpclient.pc libolaslpserver.pc

# Libraries
noinst_LTLIBRARIES = libolaslpproto.la

lib_LTLIBRARIES = libolaslpcore.la \
                  libolaslpserver.la \
                  libolaslpclient.la

# libolaslpproto.la
nodist_libolaslpproto_la_SOURCES = SLP.pb.cc
libolaslpproto_la_LIBADD = $(libprotobuf_LIBS)
# required, otherwise we get build errors
libolaslpproto_la_CXXFLAGS = $(WARNING_CXXFLAGS)

# libolaslpcore.la
libolaslpcore_la_SOURCES = SLPUtil.cpp \
                           URLEntry.cpp

# libolaslpserver.la
# This is an SLP server, without the RPC interface.
libolaslpserver_la_SOURCES = DATracker.cpp \
                             RegistrationFileParser.cpp \
                             SLPPacketBuilder.cpp \
                             SLPPacketParser.cpp \
                             SLPPendingOperations.cpp \
                             SLPServer.cpp \
                             SLPStore.cpp \
                             SLPStrings.cpp \
                             SLPUDPSender.cpp \
                             ScopeSet.cpp
libolaslpserver_la_LIBADD = $(top_builddir)/common/libolacommon.la \
                            ./libolaslpcore.la

#libolaslpclient.la
libolaslpclient_la_SOURCES = SLPClient.cpp \
                             SLPClientCore.cpp
libolaslpclient_la_LIBADD = $(top_builddir)/common/libolacommon.la \
                            $(top_builddir)/ola/libola.la \
                            ./libolaslpproto.la \
                            ./libolaslpcore.la

BUILT_SOURCES = SLP.pb.cc SLP.pb.h

SLP.pb.cc SLP.pb.h: SLP.proto
	$(PROTOC)  --cpp_out ./ SLP.proto

# Programs
noinst_PROGRAMS = slp_client

if HAVE_LIBMICROHTTPD
noinst_PROGRAMS += slp_server
endif

slp_server_SOURCES = SLPDaemon.cpp \
                     slp-server.cpp
slp_server_LDADD = $(top_builddir)/common/libolacommon.la \
                   $(top_builddir)/common/http/libolahttp.la \
                   ./libolaslpproto.la \
                   ./libolaslpserver.la

slp_client_SOURCES = slp-client.cpp
slp_client_LDADD = $(top_builddir)/common/libolacommon.la \
                   $(top_builddir)/ola/libola.la \
                   ./libolaslpclient.la \
                   ./libolaslpcore.la

clean-local:
	rm -f *.pb.{h,cc} *.gcno *.gcda

# Tests
if BUILD_TESTS
TESTS = SLPServerTester DATrackerTester PacketParserTester \
        RegistrationFileParserTester ServiceEntryTester SLPStoreTester \
        XIDAllocatorTester PacketBuilderTester ScopeSetTester SLPStringsTester
endif
COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    $(top_builddir)/common/libolacommon.la \
                    ./libolaslpcore.la \
                    ./libolaslpserver.la

check_PROGRAMS = $(TESTS)
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

SLPServerTester_SOURCES = SLPServerTestHelper.cpp \
                          SLPServerDATest.cpp \
                          SLPServerNetworkTest.cpp \
                          SLPServerSATest.cpp \
                          SLPServerUATest.cpp
SLPServerTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
SLPServerTester_LDADD = $(COMMON_TEST_LDADD)

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
