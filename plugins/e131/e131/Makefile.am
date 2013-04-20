include $(top_srcdir)/common.mk

EXTRA_DIST = BaseInflator.h CIDImpl.h \
             DMPE131Inflator.h DMPAddress.h DMPHeader.h \
             DMPInflator.h DMPPDU.h \
             E131Header.h E131Includes.h E131Inflator.h E131Sender.h \
             E131Node.h E131PDU.h E131TestFramework.h \
             E133Header.h E133Inflator.h E133PDU.h \
             E133StatusInflator.h E133StatusPDU.h \
             HeaderSet.h PreamblePacker.h PDU.h PDUTestCommon.h RDMInflator.h \
             RDMPDU.h RootHeader.h RootInflator.h RootSender.h RootPDU.h \
             TCPTransport.h Transport.h TransportHeader.h UDPTransport.h

COMMON_CXXFLAGS += -Wconversion

pkgconfigdir = $(libdir)/pkgconfig

if INSTALL_E133
pkgconfig_DATA = libolaacn.pc
endif

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

if INSTALL_ACN
  lib_LTLIBRARIES = libolaacn.la
else
  noinst_LTLIBRARIES += libolaacn.la
endif

# This needs to be after libolaacn.la since it depends on it. Otherwise it
# breaks the freeBSD build
noinst_LTLIBRARIES += libolae131core.la

# libolaacn.la
libolaacn_la_SOURCES = CID.cpp \
                       CIDImpl.cpp
libolaacn_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(uuid_CFLAGS)
libolaacn_la_LIBADD = $(uuid_LIBS) \
                      ../../../common/libolacommon.la

# libolae131core.la
libolae131core_la_SOURCES = BaseInflator.cpp \
                            DMPAddress.cpp DMPE131Inflator.cpp \
                            DMPInflator.cpp \
                            DMPPDU.cpp \
                            E131Inflator.cpp E131Sender.cpp E131Node.cpp \
                            E131PDU.cpp E133Inflator.cpp \
                            E133PDU.cpp \
                            E133StatusInflator.cpp \
                            E133StatusPDU.cpp \
                            PreamblePacker.cpp \
                            PDU.cpp \
                            RDMInflator.cpp \
                            RDMPDU.cpp \
                            RootInflator.cpp RootSender.cpp RootPDU.cpp \
                            TCPTransport.cpp \
                            UDPTransport.cpp
libolae131core_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(uuid_CFLAGS)
libolae131core_la_LIBADD = $(uuid_LIBS) \
                           ../../../common/libolacommon.la \
                           ./libolaacn.la

# E1.31 dev programs
noinst_PROGRAMS = e131_transmit_test e131_loadtest
e131_transmit_test_SOURCES = e131_transmit_test.cpp E131TestFramework.cpp
e131_transmit_test_LDADD = ./libolae131core.la

e131_loadtest_SOURCES = e131_loadtest.cpp
e131_loadtest_LDADD = ./libolae131core.la

# Test programs
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
E131Tester_LDADD = ./libolae131core.la $(COMMON_TESTING_LIBS)

E133Tester_SOURCES = E133InflatorTest.cpp \
                     E133PDUTest.cpp \
                     RDMPDUTest.cpp
E133Tester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
E133Tester_LDADD = ./libolae131core.la $(COMMON_TESTING_LIBS)

TransportTester_SOURCES = TCPTransportTest.cpp \
                          UDPTransportTest.cpp
TransportTester_CPPFLAGS = $(COMMON_TESTING_FLAGS)
TransportTester_LDADD = ./libolae131core.la $(COMMON_TESTING_LIBS)
