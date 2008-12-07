

noinst_LTLIBRARIES = libllacommon.la
libllacommon_la_SOURCES =
libllacommon_la_LIBADD = protocol/libllaproto.la \
						 rpc/libstreamrpcchannel.la \
						 select_server/libselectserver.la

nodist_EXTRA_libllacommon_la_SOURCES = dummy.cpp


SUBDIRS = protocol select_server rpc
