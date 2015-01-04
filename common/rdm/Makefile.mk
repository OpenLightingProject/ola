built_sources += \
    common/rdm/Pids.pb.cc \
    common/rdm/Pids.pb.h

# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += \
    common/rdm/AckTimerResponder.cpp \
    common/rdm/AdvancedDimmerResponder.cpp \
    common/rdm/CommandPrinter.cpp \
    common/rdm/DescriptorConsistencyChecker.cpp \
    common/rdm/DescriptorConsistencyChecker.h \
    common/rdm/DimmerResponder.cpp \
    common/rdm/DimmerRootDevice.cpp \
    common/rdm/DimmerSubDevice.cpp \
    common/rdm/DiscoveryAgent.cpp \
    common/rdm/DiscoveryAgentTestHelper.h \
    common/rdm/DummyResponder.cpp \
    common/rdm/FakeNetworkManager.cpp \
    common/rdm/FakeNetworkManager.h \
    common/rdm/GroupSizeCalculator.cpp \
    common/rdm/GroupSizeCalculator.h \
    common/rdm/MessageDeserializer.cpp \
    common/rdm/MessageSerializer.cpp \
    common/rdm/MovingLightResponder.cpp \
    common/rdm/NetworkManager.cpp \
    common/rdm/NetworkManager.h \
    common/rdm/NetworkResponder.cpp \
    common/rdm/OpenLightingEnums.cpp \
    common/rdm/PidStore.cpp \
    common/rdm/PidStoreHelper.cpp \
    common/rdm/PidStoreLoader.cpp \
    common/rdm/PidStoreLoader.h \
    common/rdm/QueueingRDMController.cpp \
    common/rdm/RDMAPI.cpp \
    common/rdm/RDMCommand.cpp \
    common/rdm/RDMCommandSerializer.cpp \
    common/rdm/RDMHelper.cpp \
    common/rdm/ResponderHelper.cpp \
    common/rdm/ResponderLoadSensor.cpp \
    common/rdm/ResponderPersonality.cpp \
    common/rdm/ResponderSettings.cpp \
    common/rdm/ResponderSlotData.cpp \
    common/rdm/SensorResponder.cpp \
    common/rdm/StringMessageBuilder.cpp \
    common/rdm/SubDeviceDispatcher.cpp \
    common/rdm/UID.cpp \
    common/rdm/VariableFieldSizeCalculator.cpp \
    common/rdm/VariableFieldSizeCalculator.h
nodist_common_libolacommon_la_SOURCES += common/rdm/Pids.pb.cc
common_libolacommon_la_CXXFLAGS += -DPID_DATA_DIR=\"${piddatadir}\"
common_libolacommon_la_LIBADD += $(libprotobuf_LIBS)

EXTRA_DIST += common/rdm/Pids.proto

common/rdm/Pids.pb.cc common/rdm/Pids.pb.h: common/rdm/Makefile.mk common/rdm/Pids.proto
	$(PROTOC) --cpp_out common/rdm --proto_path $(srcdir)/common/rdm $(srcdir)/common/rdm/Pids.proto

# TESTS_DATA
##################################################

EXTRA_DIST += \
    common/rdm/testdata/duplicate_manufacturer.proto \
    common/rdm/testdata/duplicate_pid_name.proto \
    common/rdm/testdata/duplicate_pid_value.proto \
    common/rdm/testdata/inconsistent_pid.proto \
    common/rdm/testdata/invalid_esta_pid.proto \
    common/rdm/testdata/pids/pids1.proto \
    common/rdm/testdata/pids/pids2.proto \
    common/rdm/testdata/test_pids.proto

# TESTS
##################################################
test_programs += \
    common/rdm/DiscoveryAgentTester \
    common/rdm/PidStoreTester \
    common/rdm/RDMHelperTester \
    common/rdm/RDMMessageTester \
    common/rdm/RDMTester

common_rdm_DiscoveryAgentTester_SOURCES = common/rdm/DiscoveryAgentTest.cpp
common_rdm_DiscoveryAgentTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_rdm_DiscoveryAgentTester_LDADD = $(COMMON_TESTING_LIBS)

common_rdm_PidStoreTester_SOURCES = \
    common/rdm/DescriptorConsistencyCheckerTest.cpp \
    common/rdm/PidStoreTest.cpp
common_rdm_PidStoreTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_rdm_PidStoreTester_LDADD = $(COMMON_TESTING_LIBS)

common_rdm_RDMHelperTester_SOURCES = common/rdm/RDMHelperTest.cpp
common_rdm_RDMHelperTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_rdm_RDMHelperTester_LDADD = $(COMMON_TESTING_LIBS)

common_rdm_RDMMessageTester_SOURCES = \
    common/rdm/GroupSizeCalculatorTest.cpp \
    common/rdm/MessageSerializerTest.cpp \
    common/rdm/MessageDeserializerTest.cpp \
    common/rdm/RDMMessageInterationTest.cpp \
    common/rdm/StringMessageBuilderTest.cpp \
    common/rdm/VariableFieldSizeCalculatorTest.cpp
common_rdm_RDMMessageTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_rdm_RDMMessageTester_LDADD = $(COMMON_TESTING_LIBS)

common_rdm_RDMTester_SOURCES = \
    common/rdm/RDMAPITest.cpp \
    common/rdm/RDMCommandTest.cpp \
    common/rdm/QueueingRDMControllerTest.cpp \
    common/rdm/UIDAllocatorTest.cpp \
    common/rdm/UIDTest.cpp
common_rdm_RDMTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_rdm_RDMTester_LDADD = $(COMMON_TESTING_LIBS)
