include $(top_srcdir)/common.mk

SUBDIRS = testdata

BUILT_SOURCES = Pids.pb.cc Pids.pb.h

# LIBRARIES
##################################################
noinst_LTLIBRARIES = libolardm.la
libolardm_la_SOURCES = AckTimerResponder.cpp AdvancedDimmerResponder.cpp \
                       CommandPrinter.cpp DescriptorConsistencyChecker.cpp \
                       DescriptorConsistencyChecker.h DimmerResponder.cpp \
                       DimmerRootDevice.cpp DimmerSubDevice.cpp \
                       DiscoveryAgent.cpp DiscoveryAgentTestHelper.h \
                       DummyResponder.cpp FakeNetworkManager.cpp \
                       FakeNetworkManager.h GroupSizeCalculator.cpp \
                       GroupSizeCalculator.h MessageDeserializer.cpp \
                       MessageSerializer.cpp MovingLightResponder.cpp \
                       NetworkManager.cpp NetworkManager.h \
                       NetworkResponder.cpp OpenLightingEnums.cpp \
                       PidStore.cpp PidStoreHelper.cpp PidStoreLoader.cpp \
                       PidStoreLoader.h Pids.proto QueueingRDMController.cpp \
                       RDMAPI.cpp RDMCommand.cpp RDMCommandSerializer.cpp \
                       RDMHelper.cpp ResponderHelper.cpp \
                       ResponderLoadSensor.cpp ResponderPersonality.cpp \
                       ResponderSettings.cpp ResponderSlotData.cpp \
                       SensorResponder.cpp StringMessageBuilder.cpp \
                       SubDeviceDispatcher.cpp UID.cpp \
                       VariableFieldSizeCalculator.cpp \
                       VariableFieldSizeCalculator.h
libolardm_la_CXXFLAGS = $(COMMON_CXXFLAGS) -DPID_DATA_DIR=\"${piddatadir}\"

nodist_libolardm_la_SOURCES = Pids.pb.cc
libolardm_la_LIBADD = $(libprotobuf_LIBS)

Pids.pb.cc Pids.pb.h: Pids.proto
	$(PROTOC) --cpp_out ./ --proto_path $(srcdir) $(srcdir)/Pids.proto

CLEANFILES = *.pb.{h,cc}

# TESTS
##################################################
if BUILD_TESTS
TESTS = DiscoveryAgentTester PidStoreTester RDMHelperTester RDMMessageTester \
        RDMTester
endif
check_PROGRAMS = $(TESTS)

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) ../libolacommon.la

DiscoveryAgentTester_SOURCES = DiscoveryAgentTest.cpp
DiscoveryAgentTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
DiscoveryAgentTester_LDADD = $(COMMON_TEST_LDADD)

PidStoreTester_SOURCES = DescriptorConsistencyCheckerTest.cpp \
                         PidStoreTest.cpp
PidStoreTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
PidStoreTester_LDADD = $(COMMON_TEST_LDADD)

RDMHelperTester_SOURCES = RDMHelperTest.cpp
RDMHelperTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RDMHelperTester_LDADD = $(COMMON_TEST_LDADD)

RDMMessageTester_SOURCES = GroupSizeCalculatorTest.cpp \
                           MessageSerializerTest.cpp \
                           MessageDeserializerTest.cpp \
                           RDMMessageInterationTest.cpp \
                           StringMessageBuilderTest.cpp \
                           VariableFieldSizeCalculatorTest.cpp
RDMMessageTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RDMMessageTester_LDADD = $(COMMON_TEST_LDADD)

RDMTester_SOURCES = RDMAPITest.cpp RDMCommandTest.cpp \
                    QueueingRDMControllerTest.cpp UIDAllocatorTest.cpp \
                    UIDTest.cpp
RDMTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
RDMTester_LDADD = $(COMMON_TEST_LDADD)
