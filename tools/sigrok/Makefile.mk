if HAVE_LIBSIGROK
bin_PROGRAMS += tools/sigrok/sigrok_rdm_sniffer
endif

tools_sigrok_sigrok_rdm_sniffer_SOURCES = \
    tools/sigrok/sigrok-rdm-sniffer.cpp
tools_sigrok_sigrok_rdm_sniffer_CXXFLAGS = $(COMMON_CXXFLAGS) \
                                           $(libsigrok_CFLAGS)
tools_sigrok_sigrok_rdm_sniffer_LDADD = common/libolacommon.la \
                                        libs/sniffer/libolasniffer.la \
                                        $(libsigrok_LIBS)

EXTRA_DIST += tools/sigrok/README.md
