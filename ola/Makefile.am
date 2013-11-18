include $(top_srcdir)/common.mk

DEPRECATED = OlaCallbackClient.h OlaDevice.h StreamingClient.h
HEADER_FILES = AutoStart.h OlaClientWrapper.h common.h $(DEPRECATED)

pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = $(HEADER_FILES)

EXTRA_DIST = $(HEADER_FILES) OlaClientCore.h common-h.in

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

## @end 1
## @start 2
# Regenerate common.h with config.status whenever common-h.in changes.
common.h: stamp-common
	@:
stamp-common: $(srcdir)/common-h.in $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES= CONFIG_HEADERS= CONFIG_OTHER=sic/common.h \
	  $(SHELL) ./config.status
	echo timestamp > $@
## @end 2
distclean-local:
	rm -f $(builddir)/common.h $(builddir)/stamp-common

# Don't distribute common.h, since it is build host dependent!
dist-hook:
	rm -f $(distdir)/common.h
