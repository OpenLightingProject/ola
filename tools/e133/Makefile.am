include $(top_srcdir)/common.mk

EXTRA_DIST = E133Component.h E133Node.h E133Receiver.h \
             E133UniverseController.h SlpConstants.h SlpThread.h SlpUrlParser.h

if HAVE_SLP
noinst_PROGRAMS = e133_controller e133_receiver slp_locate slp_register

e133_receiver_SOURCES = e133-receiver.cpp E133Node.cpp E133Receiver.cpp \
                        SlpThread.cpp
e133_receiver_LDADD = $(UUID_LIBS) $(OSSP_UUID_LIBS) \
                      $(libprotobuf_LIBS) $(openslp_LIBS) \
                      $(top_builddir)/common/libolacommon.la \
                      $(top_builddir)/plugins/dummy/liboladummyresponder.la \
                      $(top_builddir)/plugins/e131/e131/libolae131core.la

e133_controller_SOURCES = e133-controller.cpp E133Node.cpp \
                          E133UniverseController.cpp SlpThread.cpp \
                          SlpUrlParser.cpp
# required for PID_DATA_FILE
e133_controller_CXXFLAGS = @OLAD_DEFINES@ $(COMMON_CXXFLAGS)
e133_controller_LDADD = $(UUID_LIBS) $(OSSP_UUID_LIBS) \
                        $(libprotobuf_LIBS) $(openslp_LIBS) \
                        $(top_builddir)/common/libolacommon.la \
                        $(top_builddir)/plugins/e131/e131/libolae131core.la

slp_locate_SOURCES = slp-locate.cpp SlpThread.cpp
slp_locate_LDADD = $(openslp_LIBS) \
                   $(top_builddir)/common/libolacommon.la

slp_register_SOURCES = slp-register.cpp SlpThread.cpp
slp_register_LDADD = $(openslp_LIBS) \
                     $(top_builddir)/common/libolacommon.la

# Test Programs
TESTS = SlpTester
check_PROGRAMS = $(TESTS)
SlpTester_SOURCES = SlpTester.cpp SlpUrlParser.cpp SlpUrlParserTest.cpp
SlpTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
SlpTester_LDADD = $(CPPUNIT_LIBS) $(top_builddir)/common/libolacommon.la

endif
