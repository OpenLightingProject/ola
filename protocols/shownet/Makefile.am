INCLUDES =  -I$(top_builddir)/include

#AM_CFLAGS = -Wall -Werror -Wformat -W -fprofile-arcs
AM_CFLAGS = -Wall

lib_LTLIBRARIES = libllashownet.la
libllashownet_la_SOURCES = ShowNetNode.cpp RunLengthEncoder.cpp
libllashownet_la_LDFLAGS = -version-info 0:0:0

pkginclude_HEADERS = ShowNetNode.h
pkgincludedir = $(includedir)/lla/shownet


# Test Programs
TESTS = ShowNetTester
check_PROGRAMS = $(TESTS)
ShowNetTester_SOURCES = ShowNetTester.cpp RunLengthEncoderTest.cpp \
                        RunLengthEncoder.cpp
ShowNetTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
ShowNetTester_LDFLAGS = $(CPPUNIT_LIBS)
ShowNetTester_LDADD = @LIBS@ @LIBLTDL@  -lprotobuf ../../common/libllacommon.la
