include $(top_srcdir)/common.mk

# Note: gcc 4.6.1 is pretty strict about library ordering. Libraries need to be
# ordered in the order they are used. i.e. a library should depend on things to
# the right, not the left.
# See http://www.network-theory.co.uk/docs/gccintro/gccintro_18.html

EXTRA_DIST = OlaConfigurator.h ShowLoader.h ShowPlayer.h ShowRecorder.h \
             ShowSaver.h

if BUILD_EXAMPLES

# The following should match what pkg-config --libs libola returns
COMMON_LIBS = $(top_builddir)/common/libolacommon.la \
              $(top_builddir)/ola/libola.la
LDADD = $(COMMON_LIBS)

bin_PROGRAMS = ola_dev_info ola_usbpro ola_e131 \
               ola_streaming_client ola_rdm_discover ola_rdm_get \
               ola_recorder ola_timecode ola_uni_stats

if USE_ARTNET
bin_PROGRAMS += ola_artnet
ola_artnet_SOURCES = ola-artnet.cpp
ola_artnet_LDADD = libolaconfig.la $(COMMON_LIBS) $(libprotobuf_LIBS) \
                   $(top_builddir)/plugins/artnet/messages/libolaartnetconf.la
endif

ola_dev_info_SOURCES = ola-client.cpp

ola_e131_SOURCES = ola-e131.cpp
ola_e131_LDADD = libolaconfig.la $(COMMON_LIBS) $(libprotobuf_LIBS) \
                 $(top_builddir)/plugins/e131/messages/libolae131conf.la

ola_streaming_client_SOURCES = ola-streaming-client.cpp

ola_usbpro_SOURCES = ola-usbpro.cpp
ola_usbpro_LDADD = libolaconfig.la $(COMMON_LIBS) $(libprotobuf_LIBS) \
                   $(top_builddir)/plugins/usbpro/messages/libolausbproconf.la

ola_rdm_get_SOURCES = ola-rdm.cpp
ola_rdm_get_CXXFLAGS = $(COMMON_CXXFLAGS) @OLAD_DEFINES@

ola_rdm_discover_SOURCES = ola-rdm-discover.cpp

ola_recorder_SOURCES = ola-recorder.cpp ShowLoader.cpp ShowPlayer.cpp \
                       ShowRecorder.cpp ShowSaver.cpp

ola_timecode_SOURCES = ola-timecode.cpp

ola_uni_stats_SOURCES = ola-uni-stats.cpp

if HAVE_NCURSES
bin_PROGRAMS += ola_dmxconsole ola_dmxmonitor
ola_dmxconsole_SOURCES = ola-dmxconsole.cpp
ola_dmxconsole_LDADD = @LIBS@ -lcurses $(COMMON_LIBS)
ola_dmxmonitor_SOURCES = ola-dmxmonitor.cpp
ola_dmxmonitor_LDADD = @LIBS@ -lcurses $(COMMON_LIBS)
endif

noinst_PROGRAMS = ola_throughput ola_latency
ola_throughput_SOURCES = ola-throughput.cpp
ola_latency_SOURCES = ola-latency.cpp

noinst_LTLIBRARIES = libolaconfig.la
libolaconfig_la_SOURCES = OlaConfigurator.cpp


install-exec-hook:
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_patch
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_plugin_info
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_set_dmx
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_set_priority
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_uni_info
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_uni_merge
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_uni_name
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_plugin_state
	$(LN_S) -f $(bindir)/ola_rdm_get $(DESTDIR)$(bindir)/ola_rdm_set

endif
