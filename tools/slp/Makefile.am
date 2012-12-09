include $(top_srcdir)/common.mk

EXTRA_DIST = Base.h \
             DATracker.h \
             RegistrationFileParser.h \
             SLP.proto \
             SLPClient.h \
             SLPClientCore.h \
             SLPDaemon.h \
             SLPPacketBuilder.h \
             SLPPacketConstants.h \
             SLPPacketParser.h \
             SLPPendingOperations.h \
             SLPServer.h \
             SLPStore.h \
             SLPStrings.h \
             SLPUDPSender.h \
             SLPUtil.h \
             ScopeSet.h \
             ServerCommon.h \
             ServiceEntry.h \
             URLEntry.h \
             XIDAllocator.h \
             slp-parser-actions.h

# Libraries
noinst_LTLIBRARIES = libolaslpcore.la libolaslp.la libolaslpproto.la

libolaslpcore_la_SOURCES = SLPUtil.cpp \
                           URLEntry.cpp

libolaslp_la_SOURCES = DATracker.cpp \
                       RegistrationFileParser.cpp \
                       SLPPacketBuilder.cpp \
                       SLPPacketParser.cpp \
                       SLPPendingOperations.cpp \
                       SLPServer.cpp \
                       SLPStore.cpp \
                       SLPStrings.cpp \
                       SLPUDPSender.cpp \
                       ScopeSet.cpp \
                       XIDAllocator.cpp
libolaslp_la_LIBADD = ../../common/libolacommon.la \
                      ./libolaslpcore.la

nodist_libolaslpproto_la_SOURCES = SLP.pb.cc
libolaslpproto_la_LIBADD = $(libprotobuf_LIBS)
# required, otherwise we get build errors
libolaslpproto_la_CXXFLAGS = $(WARNING_CXXFLAGS)

BUILT_SOURCES = SLP.pb.cc SLP.pb.h

SLP.pb.cc SLP.pb.h: SLP.proto
	$(PROTOC)  --cpp_out ./ SLP.proto

# Programs
noinst_PROGRAMS = slp_server slp_client slp_parser
slp_server_SOURCES = SLPDaemon.cpp \
                     slp-server.cpp
slp_server_LDADD = $(top_builddir)/common/libolacommon.la \
                   $(top_builddir)/common/http/libolahttp.la \
                   ./libolaslp.la \
                   ./libolaslpproto.la

slp_client_SOURCES = slp-client.cpp \
                     SLPClient.cpp \
                     SLPClientCore.cpp
slp_client_LDADD = $(top_builddir)/common/libolacommon.la \
                   $(top_builddir)/ola/libola.la \
                   ./libolaslpcore.la \
                   ./libolaslpproto.la

slp_parser_SOURCES = slp-parser.cpp \
                     config.tab.cpp \
                     slp-parser-actions.cpp \
                     lex.yy.cpp
slp_parser_CXXFLAGS = $(WARNING_CXXFLAGS)
slp_parser_LDADD =  $(top_builddir)/common/libolacommon.la \
                    $(LEXLIB)


clean-local:
	rm -f *.pb.{h,cc} lex.yy.cpp config.tab.cpp config.tab.h *.gcno *.gcda

lex.yy.cpp: config.lex
	$(LEX) --header-file=lex.yy.h -olex.yy.cpp config.lex

config.tab.cpp config.tab.h: config.ypp
	$(BISON) --defines=config.tab.h --output-file=config.tab.cpp config.ypp

# Tests
if BUILD_TESTS
TESTS = DATrackerTester PacketBuilderTester PacketParserTester \
        RegistrationFileParserTester ServiceEntryTester SLPStoreTester \
        SLPStringsTester ScopeSetTester XIDAllocatorTester
endif
COMMON_TEST_LDADD = $(CPPUNIT_LIBS) \
                    $(top_builddir)/common/libolacommon.la \
                    $(top_builddir)/common/testing/libolatesting.la \
                    ./libolaslp.la

check_PROGRAMS = $(TESTS)
DATrackerTester_SOURCES = GenericTester.cpp DATrackerTest.cpp
DATrackerTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DATrackerTester_LDADD = $(COMMON_TEST_LDADD)

PacketBuilderTester_SOURCES = GenericTester.cpp PacketBuilderTest.cpp
PacketBuilderTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
PacketBuilderTester_LDADD = $(COMMON_TEST_LDADD)

PacketParserTester_SOURCES = GenericTester.cpp PacketParserTest.cpp
PacketParserTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
PacketParserTester_LDADD = $(COMMON_TEST_LDADD)

SLPStoreTester_SOURCES = GenericTester.cpp SLPStoreTest.cpp
SLPStoreTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SLPStoreTester_LDADD = $(COMMON_TEST_LDADD)

SLPStringsTester_SOURCES = GenericTester.cpp SLPStringsTest.cpp
SLPStringsTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SLPStringsTester_LDADD = $(COMMON_TEST_LDADD)

ScopeSetTester_SOURCES = GenericTester.cpp ScopeSetTest.cpp
ScopeSetTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ScopeSetTester_LDADD = $(COMMON_TEST_LDADD)

ServiceEntryTester_SOURCES = GenericTester.cpp \
                             ServiceEntryTest.cpp \
                             URLEntryTest.cpp
ServiceEntryTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ServiceEntryTester_LDADD = $(COMMON_TEST_LDADD)

RegistrationFileParserTester_SOURCES = GenericTester.cpp \
                                       RegistrationFileParserTest.cpp
RegistrationFileParserTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
RegistrationFileParserTester_LDADD = $(COMMON_TEST_LDADD)

XIDAllocatorTester_SOURCES = GenericTester.cpp XIDAllocatorTest.cpp
XIDAllocatorTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
XIDAllocatorTester_LDADD = $(COMMON_TEST_LDADD)
