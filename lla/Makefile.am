
INCLUDES = -I$(top_builddir) -I$(top_srcdir)
AM_CFLAGS = -Wall -Werror -Wformat -W 

lib_LTLIBRARIES = liblla.la
liblla_la_SOURCES = lla.c private.h lla.h
liblla_la_LDFLAGS = -version-info 0:0:0


EXTRA_DIST = lla.h plugin.h fdlistener.h fdmanager.h timeoutlistener.h

pkginclude_HEADERS = lla.h messages.h device.h fdlistener.h logger.h \
						pluginadaptor.h plugin.h port.h universe.h preferences.h fdmanager.h

pkgincludedir = $(includedir)/lla


