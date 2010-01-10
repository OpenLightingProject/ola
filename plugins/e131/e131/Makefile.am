EXTRA_DIST = BaseInflator.h CID.h \
             DMPE131Inflator.h DMPAddress.h DMPHeader.h DMPInflator.h DMPPDU.h \
             E131Header.h E131Inflator.h E131Layer.h E131Node.h E131PDU.h \
             HeaderSet.h PDU.h PDUTestCommon.h \
             RootHeader.h RootInflator.h RootLayer.h RootPDU.h \
             UDPTransport.h

INCLUDES = -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wconversion -Wformat -W

noinst_LTLIBRARIES = libolae131core.la
libolae131core_la_SOURCES = BaseInflator.cpp CID.cpp \
                            DMPAddress.cpp DMPE131Inflator.cpp DMPInflator.cpp \
                            DMPPDU.cpp \
                            E131Inflator.cpp E131Layer.cpp E131Node.cpp \
                            E131PDU.cpp PDU.cpp \
                            RootInflator.cpp RootLayer.cpp RootPDU.cpp \
                            UDPTransport.cpp
libolae131core_la_LIBADD = @LIBS@

# test program
#bin_PROGRAMS = e131_test
#e131_test_SOURCES = e131_test.cpp
#e131_test_LDADD = @LIBS@ @LIBLTDL@  -lprotobuf \
#                  ./libolae131core.la \
#                  ../../../common/libolacommon.la
#e131_test_LDFLAGS = $(UUID_LIBS)

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
E131Tester_CXXFLAGS = $(CPPUNIT_CFLAGS)
E131Tester_LDFLAGS = $(CPPUNIT_LIBS) $(UUID_LIBS)
E131Tester_LDADD = @LIBS@ @LIBLTDL@  -lprotobuf \
                   ./libolae131core.la \
                   ../../../common/libolacommon.la
