
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CFLAGS = -Wall -Werror -Wformat -W 

lib_LTLIBRARIES = liblla.la

liblla_la_SOURCES = LlaClient.cpp LlaDevice.cpp LlaPort.cpp 
liblla_la_LDFLAGS = -version-info 1:0:0


EXTRA_DIST = LlaClient.h LlaPlugin.h LlaDevice.h LlaPort.h LlaUniverse.h LlaClientObserver.h
				
pkginclude_HEADERS = LlaClient.h LlaPlugin.h LlaDevice.h LlaPort.h LlaUniverse.h LlaClientObserver.h

pkgincludedir = $(includedir)/lla


