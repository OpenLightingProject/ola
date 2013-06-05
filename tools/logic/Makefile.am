include $(top_srcdir)/common.mk

if HAVE_SALEAE_LOGIC
bin_PROGRAMS = logic_rdm_sniffer
endif

logic_rdm_sniffer_SOURCES = logic-rdm-sniffer.cpp
logic_rdm_sniffer_LDADD = $(top_builddir)/common/libolacommon.la \
                          $(libSaleaeDevice_LIBS)
