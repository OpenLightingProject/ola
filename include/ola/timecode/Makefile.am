SOURCES = TimeCode.h
BUILT_SOURCES = TimeCodeEnums.h

pkginclude_HEADERS = $(SOURCES) $(BUILT_SOURCES)
pkgincludedir = $(includedir)/ola/timecode/


EXTRA_DIST = $(SOURCES) make_timecode.sh

TimeCodeEnums.h: make_timecode.sh
	sh $(srcdir)/make_timecode.sh

CLEANFILES = TimeCodeEnums.h
