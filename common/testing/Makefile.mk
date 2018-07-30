# LIBRARIES
##################################################
if BUILD_TESTS
noinst_LTLIBRARIES += common/testing/libolatesting.la \
                      common/testing/libtestmain.la
common_testing_libolatesting_la_SOURCES = \
    common/testing/MockUDPSocket.cpp \
    common/testing/TestUtils.cpp
common_testing_libolatesting_la_CXXFLAGS = $(COMMON_TESTING_PROTOBUF_FLAGS)
common_testing_libtestmain_la_SOURCES = common/testing/GenericTester.cpp
endif
