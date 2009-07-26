
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

noinst_LTLIBRARIES = libolautils.la
libolautils_la_SOURCES = StringUtils.cpp DmxBuffer.cpp

TESTS = UtilsTester
check_PROGRAMS = $(TESTS)
UtilsTester_SOURCES = UtilsTester.cpp StringUtilsTest.cpp StringUtils.cpp \
                      DmxBuffer.cpp DmxBufferTest.cpp
UtilsTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
UtilsTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl
UtilsTester_LDADD = ../logging/liblogging.la
