olatimecodeincludedir = $(pkgincludedir)/timecode/

olatimecodeinclude_HEADERS = include/ola/timecode/TimeCode.h
nodist_olatimecodeinclude_HEADERS = include/ola/timecode/TimeCodeEnums.h

dist_noinst_SCRIPTS += include/ola/timecode/make_timecode.sh

# BUILT_SOURCES is our only option here since we can't override the generated
# automake rules for common/libolacommon.la
BUILT_SOURCES += include/ola/timecode/TimeCodeEnums.h

include/ola/timecode/TimeCodeEnums.h: include/ola/timecode/Makefile.mk include/ola/timecode/make_timecode.sh $(top_srcdir)/common/protocol/Ola.proto
	sh $(srcdir)/include/ola/timecode/make_timecode.sh $(top_srcdir)/common/protocol/Ola.proto > include/ola/timecode/TimeCodeEnums.h

CLEANFILES += include/ola/timecode/TimeCodeEnums.h
