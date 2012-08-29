include $(top_srcdir)/common.mk

EXTRA_DIST = Base.h \
             SLP.proto \
             SLPClient.h \
             SLPClientCore.h \
             SLPServer.h \
             SLPServiceImpl.h

noinst_PROGRAMS = slp_server slp_client

slp_server_SOURCES = SLPServiceImpl.cpp \
                     SLPServer.cpp \
                     slp-server.cpp
slp_server_LDADD = $(top_builddir)/common/libolacommon.la \
                   $(top_builddir)/common/http/libolahttp.la \
                   ./libolaslpproto.la

slp_client_SOURCES = slp-client.cpp \
                     SLPClient.cpp \
                     SLPClientCore.cpp
slp_client_LDADD = $(top_builddir)/common/libolacommon.la \
                   $(top_builddir)/ola/libola.la \
                   ./libolaslpproto.la

noinst_LTLIBRARIES = libolaslpproto.la
nodist_libolaslpproto_la_SOURCES = SLP.pb.cc
libolaslpproto_la_LIBADD = $(libprotobuf_LIBS)
# required, otherwise we get build errors
libolaslpproto_la_CXXFLAGS = $(WARNING_CXXFLAGS)

BUILT_SOURCES = SLP.pb.cc SLP.pb.h

SLP.pb.cc SLP.pb.h: SLP.proto
	$(PROTOC)  --cpp_out ./ SLP.proto

clean-local:
	rm -f *.pb.{h,cc}
