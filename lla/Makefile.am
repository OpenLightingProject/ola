
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

HEADER_FILES = LlaClient.h LlaPlugin.h LlaDevice.h LlaPort.h LlaUniverse.h LlaClientObserver.h

lib_LTLIBRARIES = liblla.la
liblla_la_SOURCES = LlaClient.cpp LlaDevice.cpp LlaPort.cpp
liblla_la_LDFLAGS = -version-info 1:0:0
liblla_la_CXXFLAGS = -Wall -Wformat

pkgincludedir = $(includedir)/lla
pkginclude_HEADERS = $(LLA_HEADERS)

EXTRA_DIST = $(LLA_HEADERS)
