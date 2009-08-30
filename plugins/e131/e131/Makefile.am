libdir = $(plugindir)
EXTRA_DIST = BaseInflator.h CID.h HeaderSet.h \
             PDU.h PDUTest.h PDUTestCommon.h \
             RootHeader.h RootPDU.h # RootLayer.h  E131Core.h 

INCLUDES = -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Wformat -W

lib_LTLIBRARIES = libolae131core.la
libolae131core_la_SOURCES = BaseInflator.cpp CID.cpp RootPDU.cpp #E131Node.cpp RootLayer.h
libolae131core_la_LIBADD = @LIBS@

# Test programs
TESTS = E131Tester
check_PROGRAMS = $(TESTS)
E131Tester_SOURCES = E131Tester.cpp \
                     BaseInflatorTest.cpp BaseInflator.cpp \
                     CIDTest.cpp CID.cpp \
                     HeaderSetTest.cpp \
                     PDUTest.cpp \
                     RootPDU.cpp RootPDUTest.cpp
E131Tester_CXXFLAGS = $(CPPUNIT_CFLAGS)
E131Tester_LDFLAGS = $(CPPUNIT_LIBS)
E131Tester_LDADD = @LIBS@ @LIBLTDL@  -lprotobuf \
                   ../../../common/libolacommon.la
