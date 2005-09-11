
INCLUDES = -I$(top_builddir) -I$(top_srcdir)
AM_CFLAGS = -Wall -Werror -Wformat -W 

lib_LTLIBRARIES = liblla.la
liblla_la_SOURCES = lla.c private.h lla.h

EXTRA_DIST = lla.h plugin.h

pkginclude_HEADERS = lla.h messages.h device.h fdlistener.h logger.h \
						pluginadaptor.h plugin.h port.h universe.h

pkgincludedir = $(includedir)/lla


