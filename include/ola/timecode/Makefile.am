pkgincludedir = $(includedir)/ola/timecode/

pkginclude_HEADERS = TimeCode.h
nodist_pkginclude_HEADERS = TimeCodeEnums.h

dist_noinst_SCRIPTS = make_timecode.sh

TimeCodeEnums.h: Makefile make_timecode.sh $(top_srcdir)/common/protocol/Ola.proto
	sh $(srcdir)/make_timecode.sh $(top_srcdir)/common/protocol/Ola.proto

CLEANFILES = TimeCodeEnums.h
