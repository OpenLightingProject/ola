# The following should match what pkg-config --libs libola returns
LDADD = ../../common/libolacommon.la \
        ../../ola/libola.la

if BUILD_EXAMPLES
noinst_PROGRAMS += \
    doxygen/examples/callback_client_transmit \
    doxygen/examples/flags \
    doxygen/examples/receiver \
    doxygen/examples/streaming_client \
    doxygen/examples/udp_server

if HAVE_DLOPEN
noinst_PROGRAMS += doxygen/examples/streaming_client_plugin
endif
endif

doxygen_examples_callback_client_transmit_SOURCES = \
    doxygen/examples/callback_client_transmit.cpp
doxygen_examples_flags_SOURCES = \
    doxygen/examples/flags.cpp
doxygen_examples_streaming_client_SOURCES = \
    doxygen/examples/streaming_client.cpp
doxygen_examples_streaming_client_plugin_SOURCES = \
    doxygen/examples/streaming_client_plugin.cpp
doxygen_examples_receiver_SOURCES = \
    doxygen/examples/receiver.cpp
doxygen_examples_udp_server_SOURCES = \
    doxygen/examples/udp_server.cpp
