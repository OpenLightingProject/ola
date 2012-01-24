include $(top_srcdir)/common.mk

noinst_PROGRAMS = rdmpro_sniffer

rdmpro_sniffer_SOURCES = rdm-sniffer.cpp
rdmpro_sniffer_LDADD = $(top_builddir)/common/libolacommon.la \
                      $(top_builddir)/plugins/usbpro/libolausbprowidget.la
rdmpro_sniffer_CXXFLAGS = $(COMMON_CXXFLAGS) @OLAD_DEFINES@
