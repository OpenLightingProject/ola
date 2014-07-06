olatimecodeincludedir = $(pkgincludedir)/timecode/

olatimecodeinclude_HEADERS = include/ola/timecode/TimeCode.h
nodist_olatimecodeinclude_HEADERS = include/ola/timecode/TimeCodeEnums.h

dist_noinst_SCRIPTS += include/ola/timecode/make_timecode.sh

# BUILT_SOURCES is our only option here since we can't override the generated
# automake rules for common/libolacommon.la
built_sources += include/ola/timecode/TimeCodeEnums.h

include/ola/timecode/TimeCodeEnums.h: include/ola/timecode/Makefile.mk include/ola/timecode/make_timecode.sh common/protocol/Ola.proto
	mkdir -p $(top_builddir)/include/ola/timecode
	sh $(top_srcdir)/include/ola/timecode/make_timecode.sh $(top_srcdir)/common/protocol/Ola.proto > $(top_builddir)/include/ola/timecode/TimeCodeEnums.h
