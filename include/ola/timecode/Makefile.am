SOURCES = TimeCode.h
BUILT_SOURCES = TimeCodeEnums.h

pkginclude_HEADERS = $(SOURCES) $(BUILT_SOURCES)
pkgincludedir = $(includedir)/ola/timecode/


EXTRA_DIST = $(SOURCES) make_timecode.sh

TimeCodeEnums.h:
	sh ./make_timecode.sh

clean-local:
	rm -f TimeCodeEnums.h
