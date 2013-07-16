include $(top_srcdir)/common.mk

if BUILD_EXAMPLES

# The following should match what pkg-config --libs libola returns
LDADD = $(top_builddir)/common/libolacommon.la \
        $(top_builddir)/ola/libola.la

noinst_PROGRAMS =  callback_client_transmit flags receiver streaming_client

callback_client_transmit_SOURCES = callback_client_transmit.cpp
flags_SOURCES = flags.cpp
streaming_client_SOURCES = streaming_client.cpp
receiver_SOURCES = receiver.cpp

endif
