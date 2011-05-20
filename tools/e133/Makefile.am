include $(top_srcdir)/common.mk

bin_PROGRAMS = e133_node e133_controller
e133_node_SOURCES = e133-node.cpp
e133_node_LDFLAGS = $(PROTOBUF_LIBS)
e133_node_LDADD = @LIBS@ \
                  $(top_builddir)/common/libolacommon.la \
                  $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_controller_SOURCES = e133-controller.cpp
e133_controller_LDFLAGS = $(PROTOBUF_LIBS)
e133_controller_LDADD = @LIBS@ \
                  $(top_builddir)/common/libolacommon.la \
                  $(top_builddir)/plugins/e131/e131/libolae131core.la
