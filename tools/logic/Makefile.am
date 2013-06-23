include $(top_srcdir)/common.mk

EXTRA_DIST = DMXSignalProcessor.h

if HAVE_SALEAE_LOGIC
bin_PROGRAMS = logic_rdm_sniffer
endif

logic_rdm_sniffer_SOURCES = DMXSignalProcessor.cpp \
                            logic-rdm-sniffer.cpp
logic_rdm_sniffer_CXXFLAGS = $(COMMON_CXXFLAGS) @OLAD_DEFINES@
logic_rdm_sniffer_LDADD = $(top_builddir)/common/libolacommon.la \
                          $(libSaleaeDevice_LIBS)
