include $(top_srcdir)/common.mk

EXTRA_DIST = OlaConfigurator.h RDMController.h RDMHandler.h

if BUILD_EXAMPLES

bin_PROGRAMS = ola_dev_info ola_usbpro ola_artnet ola_e131 \
               ola_streaming_client ola_rdm_discover ola_rdm_get

ola_artnet_SOURCES = ola-artnet.cpp
ola_artnet_LDADD = libolaconfig.la \
                   $(top_builddir)/ola/libola.la \
                   $(top_builddir)/plugins/artnet/messages/libolaartnetconf.la

ola_dev_info_SOURCES = ola-client.cpp
ola_dev_info_LDADD = $(top_builddir)/ola/libola.la \
                     $(top_builddir)/common/libolacommon.la

ola_e131_SOURCES = ola-e131.cpp
ola_e131_LDADD = libolaconfig.la \
                 $(top_builddir)/ola/libola.la \
                 $(top_builddir)/plugins/e131/messages/libolae131conf.la

ola_streaming_client_SOURCES = ola-streaming-client.cpp
ola_streaming_client_LDADD = $(top_builddir)/ola/libola.la \
                             $(top_builddir)/common/libolacommon.la

ola_usbpro_SOURCES = ola-usbpro.cpp
ola_usbpro_LDADD = libolaconfig.la \
                   $(top_builddir)/ola/libola.la \
                   $(top_builddir)/plugins/usbpro/messages/libolausbproconf.la

ola_rdm_get_SOURCES = RDMController.cpp RDMHandler.cpp ola-rdm.cpp
ola_rdm_get_LDADD = $(top_builddir)/ola/libola.la \
                    $(top_builddir)/common/libolacommon.la

ola_rdm_discover_SOURCES = ola-rdm-discover.cpp
ola_rdm_discover_LDADD = $(top_builddir)/ola/libola.la \
                         $(top_builddir)/common/libolacommon.la

if HAVE_NCURSES
bin_PROGRAMS += ola_dmxconsole ola_dmxmonitor
ola_dmxconsole_SOURCES = ola-dmxconsole.cpp
ola_dmxconsole_LDADD = @LIBS@ -lcurses \
                       $(top_builddir)/ola/libola.la \
                       $(top_builddir)/common/libolacommon.la
ola_dmxmonitor_SOURCES = ola-dmxmonitor.cpp
ola_dmxmonitor_LDADD = @LIBS@ -lcurses \
                       $(top_builddir)/ola/libola.la \
                       $(top_builddir)/common/libolacommon.la
endif


noinst_PROGRAMS = ola_throughput
ola_throughput_SOURCES = ola-throughput.cpp
ola_throughput_LDADD = $(top_builddir)/ola/libola.la

noinst_LTLIBRARIES = libolaconfig.la
libolaconfig_la_SOURCES = OlaConfigurator.cpp


install-exec-hook:
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_plugin_info
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_uni_info
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_patch
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_uni_name
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_uni_merge
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_set_dmx
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_set_priority
	$(LN_S) -f $(bindir)/ola_rdm_get $(DESTDIR)$(bindir)/ola_rdm_set

endif
