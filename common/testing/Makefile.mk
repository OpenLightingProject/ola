# LIBRARIES
##################################################
if BUILD_TESTS
noinst_LTLIBRARIES += libolatesting.la libtestmain.la
libolatesting_la_SOURCES = \
    common/testing/MockUDPSocket.cpp \
    common/testing/TestUtils.cpp
libtestmain_la_SOURCES = common/testing/GenericTester.cpp
endif
