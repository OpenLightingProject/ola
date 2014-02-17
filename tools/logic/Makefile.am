include $(top_srcdir)/common.mk

if HAVE_SALEAE_LOGIC
bin_PROGRAMS = logic_rdm_sniffer
endif

logic_rdm_sniffer_SOURCES = DMXSignalProcessor.cpp DMXSignalProcessor.h \
                            logic-rdm-sniffer.cpp
logic_rdm_sniffer_LDADD = ../../common/libolacommon.la \
                          $(libSaleaeDevice_LIBS)
