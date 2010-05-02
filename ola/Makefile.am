include $(top_srcdir)/common.mk

HEADER_FILES = OlaClient.h OlaDevice.h SimpleClient.h StreamingClient.h \
               common.h

lib_LTLIBRARIES = libola.la
libola_la_SOURCES = OlaClient.cpp \
                    OlaClientServiceImpl.cpp OlaClientCore.cpp \
                    SimpleClient.cpp StreamingClient.cpp
libola_la_LDFLAGS = -version-info 1:1:0
libola_la_LIBADD = $(top_builddir)/common/libolacommon.la

pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = $(HEADER_FILES)

EXTRA_DIST = $(HEADER_FILES) OlaClientServiceImpl.h OlaClientCore.h \
             common-h.in


# Test Programs
TESTS = OlaClientTester
check_PROGRAMS = $(TESTS)
OlaClientTester_SOURCES = $(OLASERVER_SOURCES) OlaClientTester.cpp \
                          StreamingClientTest.cpp
OlaClientTester_CXXFLAGS = $(CPPUNIT_CFLAGS) $(LIBUSB_CFLAGS)
OlaClientTester_LDFLAGS = $(CPPUNIT_LIBS) -ldl -lprotobuf
OlaClientTester_LDADD = @LIBS@ ${PLUGIN_LIBS} \
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

# Don't distribute common.h, since it is build host dependent!
dist-hook:
	rm -f $(distdir)/common.h
