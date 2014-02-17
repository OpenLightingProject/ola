include $(top_srcdir)/common.mk

COMMON_CXXFLAGS += -Wconversion

# pkg-config
##################################################
pkgconfigdir = $(libdir)/pkgconfig
if INSTALL_E133
pkgconfig_DATA = libolaacn.pc
endif

# LIBRARIES
##################################################
lib_LTLIBRARIES =
noinst_LTLIBRARIES =

if INSTALL_ACN
  lib_LTLIBRARIES += libolaacn.la
else
  noinst_LTLIBRARIES += libolaacn.la
endif

# libolaacn.la
libolaacn_la_SOURCES = CID.cpp CIDImpl.cpp CIDImpl.h
libolaacn_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(uuid_CFLAGS)
libolaacn_la_LIBADD = $(uuid_LIBS) \
                      ../../../common/libolacommon.la

# libolae131core.la
# This needs to be after libolaacn.la since it depends on it. Otherwise it
# breaks the freeBSD build
noinst_LTLIBRARIES += libolae131core.la
libolae131core_la_SOURCES = BaseInflator.cpp BaseInflator.h DMPAddress.cpp \
                            DMPAddress.h DMPE131Inflator.cpp \
                            DMPE131Inflator.h DMPHeader.h DMPInflator.cpp \
                            DMPInflator.h DMPPDU.cpp DMPPDU.h E131Header.h \
                            E131Inflator.cpp E131Inflator.h E131Node.cpp \
                            E131Node.h E131PDU.cpp E131PDU.h E131Sender.cpp \
                            E131Sender.h E133Header.h E133Inflator.cpp \
                            E133Inflator.h E133PDU.cpp E133PDU.h \
                            E133StatusInflator.cpp E133StatusInflator.h \
                            E133StatusPDU.cpp E133StatusPDU.h HeaderSet.h \
                            PDU.cpp PDU.h PDUTestCommon.h PreamblePacker.cpp \
                            PreamblePacker.h RDMInflator.cpp RDMInflator.h \
                            RDMPDU.cpp RDMPDU.h RootHeader.h RootInflator.cpp \
                            RootInflator.h RootPDU.cpp RootPDU.h \
                            RootSender.cpp RootSender.h TCPTransport.cpp \
                            TCPTransport.h Transport.h TransportHeader.h \
                            UDPTransport.cpp UDPTransport.h
libolae131core_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(uuid_CFLAGS)
libolae131core_la_LIBADD = $(uuid_LIBS) \
                           ../../../common/libolacommon.la \
                           libolaacn.la

# PROGRAMS
##################################################
noinst_PROGRAMS = e131_transmit_test e131_loadtest
e131_transmit_test_SOURCES = e131_transmit_test.cpp E131TestFramework.cpp \
                             E131TestFramework.h
e131_transmit_test_LDADD = libolae131core.la

e131_loadtest_SOURCES = e131_loadtest.cpp
e131_loadtest_LDADD = libolae131core.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = E131Tester E133Tester TransportTester
endif
check_PROGRAMS = $(TESTS)

E131Tester_SOURCES = BaseInflatorTest.cpp \
                     CIDTest.cpp \
                     DMPAddressTest.cpp \
                     DMPInflatorTest.cpp \
                     DMPPDUTest.cpp \
                     E131InflatorTest.cpp \
                     E131PDUTest.cpp \
                     HeaderSetTest.cpp \
                     PDUTest.cpp \
                     RootInflatorTest.cpp \
                     RootPDUTest.cpp \
                     RootSenderTest.cpp
E131Tester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
# For some completely messed up reason on mac CPPUNIT_LIBS has to come after
# the ossp uuid library.
# CPPUNIT_LIBS contains -ldl which causes the unittests to fail in strange ways
E131Tester_LDADD = libolae131core.la $(COMMON_TESTING_LIBS)

E133Tester_SOURCES = E133InflatorTest.cpp \
                     E133PDUTest.cpp \
                     RDMPDUTest.cpp
E133Tester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
E133Tester_LDADD = libolae131core.la $(COMMON_TESTING_LIBS)

TransportTester_SOURCES = TCPTransportTest.cpp \
                          UDPTransportTest.cpp
TransportTester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
TransportTester_LDADD = libolae131core.la $(COMMON_TESTING_LIBS)
