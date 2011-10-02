include $(top_srcdir)/common.mk

EXTRA_DIST = ACNPort.h BaseInflator.h CID.h \
             DMPE131Inflator.h DMPE133Inflator.h DMPAddress.h DMPHeader.h \
             DMPInflator.h DMPPDU.h \
             E131Header.h E131Includes.h E131Inflator.h E131Layer.h \
             E131Node.h E131PDU.h E131TestFramework.h \
             E133Header.h E133Inflator.h E133Layer.h E133PDU.h \
             HeaderSet.h PDU.h PDUTestCommon.h RootHeader.h RootInflator.h \
             RootLayer.h RootPDU.h TransportHeader.h UDPTransport.h

COMMON_CXXFLAGS += -Wconversion

noinst_LTLIBRARIES = libolae131core.la
libolae131core_la_SOURCES = BaseInflator.cpp CID.cpp \
                            DMPAddress.cpp DMPE131Inflator.cpp \
                            DMPE133Inflator.cpp DMPInflator.cpp \
                            DMPPDU.cpp \
                            E131Inflator.cpp E131Layer.cpp E131Node.cpp \
                            E131PDU.cpp E133Inflator.cpp E133Layer.cpp \
                            E133PDU.cpp PDU.cpp \
                            RootInflator.cpp RootLayer.cpp RootPDU.cpp \
                            UDPTransport.cpp
libolae131core_la_CXXFLAGS = $(COMMON_CXXFLAGS) $(uuid_CFLAGS)
libolae131core_la_LIBADD = $(uuid_LIBS) \
                           ../../../common/libolacommon.la

# E1.31 dev programs
noinst_PROGRAMS = e131_transmit_test
e131_transmit_test_SOURCES = e131_transmit_test.cpp E131TestFramework.cpp
e131_transmit_test_LDADD = ./libolae131core.la

# Test programs
TESTS = E131Tester
check_PROGRAMS = $(TESTS)
E131Tester_SOURCES = BaseInflatorTest.cpp \
                     CIDTest.cpp \
                     DMPAddressTest.cpp \
                     DMPInflatorTest.cpp \
                     DMPPDUTest.cpp \
                     E131InflatorTest.cpp \
                     E131PDUTest.cpp \
                     E131Tester.cpp \
                     E133InflatorTest.cpp \
                     E133PDUTest.cpp \
                     HeaderSetTest.cpp \
                     PDUTest.cpp \
                     RootInflatorTest.cpp \
                     RootLayerTest.cpp \
                     RootPDUTest.cpp \
                     UDPTransportTest.cpp
E131Tester_CPPFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
# For some completely messed up reason on mac CPPUNIT_LIBS has to come after
# the ossp uuid library.
# CPPUNIT_LIBS contains -ldl which causes the unittests to fail in strange ways
E131Tester_LDADD = ./libolae131core.la \
                   $(CPPUNIT_LIBS)
