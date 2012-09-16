include $(top_srcdir)/common.mk

EXTRA_DIST = Base.h \
             RegistrationFileParser.h \
             SLP.proto \
             SLPClient.h \
             SLPClientCore.h \
             SLPPacketBuilder.h \
             SLPPacketConstants.h \
             SLPPacketParser.h \
             SLPServer.h \
             SLPServiceImpl.h \
             SLPStore.h \
             SLPStrings.h \
             ScopedSLPStore.h \
             slp-parser-actions.h \
             URLEntry.h

noinst_PROGRAMS = slp_server slp_client slp_parser

slp_server_SOURCES = RegistrationFileParser.cpp \
                     SLPNode.cpp \
                     SLPPacketBuilder.cpp \
                     SLPPacketParser.cpp \
                     SLPServer.cpp \
                     SLPServiceImpl.cpp \
                     SLPStore.cpp \
                     SLPStrings.cpp \
                     ScopedSLPStore.cpp \
                     slp-server.cpp \
                     URLEntry.cpp
slp_server_LDADD = $(top_builddir)/common/libolacommon.la \
                   $(top_builddir)/common/http/libolahttp.la \
                   ./libolaslpproto.la

slp_client_SOURCES = slp-client.cpp \
                     SLPClient.cpp \
                     SLPClientCore.cpp
slp_client_LDADD = $(top_builddir)/common/libolacommon.la \
                   $(top_builddir)/ola/libola.la \
                   ./libolaslpproto.la

slp_parser_SOURCES = slp-parser.cpp \
                     config.tab.cpp \
                     slp-parser-actions.cpp \
                     lex.yy.cpp
slp_parser_CXXFLAGS = $(WARNING_CXXFLAGS)
slp_parser_LDADD =  $(top_builddir)/common/libolacommon.la \
                    $(LEXLIB)

noinst_LTLIBRARIES = libolaslpproto.la
nodist_libolaslpproto_la_SOURCES = SLP.pb.cc
libolaslpproto_la_LIBADD = $(libprotobuf_LIBS)
# required, otherwise we get build errors
libolaslpproto_la_CXXFLAGS = $(WARNING_CXXFLAGS)

BUILT_SOURCES = SLP.pb.cc SLP.pb.h

SLP.pb.cc SLP.pb.h: SLP.proto
	$(PROTOC)  --cpp_out ./ SLP.proto

clean-local:
	rm -f *.pb.{h,cc} lex.yy.cpp config.tab.cpp config.tab.h

lex.yy.cpp: config.lex
	$(LEX) --header-file=lex.yy.h -olex.yy.cpp config.lex

config.tab.cpp config.tab.h: config.ypp
	$(BISON) --defines=config.tab.h --output-file=config.tab.cpp config.ypp


if BUILD_TESTS
TESTS = PacketBuilderTester PacketParserTester \
        SLPStoreTester SLPStringsTester
endif
COMMON_TEST_LDADD = $(CPPUNIT_LIBS) \
                    $(top_builddir)/common/libolacommon.la \
                    $(top_builddir)/common/testing/libolatesting.la

check_PROGRAMS = $(TESTS)
PacketBuilderTester_SOURCES = GenericTester.cpp \
                              PacketBuilderTest.cpp \
                              SLPPacketBuilder.cpp \
                              URLEntry.cpp \
                              URLEntryTest.cpp
PacketBuilderTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
PacketBuilderTester_LDADD = $(COMMON_TEST_LDADD)

PacketParserTester_SOURCES = GenericTester.cpp \
                             PacketParserTest.cpp \
                             SLPPacketParser.cpp \
                             SLPStrings.cpp
PacketParserTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
PacketParserTester_LDADD = $(COMMON_TEST_LDADD)

SLPStoreTester_SOURCES = GenericTester.cpp \
                         SLPStore.cpp \
                         SLPStoreTest.cpp \
                         ScopedSLPStore.cpp \
                         ScopedSLPStoreTest.cpp
SLPStoreTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SLPStoreTester_LDADD = $(COMMON_TEST_LDADD)

SLPStringsTester_SOURCES = GenericTester.cpp \
                           SLPStrings.cpp \
                           SLPStringsTest.cpp
SLPStringsTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SLPStringsTester_LDADD = $(COMMON_TEST_LDADD)
