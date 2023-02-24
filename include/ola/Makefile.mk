include include/ola/acn/Makefile.mk
include include/ola/base/Makefile.mk
include include/ola/client/Makefile.mk
include include/ola/dmx/Makefile.mk
include include/ola/e133/Makefile.mk
include include/ola/file/Makefile.mk
include include/ola/http/Makefile.mk
include include/ola/io/Makefile.mk
include include/ola/math/Makefile.mk
include include/ola/messaging/Makefile.mk
include include/ola/network/Makefile.mk
include include/ola/rdm/Makefile.mk
include include/ola/rpc/Makefile.mk
include include/ola/stl/Makefile.mk
include include/ola/strings/Makefile.mk
include include/ola/system/Makefile.mk
include include/ola/testing/Makefile.mk
include include/ola/thread/Makefile.mk
include include/ola/timecode/Makefile.mk
include include/ola/util/Makefile.mk
include include/ola/web/Makefile.mk
include include/ola/win/Makefile.mk

dist_noinst_SCRIPTS += \
  include/ola/gen_callbacks.py \
  include/ola/make_plugin_id.sh

pkginclude_HEADERS += \
    include/ola/ActionQueue.h \
    include/ola/BaseTypes.h \
    include/ola/Callback.h \
    include/ola/CallbackRunner.h \
    include/ola/Clock.h \
    include/ola/Constants.h \
    include/ola/DmxBuffer.h \
    include/ola/ExportMap.h \
    include/ola/Logging.h \
    include/ola/MultiCallback.h \
    include/ola/StringUtils.h
nodist_pkginclude_HEADERS += include/ola/plugin_id.h

# BUILT_SOURCES is our only option here since we can't override the generated
# automake rules for common/libolacommon.la
built_sources += include/ola/plugin_id.h

include/ola/plugin_id.h: include/ola/Makefile.mk include/ola/make_plugin_id.sh common/protocol/Ola.proto
	mkdir -p $(top_builddir)/include/ola
	sh $(top_srcdir)/include/ola/make_plugin_id.sh $(top_srcdir)/common/protocol/Ola.proto > $(top_builddir)/include/ola/plugin_id.h

CLEANFILES += \
    include/ola/*.pyc \
    include/ola/__pycache__/*
