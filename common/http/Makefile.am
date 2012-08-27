include $(top_srcdir)/common.mk

if HAVE_LIBMICROHTTPD
lib_LTLIBRARIES = libolahttp.la
libolahttp_la_SOURCES = HTTPServer.cpp OlaHTTPServer.cpp
libolahttp_la_LIBADD = $(libmicrohttpd_LIBS)
endif

