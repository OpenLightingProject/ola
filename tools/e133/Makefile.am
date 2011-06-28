include $(top_srcdir)/common.mk

EXTRA_DIST = E133Component.h E133Node.h E133Receiver.h \
             E133UniverseController.h SlpConstants.h SlpThread.h SlpUrlParser.h

if HAVE_SLP
noinst_PROGRAMS = e133_controller e133_receiver slp_locate slp_register

e133_receiver_SOURCES = e133-receiver.cpp E133Node.cpp E133Receiver.cpp \
                        SlpThread.cpp
e133_receiver_LDFLAGS = $(UUID_LIBS) $(OSSP_UUID_LIBS) $(PROTOBUF_LIBS) \
                        $(SLP_LIBS)
e133_receiver_LDADD = @LIBS@ \
                      $(top_builddir)/common/libolacommon.la \
                      $(top_builddir)/plugins/dummy/liboladummyresponder.la \
                      $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_controller_SOURCES = e133-controller.cpp E133Node.cpp \
                          E133UniverseController.cpp SlpThread.cpp \
                          SlpUrlParser.cpp
e133_controller_LDFLAGS = $(UUID_LIBS) $(OSSP_UUID_LIBS) $(PROTOBUF_LIBS) \
                          $(SLP_LIBS)
e133_controller_LDADD = @LIBS@ \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/e131/e131/libolae131core.la

slp_locate_SOURCES = slp-locate.cpp SlpThread.cpp
slp_locate_LDFLAGS = $(SLP_LIBS)
slp_locate_LDADD = @LIBS@ \
                   $(top_builddir)/common/libolacommon.la

slp_register_SOURCES = slp-register.cpp SlpThread.cpp
slp_register_LDFLAGS = $(SLP_LIBS)
slp_register_LDADD = @LIBS@ \
                     $(top_builddir)/common/libolacommon.la

# Test Programs
TESTS = SlpTester
check_PROGRAMS = $(TESTS)
SlpTester_SOURCES = SlpTester.cpp SlpUrlParser.cpp SlpUrlParserTest.cpp
SlpTester_CXXFLAGS = $(CPPUNIT_CFLAGS)
SlpTester_LDFLAGS = $(CPPUNIT_LIBS)
SlpTester_LDADD = @LIBS@ $(top_builddir)/common/libolacommon.la

endif
