include $(top_srcdir)/common.mk

EXTRA_DIST = Actions.h Context.h VariableInterpolator.h

lib_LTLIBRARIES = libdmxtrigger.la
libdmxtrigger_la_SOURCES = Action.cpp \
                           Context.cpp \
                           VariableInterpolator.cpp
libdmxtrigger_la_LIBADD = $(top_builddir)/common/libolacommon.la

noinst_PROGRAMS = dmx_trigger

dmx_trigger_SOURCES = dmx-trigger.cpp
dmx_trigger_LDADD = $(top_builddir)/common/libolacommon.la \
                    ./libdmxtrigger.la

# Test Programs
TESTS = ActionTester
check_PROGRAMS = $(TESTS)
ActionTester_SOURCES = ActionTester.cpp ActionTest.cpp ContextTest.cpp \
                       IntervalTest.cpp SlotActionsTest.cpp \
                       VariableInterpolatorTest.cpp
ActionTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ActionTester_LDADD = $(CPPUNIT_LIBS) \
                     $(top_builddir)/common/libolacommon.la \
                     ./libdmxtrigger.la
