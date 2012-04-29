include $(top_srcdir)/common.mk

noinst_PROGRAMS = tlp_server

tlp_server_SOURCES = tlp-server.cpp
tlp_server_LDADD = $(top_builddir)/common/libolacommon.la \
                   $(top_builddir)/plugins/e131/e131/libolae131core.la
