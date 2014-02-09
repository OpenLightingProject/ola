pkgincludedir = $(includedir)/ola/timecode/

BUILT_SOURCES = TimeCodeEnums.h

pkginclude_HEADERS = TimeCode.h
nodist_pkginclude_HEADERS = $(BUILT_SOURCES)

EXTRA_DIST = make_timecode.sh

TimeCodeEnums.h: make_timecode.sh $(top_srcdir)/common/protocol/Ola.proto
	sh $(srcdir)/make_timecode.sh

CLEANFILES = TimeCodeEnums.h
