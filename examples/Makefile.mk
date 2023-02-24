# Note: gcc 4.6.1 is pretty strict about library ordering. Libraries need to be
# specified in the order they are used. i.e. a library should depend on things
# to the right, not the left.
# See http://www.network-theory.co.uk/docs/gccintro/gccintro_18.html

if BUILD_EXAMPLES

# The following should match what pkg-config --libs libola returns
EXAMPLE_COMMON_LIBS = common/libolacommon.la \
                      ola/libola.la

# LIBRARIES
##################################################
noinst_LTLIBRARIES += examples/libolaconfig.la
examples_libolaconfig_la_SOURCES = \
    examples/OlaConfigurator.h \
    examples/OlaConfigurator.cpp
examples_libolaconfig_la_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)

# PROGRAMS
##################################################
bin_PROGRAMS += \
    examples/ola_dev_info \
    examples/ola_rdm_discover \
    examples/ola_rdm_get \
    examples/ola_recorder \
    examples/ola_streaming_client \
    examples/ola_timecode \
    examples/ola_uni_stats

if USE_E131
bin_PROGRAMS += examples/ola_e131
examples_ola_e131_SOURCES = examples/ola-e131.cpp
examples_ola_e131_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
examples_ola_e131_LDADD = examples/libolaconfig.la \
                          $(EXAMPLE_COMMON_LIBS) \
                          plugins/e131/messages/libolae131conf.la \
                          $(libprotobuf_LIBS)
endif

if USE_USBPRO
bin_PROGRAMS += examples/ola_usbpro
examples_ola_usbpro_SOURCES = examples/ola-usbpro.cpp
examples_ola_usbpro_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
examples_ola_usbpro_LDADD = examples/libolaconfig.la \
                            $(EXAMPLE_COMMON_LIBS) \
                            plugins/usbpro/messages/libolausbproconf.la \
                            $(libprotobuf_LIBS)
endif

if USE_ARTNET
bin_PROGRAMS += examples/ola_artnet
examples_ola_artnet_SOURCES = examples/ola-artnet.cpp
examples_ola_artnet_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS)
examples_ola_artnet_LDADD = examples/libolaconfig.la \
                            $(EXAMPLE_COMMON_LIBS) \
                            plugins/artnet/messages/libolaartnetconf.la \
                            $(libprotobuf_LIBS)
endif

examples_ola_dev_info_SOURCES = examples/ola-client.cpp
examples_ola_dev_info_LDADD = $(EXAMPLE_COMMON_LIBS)

examples_ola_streaming_client_SOURCES = examples/ola-streaming-client.cpp
examples_ola_streaming_client_LDADD = $(EXAMPLE_COMMON_LIBS)

examples_ola_rdm_get_SOURCES = examples/ola-rdm.cpp
examples_ola_rdm_get_LDADD = $(EXAMPLE_COMMON_LIBS)

examples_ola_rdm_discover_SOURCES = examples/ola-rdm-discover.cpp
examples_ola_rdm_discover_LDADD = $(EXAMPLE_COMMON_LIBS)

examples_ola_recorder_SOURCES = \
    examples/ola-recorder.cpp \
    examples/ShowLoader.h \
    examples/ShowLoader.cpp \
    examples/ShowPlayer.h \
    examples/ShowPlayer.cpp \
    examples/ShowRecorder.h \
    examples/ShowRecorder.cpp \
    examples/ShowSaver.h \
    examples/ShowSaver.cpp
examples_ola_recorder_LDADD = $(EXAMPLE_COMMON_LIBS)

examples_ola_timecode_SOURCES = examples/ola-timecode.cpp
examples_ola_timecode_LDADD = $(EXAMPLE_COMMON_LIBS)

examples_ola_uni_stats_SOURCES = examples/ola-uni-stats.cpp
examples_ola_uni_stats_LDADD = $(EXAMPLE_COMMON_LIBS)

if HAVE_NCURSES
bin_PROGRAMS += examples/ola_dmxconsole examples/ola_dmxmonitor
examples_ola_dmxconsole_SOURCES = examples/ola-dmxconsole.cpp
examples_ola_dmxmonitor_SOURCES = examples/ola-dmxmonitor.cpp
if HAVE_NCURSES_PKGCONFIG
examples_ola_dmxconsole_LDADD = $(EXAMPLE_COMMON_LIBS) $(libncurses_LIBS)
examples_ola_dmxmonitor_LDADD = $(EXAMPLE_COMMON_LIBS) $(libncurses_LIBS)
else
# Fallback when pkg-config didn't know about ncurses
examples_ola_dmxconsole_LDADD = $(EXAMPLE_COMMON_LIBS) -lncurses
examples_ola_dmxmonitor_LDADD = $(EXAMPLE_COMMON_LIBS) -lncurses
endif
endif

noinst_PROGRAMS += examples/ola_throughput examples/ola_latency
examples_ola_throughput_SOURCES = examples/ola-throughput.cpp
examples_ola_throughput_LDADD = $(EXAMPLE_COMMON_LIBS)
examples_ola_latency_SOURCES = examples/ola-latency.cpp
examples_ola_latency_LDADD = $(EXAMPLE_COMMON_LIBS)

if USING_WIN32
# rename this program, otherwise UAC will block it
OLA_PATCH_NAME = ola_ptch
else
OLA_PATCH_NAME = ola_patch
endif

# Many of the example programs are just symlinks to ola_dev_info
install-exec-hook-examples:
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/$(OLA_PATCH_NAME)
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_plugin_info
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_set_dmx
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_set_priority
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_uni_info
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_uni_merge
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_uni_name
	$(LN_S) -f $(bindir)/ola_dev_info $(DESTDIR)$(bindir)/ola_plugin_state
	$(LN_S) -f $(bindir)/ola_rdm_get $(DESTDIR)$(bindir)/ola_rdm_set

INSTALL_EXEC_HOOKS += install-exec-hook-examples

# TESTS_DATA
##################################################

EXTRA_DIST += \
    examples/testdata/dos_line_endings \
    examples/testdata/multiple_unis \
    examples/testdata/partial_frames \
    examples/testdata/single_uni \
    examples/testdata/trailing_timeout

# TESTS
##################################################
test_scripts += examples/RecorderVerifyTest.sh

examples/RecorderVerifyTest.sh: examples/Makefile.mk
	echo "for FILE in ${srcdir}/examples/testdata/dos_line_endings ${srcdir}/examples/testdata/multiple_unis ${srcdir}/examples/testdata/partial_frames ${srcdir}/examples/testdata/single_uni ${srcdir}/examples/testdata/trailing_timeout; do echo \"Checking \$$FILE\"; ${top_builddir}/examples/ola_recorder${EXEEXT} --verify \$$FILE; STATUS=\$$?; if [ \$$STATUS -ne 0 ]; then echo \"FAIL: \$$FILE caused ola_recorder to exit with status \$$STATUS\"; exit \$$STATUS; fi; done; exit 0" > examples/RecorderVerifyTest.sh
	chmod +x examples/RecorderVerifyTest.sh

CLEANFILES += examples/RecorderVerifyTest.sh
endif
