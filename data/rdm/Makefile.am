# RDM PIDs config
include $(top_srcdir)/common.mk

piddir = $(pid_datadir)
dist_pid_DATA = draft_pids.proto pids.proto manufacturer_pids.proto

EXTRA_DIST = download.sh

if BUILD_TESTS
TESTS = PidDataTester
endif
check_PROGRAMS = $(TESTS)

PidDataTester_SOURCES = PidDataTest.cpp
PidDataTester_CXXFLAGS = $(COMMON_TESTING_FLAGS) -DDATADIR=\"$(srcdir)/\"
PidDataTester_LDADD = $(COMMON_TESTING_LIBS) \
                      $(top_builddir)/common/libolacommon.la
