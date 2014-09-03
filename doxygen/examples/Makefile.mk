# The following should match what pkg-config --libs libola returns
DOXYGEN_EXAMPLES_LDADD = common/libolacommon.la \
                         ola/libola.la

if BUILD_EXAMPLES
noinst_PROGRAMS += \
    doxygen/examples/callback_client_transmit \
    doxygen/examples/flags \
    doxygen/examples/legacy_receiver \
    doxygen/examples/receiver \
    doxygen/examples/streaming_client \
    doxygen/examples/udp_server

if HAVE_DLOPEN
noinst_PROGRAMS += doxygen/examples/streaming_client_plugin
endif
endif

doxygen_examples_callback_client_transmit_SOURCES = \
    doxygen/examples/callback_client_transmit.cpp
doxygen_examples_callback_client_transmit_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_flags_SOURCES = \
    doxygen/examples/flags.cpp
doxygen_examples_flags_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_streaming_client_SOURCES = \
    doxygen/examples/streaming_client.cpp
doxygen_examples_streaming_client_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_streaming_client_plugin_SOURCES = \
    doxygen/examples/streaming_client_plugin.cpp
doxygen_examples_streaming_client_plugin_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_legacy_receiver_SOURCES = \
    doxygen/examples/legacy_receiver.cpp
doxygen_examples_legacy_receiver_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_receiver_SOURCES = \
    doxygen/examples/receiver.cpp
doxygen_examples_receiver_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_udp_server_SOURCES = \
    doxygen/examples/udp_server.cpp
doxygen_examples_udp_server_LDADD = $(DOXYGEN_EXAMPLES_LDADD)
