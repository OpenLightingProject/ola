
INCLUDES = -I$(top_builddir) -I$(top_srcdir)
#AM_CFLAGS = -Wall -Werror -Wformat -W -fprofile-arcs 
AM_CFLAGS = -Wall

lib_LTLIBRARIES = libshownet.la
libshownet_la_SOURCES = shownet.c network.c shownet.h private.h packets.h
libshownet_la_LDFLAGS = -version-info 0:0:0

pkginclude_HEADERS = shownet.h packets.h
pkgincludedir = $(includedir)/shownet
