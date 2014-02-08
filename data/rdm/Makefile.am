# RDM PIDs config
include $(top_srcdir)/common.mk

dist_piddata_DATA = draft_pids.proto pids.proto manufacturer_pids.proto

EXTRA_DIST = download.sh PidDataTest.py

PidDataTest.sh: Makefile
	echo "export PYTHONPATH=${top_builddir}/python; export PIDDATA=${srcdir}; $(PYTHON) ${srcdir}/PidDataTest.py; exit \$$?" > PidDataTest.sh
	chmod +x PidDataTest.sh

CLEANFILES = PidDataTest.sh

if BUILD_TESTS
TESTS = PidDataTester
check_PROGRAMS = PidDataTester

if BUILD_PYTHON_LIBS
TESTS += PidDataTest.sh
endif
endif

PidDataTester_SOURCES = PidDataTest.cpp
PidDataTester_CXXFLAGS = $(COMMON_TESTING_FLAGS) -DDATADIR=\"$(srcdir)/\"
PidDataTester_LDADD = $(COMMON_TESTING_LIBS) \
                      $(top_builddir)/common/libolacommon.la
