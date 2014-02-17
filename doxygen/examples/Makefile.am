include $(top_srcdir)/common.mk

# The following should match what pkg-config --libs libola returns
LDADD = ../../common/libolacommon.la \
        ../../ola/libola.la

if BUILD_EXAMPLES
noinst_PROGRAMS =  callback_client_transmit flags receiver streaming_client
endif

callback_client_transmit_SOURCES = callback_client_transmit.cpp
flags_SOURCES = flags.cpp
streaming_client_SOURCES = streaming_client.cpp
receiver_SOURCES = receiver.cpp
