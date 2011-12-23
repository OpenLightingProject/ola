include $(top_srcdir)/common.mk

EXTRA_DIST = Actions.h ConfigCommon.h Context.h DMXTrigger.h MockAction.h \
             ParserActions.h VariableInterpolator.h \
             config.lex config.ypp

BUILT_SOURCES = config.tab.cpp config.tab.h lex.yy.cpp

lib_LTLIBRARIES = libdmxtrigger.la
libdmxtrigger_la_SOURCES = Action.cpp \
                           Context.cpp \
                           DMXTrigger.cpp \
                           VariableInterpolator.cpp
libdmxtrigger_la_LIBADD = $(top_builddir)/common/libolacommon.la

noinst_PROGRAMS = dmx_trigger

dmx_trigger_SOURCES = ParserActions.cpp config.tab.cpp dmx-trigger.cpp \
                      lex.yy.cpp
dmx_trigger_LDADD = $(top_builddir)/ola/libola.la \
                    $(top_builddir)/common/libolacommon.la \
                    ./libdmxtrigger.la \
                    $(LEXLIB)

lex.yy.cpp: config.lex
	$(LEX) -o lex.yy.cpp config.lex

config.tab.cpp config.tab.h: config.ypp
	$(YACC) --defines=config.tab.h --output-file=config.tab.cpp config.ypp

clean-local:
	rm -f lex.yy.c config.tab.cpp config.tab.h


# Test Programs
TESTS = ActionTester
check_PROGRAMS = $(TESTS)
ActionTester_SOURCES = ActionTester.cpp ActionTest.cpp ContextTest.cpp \
                       DMXTriggerTest.cpp IntervalTest.cpp \
                       SlotActionsTest.cpp VariableInterpolatorTest.cpp
ActionTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ActionTester_LDADD = $(CPPUNIT_LIBS) \
                     $(top_builddir)/common/libolacommon.la \
                     ./libdmxtrigger.la
