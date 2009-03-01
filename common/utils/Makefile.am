
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

noinst_LTLIBRARIES = libllautils.la
libllautils_la_SOURCES = StringUtils.cpp DmxUtils.cpp

TESTS = UtilsTester
check_PROGRAMS = $(TESTS)
UtilsTester_SOURCES = UtilsTester.cpp StringUtilsTest.cpp StringUtils.cpp \
                      DmxUtilsTest.cpp DmxUtils.cpp
UtilsTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
UtilsTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl
