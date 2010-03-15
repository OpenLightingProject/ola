include $(top_srcdir)/common.mk

EXTRA_DIST = BaseInflator.h CID.h \
             DMPE131Inflator.h DMPAddress.h DMPHeader.h DMPInflator.h DMPPDU.h \
             E131Header.h E131Includes.h E131Inflator.h E131Layer.h E131Node.h \
             E131PDU.h E131TestFramework.h HeaderSet.h PDU.h PDUTestCommon.h \
             RootHeader.h RootInflator.h RootLayer.h RootPDU.h \
             UDPTransport.h

AM_CPPFLAGS += -Wconversion -W

noinst_LTLIBRARIES = libolae131core.la libolae131tester.la
libolae131core_la_SOURCES = BaseInflator.cpp CID.cpp \
                            DMPAddress.cpp DMPE131Inflator.cpp DMPInflator.cpp \
                            DMPPDU.cpp \
                            E131Inflator.cpp E131Layer.cpp E131Node.cpp \
                            E131PDU.cpp PDU.cpp \
                            RootInflator.cpp RootLayer.cpp RootPDU.cpp \
                            UDPTransport.cpp
libolae131core_la_LIBADD = @LIBS@
libolae131core_la_CPPFLAGS = $(UUID_CFLAGS) $(OSSP_UUID_CFLAGS) $(AM_CPPFLAGS)
libolae131tester_la_SOURCES = E131TestFramework.cpp
libolae131tester_la_CPPFLAGS = $(UUID_CFLAGS) $(OSSP_UUID_CFLAGS) \
                               $(AM_CPPFLAGS)

# test program
noinst_PROGRAMS = e131_transmit_test
e131_transmit_test_SOURCES = e131_transmit_test.cpp
e131_transmit_test_CPPFLAGS = $(UUID_CFLAGS) $(OSSP_UUID_CFLAGS) $(AM_CPPFLAGS)
e131_transmit_test_LDADD = @LIBS@ -lprotobuf \
                  ./libolae131core.la \
                  ./libolae131tester.la \
                  ../../../common/libolacommon.la
e131_transmit_test_LDFLAGS = $(UUID_LIBS) $(OSSP_UUID_LIBS)

# Test programs
TESTS = E131Tester
check_PROGRAMS = $(TESTS)
E131Tester_SOURCES = BaseInflatorTest.cpp \
                     CIDTest.cpp \
                     DMPInflatorTest.cpp \
                     DMPAddressTest.cpp \
                     DMPPDUTest.cpp \
                     E131InflatorTest.cpp \
                     E131PDUTest.cpp \
                     E131Tester.cpp \
                     HeaderSetTest.cpp \
                     PDUTest.cpp \
                     RootInflatorTest.cpp \
                     RootLayerTest.cpp \
                     RootPDUTest.cpp \
                     UDPTransportTest.cpp
E131Tester_CPPFLAGS = $(CPPUNIT_CFLAGS) $(UUID_CFLAGS) $(OSSP_UUID_CFLAGS) \
                      $(AM_CPPFLAGS)
# For some completely messed up reason on mac CPPUNIT_LIBS has to come after
# UUID_LIBS. CPPUNIT_LIBS contains -ldl which causes the unittests to fail
E131Tester_LDFLAGS = $(UUID_LIBS) $(OSSP_UUID_LIBS) $(CPPUNIT_LIBS)
E131Tester_LDADD = @LIBS@ -lprotobuf \
                   ./libolae131core.la \
                   ../../../common/libolacommon.la
