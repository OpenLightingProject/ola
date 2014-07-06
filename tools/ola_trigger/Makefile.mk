EXTRA_DIST += \
    tools/ola_trigger/config.lex \
    tools/ola_trigger/config.ypp

dist_noinst_DATA += \
    tools/ola_trigger/contrib/mac_itunes.conf \
    tools/ola_trigger/contrib/mac_volume.conf \
    tools/ola_trigger/example.conf

# LIBRARIES
##################################################
lib_LTLIBRARIES += tools/ola_trigger/libolatrigger.la
tools_ola_trigger_libolatrigger_la_SOURCES = \
    tools/ola_trigger/Action.cpp \
    tools/ola_trigger/Action.h \
    tools/ola_trigger/Context.cpp \
    tools/ola_trigger/Context.h \
    tools/ola_trigger/DMXTrigger.cpp \
    tools/ola_trigger/DMXTrigger.h \
    tools/ola_trigger/VariableInterpolator.h \
    tools/ola_trigger/VariableInterpolator.cpp
tools_ola_trigger_libolatrigger_la_LIBADD = common/libolacommon.la

# PROGRAMS
##################################################
bin_PROGRAMS += tools/ola_trigger/ola_trigger

tools_ola_trigger_ola_trigger_SOURCES = \
    tools/ola_trigger/ConfigCommon.h \
    tools/ola_trigger/ParserActions.cpp \
    tools/ola_trigger/ParserActions.h \
    tools/ola_trigger/ParserGlobals.h \
    tools/ola_trigger/config.tab.cpp \
    tools/ola_trigger/lex.yy.cpp \
    tools/ola_trigger/ola-trigger.cpp
# required, otherwise we get build errors from the flex output
tools_ola_trigger_ola_trigger_CXXFLAGS = $(COMMON_CXXFLAGS_ONLY_WARNINGS)
tools_ola_trigger_ola_trigger_LDADD = common/libolacommon.la \
                                      ola/libola.la \
                                      tools/ola_trigger/libolatrigger.la \
                                      $(LEXLIB)

BUILT_SOURCES += \
    tools/ola_trigger/lex.yy.cpp \
    tools/ola_trigger/config.tab.cpp \
    tools/ola_trigger/config.tab.h

tools/ola_trigger/lex.yy.cpp: tools/ola_trigger/Makefile.mk tools/ola_trigger/config.lex
	$(LEX) -otools/ola_trigger/lex.yy.cpp $(srcdir)/tools/ola_trigger/config.lex

tools/ola_trigger/config.tab.cpp tools/ola_trigger/config.tab.h: tools/ola_trigger/Makefile.mk tools/ola_trigger/config.ypp
	$(BISON) --defines=tools/ola_trigger/config.tab.h --output-file=tools/ola_trigger/config.tab.cpp $(srcdir)/tools/ola_trigger/config.ypp

CLEANFILES += \
    tools/ola_trigger/lex.yy.cpp \
    tools/ola_trigger/config.tab.cpp \
    tools/ola_trigger/config.tab.h

# TESTS
##################################################
test_programs += tools/ola_trigger/ActionTester

tools_ola_trigger_ActionTester_SOURCES = \
    tools/ola_trigger/ActionTest.cpp \
    tools/ola_trigger/ContextTest.cpp \
    tools/ola_trigger/DMXTriggerTest.cpp \
    tools/ola_trigger/IntervalTest.cpp \
    tools/ola_trigger/MockAction.h \
    tools/ola_trigger/SlotTest.cpp \
    tools/ola_trigger/VariableInterpolatorTest.cpp
tools_ola_trigger_ActionTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
tools_ola_trigger_ActionTester_LDADD = $(COMMON_TESTING_LIBS) \
                                       tools/ola_trigger/libolatrigger.la
