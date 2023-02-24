bin_PROGRAMS += tools/rdmpro/rdmpro_sniffer

tools_rdmpro_rdmpro_sniffer_SOURCES = tools/rdmpro/rdm-sniffer.cpp
tools_rdmpro_rdmpro_sniffer_LDADD = common/libolacommon.la \
                                    plugins/usbpro/libolausbprowidget.la

EXTRA_DIST += tools/rdmpro/README.md
