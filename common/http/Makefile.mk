# LIBRARIES
##################################################
if HAVE_LIBMICROHTTPD
noinst_LTLIBRARIES += libolahttp.la
libolahttp_la_SOURCES = \
    common/http/HTTPServer.cpp \
    common/http/OlaHTTPServer.cpp
libolahttp_la_LIBADD = $(libmicrohttpd_LIBS)
endif
