
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

HEADER_FILES = LlaClient.h LlaPlugin.h LlaDevice.h LlaPort.h LlaUniverse.h LlaClientObserver.h common.h

if HAVE_PTHREAD
THREAD_LIBS = -lpthread
endif

lib_LTLIBRARIES = liblla.la
liblla_la_SOURCES = LlaClient.cpp LlaDevice.cpp LlaPort.cpp
liblla_la_LDFLAGS = -version-info 1:0:0 $(THREAD_LIBS)
liblla_la_CXXFLAGS = -Wall -Wformat

pkgincludedir = $(includedir)/lla
pkginclude_HEADERS = $(HEADER_FILES)

EXTRA_DIST = $(HEADER_FILES) common-h.in

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
