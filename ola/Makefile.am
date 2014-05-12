include $(top_srcdir)/common.mk

# These headers are deprecated and will be removed 6 months after the 0.9.0
# release.
DEPRECATED = OlaCallbackClient.h OlaDevice.h StreamingClient.h

# HEADERS
##################################################
pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = AutoStart.h OlaClientWrapper.h $(DEPRECATED)

# LIBRARIES
##################################################
lib_LTLIBRARIES = libola.la
libola_la_SOURCES = AutoStart.cpp \
                    ClientRDMAPIShim.cpp \
                    OlaCallbackClient.cpp \
                    OlaClient.cpp \
                    OlaClientCore.h \
                    OlaClientCore.cpp \
                    OlaClientWrapper.cpp \
                    StreamingClient.cpp
libola_la_LDFLAGS = -version-info 1:1:0
libola_la_LIBADD = ../common/libolacommon.la

# TESTS
##################################################
if BUILD_TESTS
TESTS = OlaClientTester
endif
check_PROGRAMS = $(TESTS)

OlaClientTester_SOURCES = StreamingClientTest.cpp
OlaClientTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
OlaClientTester_LDADD = $(COMMON_TESTING_LIBS) \
                        $(PLUGIN_LIBS) \
                        ../common/libolacommon.la \
                        ../olad/libolaserver.la \
                        ./libola.la
if USING_WIN32
OlaClientTester_LDADD += -lWs2_32 -lIphlpapi
endif
