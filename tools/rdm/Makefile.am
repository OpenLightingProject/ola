include $(top_srcdir)/common.mk

EXTRA_DIST = DmxTriRDMAPI.h

bin_PROGRAMS = rdm_test
rdm_test_SOURCES = rdm_test.cpp DmxTriRDMAPI.cpp
rdm_test_LDFLAGS =
rdm_test_LDADD = @LIBS@ \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/usbpro/libolausbprowidget.la
