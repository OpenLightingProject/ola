include $(top_srcdir)/common.mk

EXTRA_DIST = E133Component.h E133Node.h E133Receiver.h \
             E133UniverseController.h SlpThread.h

bin_PROGRAMS = e133_controller e133_receiver

e133_receiver_SOURCES = e133-receiver.cpp E133Node.cpp E133Receiver.cpp
e133_receiver_LDFLAGS = $(PROTOBUF_LIBS)
e133_receiver_LDADD = @LIBS@ \
                      $(top_builddir)/common/libolacommon.la \
                      $(top_builddir)/plugins/dummy/liboladummyresponder.la \
                      $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_controller_SOURCES = e133-controller.cpp E133Node.cpp \
                          E133UniverseController.cpp
e133_controller_LDFLAGS = $(PROTOBUF_LIBS)
e133_controller_LDADD = @LIBS@ \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/e131/e131/libolae131core.la

if HAVE_SLP
bin_PROGRAMS += slp_locate slp_register

slp_locate_SOURCES = slp-locate.cpp SlpThread.cpp
slp_locate_LDFLAGS = $(SLP_LIBS)
slp_locate_LDADD = @LIBS@ \
                   $(top_builddir)/common/libolacommon.la

slp_register_SOURCES = slp-register.cpp SlpThread.cpp
slp_register_LDFLAGS = $(SLP_LIBS)
slp_register_LDADD = @LIBS@ \
                     $(top_builddir)/common/libolacommon.la
endif
