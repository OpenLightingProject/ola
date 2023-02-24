# LIBRARIES
##################################################
common_libolacommon_la_SOURCES += common/dmx/RunLengthEncoder.cpp

# TESTS
##################################################
test_programs += common/dmx/RunLengthEncoderTester

common_dmx_RunLengthEncoderTester_SOURCES = common/dmx/RunLengthEncoderTest.cpp
common_dmx_RunLengthEncoderTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
common_dmx_RunLengthEncoderTester_LDADD = $(COMMON_TESTING_LIBS)
