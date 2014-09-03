COMMON_E131_CXXFLAGS = $(COMMON_CXXFLAGS) -Wconversion

# pkg-config
##################################################
if INSTALL_E133
pkgconfig_DATA += plugins/e131/e131/libolaacn.pc
endif

# LIBRARIES
##################################################

if INSTALL_ACN
  lib_LTLIBRARIES += plugins/e131/e131/libolaacn.la
else
  noinst_LTLIBRARIES += plugins/e131/e131/libolaacn.la
endif

# libolaacn.la
plugins_e131_e131_libolaacn_la_SOURCES = \
    plugins/e131/e131/CID.cpp \
    plugins/e131/e131/CIDImpl.cpp \
    plugins/e131/e131/CIDImpl.h
plugins_e131_e131_libolaacn_la_CXXFLAGS = $(COMMON_E131_CXXFLAGS) $(uuid_CFLAGS)
plugins_e131_e131_libolaacn_la_LIBADD = $(uuid_LIBS) \
                                        common/libolacommon.la

# libolae131core.la
# This needs to be after libolaacn.la since it depends on it. Otherwise it
# breaks the freeBSD build
noinst_LTLIBRARIES += plugins/e131/e131/libolae131core.la
plugins_e131_e131_libolae131core_la_SOURCES = \
    plugins/e131/e131/BaseInflator.cpp \
    plugins/e131/e131/BaseInflator.h \
    plugins/e131/e131/DMPAddress.cpp \
    plugins/e131/e131/DMPAddress.h \
    plugins/e131/e131/DMPE131Inflator.cpp \
    plugins/e131/e131/DMPE131Inflator.h \
    plugins/e131/e131/DMPHeader.h \
    plugins/e131/e131/DMPInflator.cpp \
    plugins/e131/e131/DMPInflator.h \
    plugins/e131/e131/DMPPDU.cpp \
    plugins/e131/e131/DMPPDU.h \
    plugins/e131/e131/E131DiscoveryInflator.cpp \
    plugins/e131/e131/E131DiscoveryInflator.h \
    plugins/e131/e131/E131Header.h \
    plugins/e131/e131/E131Inflator.cpp \
    plugins/e131/e131/E131Inflator.h \
    plugins/e131/e131/E131Node.cpp \
    plugins/e131/e131/E131Node.h \
    plugins/e131/e131/E131PDU.cpp \
    plugins/e131/e131/E131PDU.h \
    plugins/e131/e131/E131Sender.cpp \
    plugins/e131/e131/E131Sender.h \
    plugins/e131/e131/E133Header.h \
    plugins/e131/e131/E133Inflator.cpp \
    plugins/e131/e131/E133Inflator.h \
    plugins/e131/e131/E133PDU.cpp \
    plugins/e131/e131/E133PDU.h \
    plugins/e131/e131/E133StatusInflator.cpp \
    plugins/e131/e131/E133StatusInflator.h \
    plugins/e131/e131/E133StatusPDU.cpp \
    plugins/e131/e131/E133StatusPDU.h \
    plugins/e131/e131/HeaderSet.h \
    plugins/e131/e131/PDU.cpp \
    plugins/e131/e131/PDU.h \
    plugins/e131/e131/PDUTestCommon.h \
    plugins/e131/e131/PreamblePacker.cpp \
    plugins/e131/e131/PreamblePacker.h \
    plugins/e131/e131/RDMInflator.cpp \
    plugins/e131/e131/RDMInflator.h \
    plugins/e131/e131/RDMPDU.cpp \
    plugins/e131/e131/RDMPDU.h \
    plugins/e131/e131/RootHeader.h \
    plugins/e131/e131/RootInflator.cpp \
    plugins/e131/e131/RootInflator.h \
    plugins/e131/e131/RootPDU.cpp \
    plugins/e131/e131/RootPDU.h \
    plugins/e131/e131/RootSender.cpp \
    plugins/e131/e131/RootSender.h \
    plugins/e131/e131/TCPTransport.cpp \
    plugins/e131/e131/TCPTransport.h \
    plugins/e131/e131/Transport.h \
    plugins/e131/e131/TransportHeader.h \
    plugins/e131/e131/UDPTransport.cpp \
    plugins/e131/e131/UDPTransport.h

plugins_e131_e131_libolae131core_la_CXXFLAGS = \
    $(COMMON_E131_CXXFLAGS) $(uuid_CFLAGS)
plugins_e131_e131_libolae131core_la_LIBADD = $(uuid_LIBS) \
                                             common/libolacommon.la \
                                             plugins/e131/e131/libolaacn.la

# PROGRAMS
##################################################
noinst_PROGRAMS += plugins/e131/e131/e131_transmit_test \
                   plugins/e131/e131/e131_loadtest
plugins_e131_e131_e131_transmit_test_SOURCES = \
    plugins/e131/e131/e131_transmit_test.cpp \
    plugins/e131/e131/E131TestFramework.cpp \
    plugins/e131/e131/E131TestFramework.h
plugins_e131_e131_e131_transmit_test_LDADD = plugins/e131/e131/libolae131core.la

plugins_e131_e131_e131_loadtest_SOURCES = plugins/e131/e131/e131_loadtest.cpp
plugins_e131_e131_e131_loadtest_LDADD = plugins/e131/e131/libolae131core.la

# TESTS
##################################################
test_programs += \
    plugins/e131/e131/E131Tester \
    plugins/e131/e131/E133Tester \
    plugins/e131/e131/TransportTester

plugins_e131_e131_E131Tester_SOURCES = \
    plugins/e131/e131/BaseInflatorTest.cpp \
    plugins/e131/e131/CIDTest.cpp \
    plugins/e131/e131/DMPAddressTest.cpp \
    plugins/e131/e131/DMPInflatorTest.cpp \
    plugins/e131/e131/DMPPDUTest.cpp \
    plugins/e131/e131/E131InflatorTest.cpp \
    plugins/e131/e131/E131PDUTest.cpp \
    plugins/e131/e131/HeaderSetTest.cpp \
    plugins/e131/e131/PDUTest.cpp \
    plugins/e131/e131/RootInflatorTest.cpp \
    plugins/e131/e131/RootPDUTest.cpp \
    plugins/e131/e131/RootSenderTest.cpp
plugins_e131_e131_E131Tester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
# For some completely messed up reason on mac CPPUNIT_LIBS has to come after
# the ossp uuid library.
# CPPUNIT_LIBS contains -ldl which causes the tests to fail in strange ways
plugins_e131_e131_E131Tester_LDADD = \
    plugins/e131/e131/libolae131core.la \
    $(COMMON_TESTING_LIBS)

plugins_e131_e131_E133Tester_SOURCES = \
    plugins/e131/e131/E133InflatorTest.cpp \
    plugins/e131/e131/E133PDUTest.cpp \
    plugins/e131/e131/RDMPDUTest.cpp
plugins_e131_e131_E133Tester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
plugins_e131_e131_E133Tester_LDADD = \
    plugins/e131/e131/libolae131core.la \
    $(COMMON_TESTING_LIBS)

plugins_e131_e131_TransportTester_SOURCES = \
    plugins/e131/e131/TCPTransportTest.cpp \
    plugins/e131/e131/UDPTransportTest.cpp
plugins_e131_e131_TransportTester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
plugins_e131_e131_TransportTester_LDADD = plugins/e131/e131/libolae131core.la \
                                          $(COMMON_TESTING_LIBS)
