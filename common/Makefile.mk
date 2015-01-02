lib_LTLIBRARIES += common/libolacommon.la

# Variables the included files can append to
# ------------------------------------------
common_libolacommon_la_CXXFLAGS = $(COMMON_CXXFLAGS)
common_libolacommon_la_LIBADD =
common_libolacommon_la_SOURCES =
nodist_common_libolacommon_la_SOURCES =
# ------------------------------------------

if USING_WIN32
common_libolacommon_la_LIBADD += -lWs2_32 -lIphlpapi
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
include common/strings/Makefile.mk
include common/system/Makefile.mk
include common/testing/Makefile.mk
include common/thread/Makefile.mk
include common/timecode/Makefile.mk
include common/utils/Makefile.mk
include common/web/Makefile.mk
