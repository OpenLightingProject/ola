include $(top_srcdir)/common.mk

DEPRECATED = OlaCallbackClient.h OlaDevice.h StreamingClient.h

pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = AutoStart.h OlaClientWrapper.h $(DEPRECATED)

EXTRA_DIST = OlaClientCore.h

lib_LTLIBRARIES = libola.la
libola_la_SOURCES = AutoStart.cpp \
                    ClientRDMAPIShim.cpp \
                    OlaCallbackClient.cpp \
                    OlaClient.cpp \
                    OlaClientCore.cpp \
                    OlaClientWrapper.cpp \
                    StreamingClient.cpp
libola_la_LDFLAGS = -version-info 1:1:0
libola_la_LIBADD = $(top_builddir)/common/libolacommon.la

# Test Programs
if BUILD_TESTS
TESTS = OlaClientTester
endif
check_PROGRAMS = $(TESTS)
OlaClientTester_SOURCES = StreamingClientTest.cpp
OlaClientTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
OlaClientTester_LDADD = $(COMMON_TESTING_LIBS) \
                        $(PLUGIN_LIBS) \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/olad/libolaserver.la \
                        ./libola.la
