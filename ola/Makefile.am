include $(top_srcdir)/common.mk

HEADER_FILES = AutoStart.h OlaClient.h OlaCallbackClient.h OlaDevice.h \
               OlaClientWrapper.h StreamingClient.h common.h

pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = $(HEADER_FILES)

EXTRA_DIST = $(HEADER_FILES) OlaClientCore.h common-h.in

lib_LTLIBRARIES = libola.la
libola_la_SOURCES = AutoStart.cpp \
                    OlaClient.cpp \
                    OlaCallbackClient.cpp \
                    OlaClientCore.cpp \
                    OlaClientWrapper.cpp \
                    StreamingClient.cpp
libola_la_LDFLAGS = -version-info 1:1:0
libola_la_LIBADD = $(top_builddir)/common/libolacommon.la

# Test Programs
TESTS = OlaClientTester
check_PROGRAMS = $(TESTS)
OlaClientTester_SOURCES = OlaClientTester.cpp \
                          StreamingClientTest.cpp
OlaClientTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
OlaClientTester_LDADD = $(CPPUNIT_LIBS) \
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

# Don't distribute common.h, since it is build host dependent!
dist-hook:
	rm -f $(distdir)/common.h

install-exec-hook:
	$(LN_S) -f $(includedir)/ola/OlaClientWrapper.h $(DESTDIR)$(includedir)/ola/SimpleClient.h	
