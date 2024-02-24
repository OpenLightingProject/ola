COMMON_E131_CXXFLAGS = $(COMMON_CXXFLAGS) -Wconversion

# pkg-config
##################################################
if INSTALL_E133
pkgconfig_DATA += libs/acn/libolaacn.pc
endif

# LIBRARIES
##################################################

if INSTALL_ACN
  lib_LTLIBRARIES += libs/acn/libolaacn.la
else
  noinst_LTLIBRARIES += libs/acn/libolaacn.la
endif

# libolaacn.la
libs_acn_libolaacn_la_SOURCES = \
    libs/acn/CID.cpp \
    libs/acn/CIDImpl.cpp \
    libs/acn/CIDImpl.h
libs_acn_libolaacn_la_CXXFLAGS = $(COMMON_E131_CXXFLAGS) $(uuid_CFLAGS)
libs_acn_libolaacn_la_LIBADD = $(uuid_LIBS) \
                               common/libolacommon.la

# libolae131core.la
# This needs to be after libolaacn.la since it depends on it. Otherwise it
# breaks the freeBSD build
noinst_LTLIBRARIES += libs/acn/libolae131core.la
libs_acn_libolae131core_la_SOURCES = \
    libs/acn/BaseInflator.cpp \
    libs/acn/BaseInflator.h \
    libs/acn/BrokerClientAddInflator.h \
    libs/acn/BrokerClientEntryChangeInflator.h \
    libs/acn/BrokerClientEntryHeader.h \
    libs/acn/BrokerClientEntryPDU.cpp \
    libs/acn/BrokerClientEntryPDU.h \
    libs/acn/BrokerClientRemoveInflator.h \
    libs/acn/BrokerConnectPDU.cpp \
    libs/acn/BrokerConnectPDU.h \
    libs/acn/BrokerInflator.h \
    libs/acn/BrokerNullInflator.h \
    libs/acn/BrokerNullPDU.cpp \
    libs/acn/BrokerNullPDU.h \
    libs/acn/BrokerPDU.cpp \
    libs/acn/BrokerPDU.h \
    libs/acn/DMPAddress.cpp \
    libs/acn/DMPAddress.h \
    libs/acn/DMPE131Inflator.cpp \
    libs/acn/DMPE131Inflator.h \
    libs/acn/DMPHeader.h \
    libs/acn/DMPInflator.cpp \
    libs/acn/DMPInflator.h \
    libs/acn/DMPPDU.cpp \
    libs/acn/DMPPDU.h \
    libs/acn/E131DiscoveryInflator.cpp \
    libs/acn/E131DiscoveryInflator.h \
    libs/acn/E131Header.h \
    libs/acn/E131Inflator.cpp \
    libs/acn/E131Inflator.h \
    libs/acn/E131Node.cpp \
    libs/acn/E131Node.h \
    libs/acn/E131PDU.cpp \
    libs/acn/E131PDU.h \
    libs/acn/E131Sender.cpp \
    libs/acn/E131Sender.h \
    libs/acn/E133Header.h \
    libs/acn/E133Inflator.cpp \
    libs/acn/E133Inflator.h \
    libs/acn/E133PDU.cpp \
    libs/acn/E133PDU.h \
    libs/acn/E133StatusInflator.cpp \
    libs/acn/E133StatusInflator.h \
    libs/acn/E133StatusPDU.cpp \
    libs/acn/E133StatusPDU.h \
    libs/acn/HeaderSet.h \
    libs/acn/LLRPHeader.h \
    libs/acn/LLRPInflator.cpp \
    libs/acn/LLRPInflator.h \
    libs/acn/LLRPProbeReplyInflator.cpp \
    libs/acn/LLRPProbeReplyInflator.h \
    libs/acn/LLRPProbeReplyPDU.cpp \
    libs/acn/LLRPProbeReplyPDU.h \
    libs/acn/LLRPProbeRequestInflator.cpp \
    libs/acn/LLRPProbeRequestInflator.h \
    libs/acn/LLRPProbeRequestPDU.cpp \
    libs/acn/LLRPProbeRequestPDU.h \
    libs/acn/LLRPPDU.cpp \
    libs/acn/LLRPPDU.h \
    libs/acn/PDU.cpp \
    libs/acn/PDU.h \
    libs/acn/PDUTestCommon.h \
    libs/acn/PreamblePacker.cpp \
    libs/acn/PreamblePacker.h \
    libs/acn/RDMInflator.cpp \
    libs/acn/RDMInflator.h \
    libs/acn/RDMPDU.cpp \
    libs/acn/RDMPDU.h \
    libs/acn/RootHeader.h \
    libs/acn/RootInflator.cpp \
    libs/acn/RootInflator.h \
    libs/acn/RootPDU.cpp \
    libs/acn/RootPDU.h \
    libs/acn/RootSender.cpp \
    libs/acn/RootSender.h \
    libs/acn/RPTHeader.h \
    libs/acn/RPTInflator.cpp \
    libs/acn/RPTInflator.h \
    libs/acn/RPTNotificationInflator.h \
    libs/acn/RPTPDU.cpp \
    libs/acn/RPTPDU.h \
    libs/acn/RPTRequestInflator.h \
    libs/acn/RPTRequestPDU.cpp \
    libs/acn/RPTRequestPDU.h \
    libs/acn/TCPTransport.cpp \
    libs/acn/TCPTransport.h \
    libs/acn/Transport.h \
    libs/acn/TransportHeader.h \
    libs/acn/UDPTransport.cpp \
    libs/acn/UDPTransport.h

libs_acn_libolae131core_la_CXXFLAGS = \
    $(COMMON_E131_CXXFLAGS) $(uuid_CFLAGS)
libs_acn_libolae131core_la_LIBADD = $(uuid_LIBS) \
                                    common/libolacommon.la \
                                    libs/acn/libolaacn.la

# PROGRAMS
##################################################
noinst_PROGRAMS += libs/acn/e131_transmit_test \
                   libs/acn/e131_loadtest
libs_acn_e131_transmit_test_SOURCES = \
    libs/acn/e131_transmit_test.cpp \
    libs/acn/E131TestFramework.cpp \
    libs/acn/E131TestFramework.h
libs_acn_e131_transmit_test_LDADD = libs/acn/libolae131core.la

libs_acn_e131_loadtest_SOURCES = libs/acn/e131_loadtest.cpp
libs_acn_e131_loadtest_LDADD = libs/acn/libolae131core.la

# TESTS
##################################################
test_programs += \
    libs/acn/E131Tester \
    libs/acn/E133Tester \
    libs/acn/LLRPTester \
    libs/acn/TransportTester

libs_acn_E131Tester_SOURCES = \
    libs/acn/BaseInflatorTest.cpp \
    libs/acn/CIDTest.cpp \
    libs/acn/DMPAddressTest.cpp \
    libs/acn/DMPInflatorTest.cpp \
    libs/acn/DMPPDUTest.cpp \
    libs/acn/E131InflatorTest.cpp \
    libs/acn/E131PDUTest.cpp \
    libs/acn/HeaderSetTest.cpp \
    libs/acn/PDUTest.cpp \
    libs/acn/RootInflatorTest.cpp \
    libs/acn/RootPDUTest.cpp \
    libs/acn/RootSenderTest.cpp
libs_acn_E131Tester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
# For some completely messed up reason on mac CPPUNIT_LIBS has to come after
# the ossp uuid library.
# CPPUNIT_LIBS contains -ldl which causes the tests to fail in strange ways
libs_acn_E131Tester_LDADD = \
    libs/acn/libolae131core.la \
    $(COMMON_TESTING_LIBS)

libs_acn_E133Tester_SOURCES = \
    libs/acn/BrokerClientEntryPDUTest.cpp \
    libs/acn/BrokerConnectPDUTest.cpp \
    libs/acn/BrokerNullPDUTest.cpp \
    libs/acn/BrokerPDUTest.cpp \
    libs/acn/E133InflatorTest.cpp \
    libs/acn/E133PDUTest.cpp \
    libs/acn/RDMPDUTest.cpp \
    libs/acn/RPTInflatorTest.cpp \
    libs/acn/RPTPDUTest.cpp \
    libs/acn/RPTRequestPDUTest.cpp
libs_acn_E133Tester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
libs_acn_E133Tester_LDADD = \
    libs/acn/libolae131core.la \
    $(COMMON_TESTING_LIBS)

libs_acn_LLRPTester_SOURCES = \
    libs/acn/LLRPInflatorTest.cpp \
    libs/acn/LLRPPDUTest.cpp \
    libs/acn/LLRPProbeReplyPDUTest.cpp \
    libs/acn/LLRPProbeRequestPDUTest.cpp
libs_acn_LLRPTester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
libs_acn_LLRPTester_LDADD = \
    libs/acn/libolae131core.la \
    $(COMMON_TESTING_LIBS)

libs_acn_TransportTester_SOURCES = \
    libs/acn/TCPTransportTest.cpp \
    libs/acn/UDPTransportTest.cpp
libs_acn_TransportTester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
libs_acn_TransportTester_LDADD = libs/acn/libolae131core.la \
                                 $(COMMON_TESTING_LIBS)
