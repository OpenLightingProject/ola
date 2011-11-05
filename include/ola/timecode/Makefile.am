SOURCES = TimeCode.h

pkginclude_HEADERS = $(SOURCES)
pkgincludedir = $(includedir)/ola/timecode/

BUILT_SOURCES = TimeCodeEnums.h

EXTRA_DIST = $(SOURCES) make_timecode.sh

TimeCodeEnums.h:
	sh ./make_timecode.sh

clean-local:
	rm -f TimeCodeEnums.h
