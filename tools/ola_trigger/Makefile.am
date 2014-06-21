include $(top_srcdir)/common.mk

EXTRA_DIST = config.lex config.ypp

dist_noinst_DATA = contrib/mac_itunes.conf \
                   contrib/mac_volume.conf \
                   example.conf

# LIBRARIES
##################################################
lib_LTLIBRARIES = libolatrigger.la
libolatrigger_la_SOURCES = Action.cpp Action.h \
                           Context.cpp Context.h \
                           DMXTrigger.cpp DMXTrigger.h \
                           VariableInterpolator.h \
                           VariableInterpolator.cpp
libolatrigger_la_LIBADD = ../../common/libolacommon.la

# PROGRAMS
##################################################
bin_PROGRAMS = ola_trigger

ola_trigger_SOURCES = ConfigCommon.h ParserActions.cpp ParserActions.h \
                      ParserGlobals.h config.tab.cpp ola-trigger.cpp \
                      lex.yy.cpp
# required, otherwise we get build errors from the flex output
ola_trigger_CXXFLAGS = $(WARNING_CXXFLAGS)
ola_trigger_LDADD = ../../common/libolacommon.la \
                    ../../ola/libola.la \
                    ./libolatrigger.la \
                    $(LEXLIB)

if USING_WIN32
ola_trigger_LDADD += -lWs2_32
endif

BUILT_SOURCES = lex.yy.cpp config.tab.cpp config.tab.h

lex.yy.cpp: Makefile.am config.lex
	$(LEX) -olex.yy.cpp $(srcdir)/config.lex

config.tab.cpp config.tab.h: Makefile.am config.ypp
	$(BISON) --defines=config.tab.h --output-file=config.tab.cpp $(srcdir)/config.ypp

CLEANFILES = lex.yy.cpp config.tab.cpp config.tab.h

# TESTS
##################################################
if BUILD_TESTS
TESTS = ActionTester
endif
check_PROGRAMS = $(TESTS)
ActionTester_SOURCES = ActionTest.cpp ContextTest.cpp \
                       DMXTriggerTest.cpp IntervalTest.cpp \
                       MockAction.h SlotTest.cpp VariableInterpolatorTest.cpp
ActionTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ActionTester_LDADD = $(COMMON_TESTING_LIBS) \
                     ../../common/libolacommon.la \
                     ./libolatrigger.la
