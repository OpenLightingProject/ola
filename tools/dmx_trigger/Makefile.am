include $(top_srcdir)/common.mk

EXTRA_DIST = Actions.h

#noinst_PROGRAMS = dmx_trigger

#dmx_trigger_SOURCES = dmx-trigger.cpp Action.cpp
#dmx_trigger_LDADD = $(top_builddir)/common/libolacommon.la

# Test Programs
TESTS = ActionTester
check_PROGRAMS = $(TESTS)
ActionTester_SOURCES = ActionTester.cpp ActionTest.cpp ContextTest.cpp \
                       IntervalTest.cpp SlotActionsTest.cpp Action.cpp
ActionTester_CXXFLAGS = $(COMMON_CXXFLAGS) $(CPPUNIT_CFLAGS)
ActionTester_LDADD = $(CPPUNIT_LIBS) $(top_builddir)/common/libolacommon.la
