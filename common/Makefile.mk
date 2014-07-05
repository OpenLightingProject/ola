lib_LTLIBRARIES += libolacommon.la
libolacommon_la_CXXFLAGS = $(COMMON_CXXFLAGS)
libolacommon_la_LIBADD =
libolacommon_la_SOURCES =
nodist_libolacommon_la_SOURCES =

if USING_WIN32
libolacommon_la_LIBADD += -lWs2_32 -lIphlpap
endif

include common/base/Makefile.mk
include common/dmx/Makefile.mk
include common/export_map/Makefile.mk
include common/file/Makefile.mk
include common/http/Makefile.mk
include common/io/Makefile.mk
include common/math/Makefile.mk
include common/messaging/Makefile.mk
include common/network/Makefile.mk
include common/protocol/Makefile.mk
include common/rdm/Makefile.mk
include common/rpc/Makefile.mk
include common/system/Makefile.mk
include common/testing/Makefile.mk
include common/thread/Makefile.mk
include common/timecode/Makefile.mk
include common/utils/Makefile.mk
include common/web/Makefile.mk
