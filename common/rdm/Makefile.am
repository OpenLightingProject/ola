include $(top_srcdir)/common.mk

SUBDIRS = testdata
EXTRA_DIST = DescriptorConsistencyChecker.h DiscoveryAgentTestHelper.h \
             GroupSizeCalculator.h Pids.proto \
             PidStoreLoader.h VariableFieldSizeCalculator.h

BUILT_SOURCES = Pids.pb.cc Pids.pb.h

noinst_LTLIBRARIES = libolardm.la
libolardm_la_SOURCES = RDMAPI.cpp RDMCommand.cpp UID.cpp

nodist_libolardm_la_SOURCES = Pids.pb.cc
libolardm_la_LIBADD = $(libprotobuf_LIBS)

if BUILD_TESTS
TESTS = DiscoveryAgentTester PidStoreTester RDMMessageTester RDMTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    libolardm.la \
                    ../base/libolabase.la \
                    ../utils/libolautils.la

DiscoveryAgentTester_SOURCES = DiscoveryAgentTest.cpp
DiscoveryAgentTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DiscoveryAgentTester_LDADD = $(COMMON_TEST_LDADD)

RDMTester_SOURCES = RDMAPITest.cpp RDMCommandTest.cpp \
                    QueueingRDMControllerTest.cpp UIDAllocatorTest.cpp \
                    UIDTest.cpp
RDMTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RDMTester_LDADD = $(COMMON_TEST_LDADD) \
                  ../io/libolaio.la \
                  ../messaging/libolamessaging.la \
                  ../network/libolanetwork.la

PidStoreTester_SOURCES = DescriptorConsistencyCheckerTest.cpp \
                         PidStoreTest.cpp
PidStoreTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PidStoreTester_LDADD = $(COMMON_TEST_LDADD) \
                       ../messaging/libolamessaging.la

RDMMessageTester_SOURCES = GroupSizeCalculatorTest.cpp \
                           MessageSerializerTest.cpp \
                           MessageDeserializerTest.cpp \
                           RDMMessageInterationTest.cpp \
                           StringMessageBuilderTest.cpp \
                           VariableFieldSizeCalculatorTest.cpp
RDMMessageTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RDMMessageTester_LDADD = $(COMMON_TEST_LDADD) \
                         ../messaging/libolamessaging.la \
                         ../network/libolanetwork.la

Pids.pb.cc Pids.pb.h: Pids.proto
	$(PROTOC) --cpp_out ./ Pids.proto

TestService.pb.cc TestService.pb.h: TestService.proto
	$(PROTOC) --cpp_out ./ TestService.proto

clean-local:
	rm -f *.pb.{h,cc}
