include $(top_srcdir)/common.mk

# DATA
################################################
dist_piddata_DATA = draft_pids.proto pids.proto manufacturer_pids.proto

# SCRIPTS
################################################
dist_noinst_SCRIPTS = download.sh PidDataTest.py

# TESTS
################################################

if BUILD_TESTS
TESTS = PidDataTester
check_PROGRAMS = PidDataTester

if BUILD_PYTHON_LIBS
TESTS += PidDataTest.sh
endif
endif


PidDataTest.sh: Makefile
	echo "PYTHONPATH=${top_builddir}/python PIDDATA=${srcdir} $(PYTHON) ${srcdir}/PidDataTest.py; exit \$$?" > PidDataTest.sh
	chmod +x PidDataTest.sh

PidDataTester_SOURCES = PidDataTest.cpp
PidDataTester_CXXFLAGS = $(COMMON_TESTING_FLAGS) -DDATADIR=\"$(srcdir)/\"
PidDataTester_LDADD = $(top_builddir)/common/libolacommon.la \
                      $(COMMON_TESTING_LIBS)

CLEANFILES = PidDataTest.sh
