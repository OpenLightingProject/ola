# The following should match what pkg-config --libs libola returns
DOXYGEN_EXAMPLES_LDADD = common/libolacommon.la \
                         ola/libola.la

if BUILD_EXAMPLES
noinst_PROGRAMS += \
    doxygen/examples/callback_client_transmit \
    doxygen/examples/client_disconnect \
    doxygen/examples/client_thread \
    doxygen/examples/fetch_plugins \
    doxygen/examples/flags \
    doxygen/examples/legacy_callback_client_transmit \
    doxygen/examples/legacy_receiver \
    doxygen/examples/legacy_streaming_client \
    doxygen/examples/receiver \
    doxygen/examples/stdin_handler \
    doxygen/examples/streaming_client \
    doxygen/examples/udp_server

if HAVE_DLOPEN
noinst_PROGRAMS += doxygen/examples/streaming_client_plugin
endif
endif

doxygen_examples_callback_client_transmit_SOURCES = \
    doxygen/examples/callback_client_transmit.cpp
doxygen_examples_callback_client_transmit_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_client_disconnect_SOURCES = \
    doxygen/examples/client_disconnect.cpp
doxygen_examples_client_disconnect_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_client_thread_SOURCES = \
    doxygen/examples/client_thread.cpp
doxygen_examples_client_thread_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_fetch_plugins_SOURCES = \
    doxygen/examples/fetch_plugins.cpp
doxygen_examples_fetch_plugins_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_legacy_callback_client_transmit_SOURCES = \
    doxygen/examples/legacy_callback_client_transmit.cpp
doxygen_examples_legacy_callback_client_transmit_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_flags_SOURCES = \
    doxygen/examples/flags.cpp
doxygen_examples_flags_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_streaming_client_SOURCES = \
    doxygen/examples/streaming_client.cpp
doxygen_examples_streaming_client_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_legacy_streaming_client_SOURCES = \
    doxygen/examples/legacy_streaming_client.cpp
doxygen_examples_legacy_streaming_client_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_streaming_client_plugin_SOURCES = \
    doxygen/examples/streaming_client_plugin.cpp
doxygen_examples_streaming_client_plugin_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_legacy_receiver_SOURCES = \
    doxygen/examples/legacy_receiver.cpp
doxygen_examples_legacy_receiver_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_receiver_SOURCES = \
    doxygen/examples/receiver.cpp
doxygen_examples_receiver_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_stdin_handler_SOURCES = \
    doxygen/examples/stdin_handler.cpp
doxygen_examples_stdin_handler_LDADD = $(DOXYGEN_EXAMPLES_LDADD)

doxygen_examples_udp_server_SOURCES = \
    doxygen/examples/udp_server.cpp
doxygen_examples_udp_server_LDADD = $(DOXYGEN_EXAMPLES_LDADD)
