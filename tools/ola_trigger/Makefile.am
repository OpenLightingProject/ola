include $(top_srcdir)/common.mk

EXTRA_DIST = Action.h ConfigCommon.h Context.h DMXTrigger.h MockAction.h \
             ParserActions.h ParserGlobals.h VariableInterpolator.h \
             config.lex config.ypp \
             contrib/mac_itunes.conf \
             contrib/mac_volume.conf \
             example.conf

BUILT_SOURCES = config.tab.cpp config.tab.h lex.yy.cpp

lib_LTLIBRARIES = libolatrigger.la
libolatrigger_la_SOURCES = Action.cpp \
                           Context.cpp \
                           DMXTrigger.cpp \
                           VariableInterpolator.cpp
libolatrigger_la_LIBADD = $(top_builddir)/common/libolacommon.la

bin_PROGRAMS = ola_trigger

ola_trigger_SOURCES = ParserActions.cpp config.tab.cpp ola-trigger.cpp \
                      lex.yy.cpp
# required, otherwise we get build errors from flex
ola_trigger_CXXFLAGS = $(WARNING_CXXFLAGS)
ola_trigger_LDADD = $(top_builddir)/ola/libola.la \
                    $(top_builddir)/common/libolacommon.la \
                    ./libolatrigger.la \
                    $(LEXLIB)

lex.yy.cpp: config.lex
	$(LEX) -olex.yy.cpp config.lex

config.tab.cpp config.tab.h: config.ypp
	$(BISON) --defines=config.tab.h --output-file=config.tab.cpp config.ypp

clean-local:
	rm -f lex.yy.cpp config.tab.cpp config.tab.h


# Test Programs
if BUILD_TESTS
TESTS = ActionTester
endif
check_PROGRAMS = $(TESTS)
ActionTester_SOURCES = ActionTester.cpp ActionTest.cpp ContextTest.cpp \
                       DMXTriggerTest.cpp IntervalTest.cpp \
                       SlotTest.cpp VariableInterpolatorTest.cpp
ActionTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ActionTester_LDADD = $(CPPUNIT_LIBS) \
                     $(top_builddir)/common/libolacommon.la \
                     ./libolatrigger.la
