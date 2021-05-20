if HAVE_SALEAE_LOGIC
bin_PROGRAMS += tools/logic/logic_rdm_sniffer
endif

if HAVE_LIBSIGROK
bin_PROGRAMS += tools/logic/sigrok_rdm_sniffer
endif

tools_logic_logic_rdm_sniffer_SOURCES = \
    tools/logic/BaseSnifferReader.cpp \
    tools/logic/BaseSnifferReader.h \
    tools/logic/DMXSignalProcessor.cpp \
    tools/logic/DMXSignalProcessor.h \
    tools/logic/logic-rdm-sniffer.cpp
tools_logic_logic_rdm_sniffer_LDADD = common/libolacommon.la \
                                      $(libSaleaeDevice_LIBS)

tools_logic_sigrok_rdm_sniffer_SOURCES = \
    tools/logic/BaseSnifferReader.cpp \
    tools/logic/BaseSnifferReader.h \
    tools/logic/DMXSignalProcessor.cpp \
    tools/logic/DMXSignalProcessor.h \
    tools/logic/sigrok-rdm-sniffer.cpp
tools_logic_sigrok_rdm_sniffer_CXXFLAGS = $(COMMON_CXXFLAGS) \
                                          $(libsigrok_CFLAGS)
tools_logic_sigrok_rdm_sniffer_LDADD = common/libolacommon.la \
                                       $(libsigrok_LIBS)

EXTRA_DIST += tools/logic/README.md
