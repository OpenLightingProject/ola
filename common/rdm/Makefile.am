include $(top_srcdir)/common.mk

SUBDIRS = testdata
EXTRA_DIST = DescriptorConsistencyChecker.h DiscoveryAgentTestHelper.h \
             GroupSizeCalculator.h Pids.proto \
             PidStoreLoader.h VariableFieldSizeCalculator.h

BUILT_SOURCES = Pids.pb.cc Pids.pb.h

noinst_LTLIBRARIES = libolardm.la
libolardm_la_SOURCES = CommandPrinter.cpp \
                       DescriptorConsistencyChecker.cpp \
                       DiscoveryAgent.cpp \
                       GroupSizeCalculator.cpp \
                       MessageSerializer.cpp MessageDeserializer.cpp \
                       PidStore.cpp PidStoreHelper.cpp PidStoreLoader.cpp \
                       RDMAPI.cpp RDMCommand.cpp RDMHelper.cpp \
                       StringMessageBuilder.cpp QueueingRDMController.cpp \
                       UID.cpp VariableFieldSizeCalculator.cpp
nodist_libolardm_la_SOURCES = Pids.pb.cc
libolardm_la_LIBADD = $(libprotobuf_LIBS)

if BUILD_TESTS
TESTS = DiscoveryAgentTester PidStoreTester RDMMessageTester RDMTester
endif
check_PROGRAMS = $(TESTS)

DiscoveryAgentTester_SOURCES = DiscoveryAgentTester.cpp DiscoveryAgentTest.cpp
DiscoveryAgentTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
DiscoveryAgentTester_LDADD = $(CPPUNIT_LIBS) \
                             libolardm.la \
                             ../logging/liblogging.la \
                             ../utils/libolautils.la

RDMTester_SOURCES = RDMAPITest.cpp RDMCommandTest.cpp RDMTester.cpp \
                    QueueingRDMControllerTest.cpp UIDTest.cpp
RDMTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
RDMTester_LDADD = $(CPPUNIT_LIBS) \
                  libolardm.la \
                  ../io/libolaio.la \
                  ../logging/liblogging.la \
                  ../network/libolanetwork.la \
                  ../testing/libolatesting.la \
                  ../utils/libolautils.la

PidStoreTester_SOURCES = DescriptorConsistencyCheckerTest.cpp \
                         PidStoreTester.cpp PidStoreTest.cpp
PidStoreTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
PidStoreTester_LDADD = $(CPPUNIT_LIBS) \
                       libolardm.la \
                       ../logging/liblogging.la \
                       ../messaging/libolamessaging.la \
                       ../utils/libolautils.la

RDMMessageTester_SOURCES = GroupSizeCalculatorTest.cpp \
                           MessageSerializerTest.cpp \
                           MessageDeserializerTest.cpp \
                           RDMMessageTester.cpp \
                           RDMMessageInterationTest.cpp \
                           StringMessageBuilderTest.cpp \
                           VariableFieldSizeCalculatorTest.cpp
RDMMessageTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
RDMMessageTester_LDADD = $(CPPUNIT_LIBS) \
                         libolardm.la \
                          ../logging/liblogging.la \
                          ../messaging/libolamessaging.la \
                          ../network/libolanetwork.la \
                          ../utils/libolautils.la

Pids.pb.cc Pids.pb.h: Pids.proto
	$(PROTOC) --cpp_out ./ Pids.proto

TestService.pb.cc TestService.pb.h: TestService.proto
	$(PROTOC) --cpp_out ./ TestService.proto

clean-local:
	rm -f *.pb.{h,cc}
