# DATA
################################################
dist_piddata_DATA = \
    data/rdm/draft_pids.proto \
    data/rdm/pids.proto \
    data/rdm/manufacturer_pids.proto

# SCRIPTS
################################################
dist_noinst_SCRIPTS += \
    data/rdm/download.sh \
    data/rdm/PidDataTest.py

# TESTS
################################################

if BUILD_TESTS
test_programs += data/rdm/PidDataTester

if BUILD_PYTHON_LIBS
test_scripts += data/rdm/PidDataTest.sh
endif
endif

data/rdm/PidDataTest.sh: data/rdm/Makefile.mk
	echo "PYTHONPATH=${top_builddir}/python PIDDATA=${srcdir}/data/rdm $(PYTHON) ${srcdir}/data/rdm/PidDataTest.py; exit \$$?" > data/rdm/PidDataTest.sh
	chmod +x data/rdm/PidDataTest.sh

data_rdm_PidDataTester_SOURCES = data/rdm/PidDataTest.cpp
data_rdm_PidDataTester_CXXFLAGS = $(COMMON_TESTING_FLAGS) -DDATADIR=\"$(srcdir)/data/rdm\"
data_rdm_PidDataTester_LDADD = $(COMMON_TESTING_LIBS)

CLEANFILES += \
    data/rdm/*.pyc \
    data/rdm/PidDataTest.sh \
    data/rdm/__pycache__/*
