include $(top_srcdir)/common.mk

noinst_LTLIBRARIES = libolautils.la
libolautils_la_SOURCES = DmxBuffer.cpp RunLengthEncoder.cpp StringUtils.cpp

TESTS = UtilsTester
check_PROGRAMS = $(TESTS)
UtilsTester_SOURCES = DmxBuffer.cpp DmxBufferTest.cpp RunLengthEncoder.cpp \
                      RunLengthEncoderTest.cpp StringUtils.cpp \
                      StringUtilsTest.cpp UtilsTester.cpp
UtilsTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
UtilsTester_LDFLAGS = $(CPPUNIT_LIBS)
UtilsTester_LDADD = ../logging/liblogging.la
