# LIBRARIES
##################################################
if HAVE_LIBMICROHTTPD
noinst_LTLIBRARIES += common/http/libolahttp.la
common_http_libolahttp_la_SOURCES = \
    common/http/HTTPServer.cpp \
    common/http/OlaHTTPServer.cpp
common_http_libolahttp_la_LIBADD = $(libmicrohttpd_LIBS)
endif
