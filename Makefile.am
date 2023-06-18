# This is a non-recursive Makefile, see
# https://github.com/OpenLightingProject/ola/issues/397 for links to useful
# documentation.
#
# Rather than defining everything in a single Makefile we include a Makefile.mk
# from each directory. This gives the build rules a bit more structure.
auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                          $(auxdir)/mkinstalldirs

# As suggested by libtoolize
ACLOCAL         = aclocal -I $(auxdir)
ACLOCAL_AMFLAGS = -I config

include aminclude.am

# When running distcheck we want to enable the Python modules.
AM_DISTCHECK_CONFIGURE_FLAGS = --enable-python-libs

# C++ FLAGS variables
# ------------------------------------------------------------------------------
# COMMON_CXXFLAGS
# The common C++ flags. This may include -Werror if fatal-warning was enabled
# in ./configure. AM_CXXFLAGS defaults to this, and it should be preferred.
#
# COMMON_PROTOBUF_CXXFLAGS
# The common C++ flags with some warnings protobuf code throws disabled. Use
# this for generated protobuf code that does not build correctly with
# COMMON_CXXFLAGS.
#
# COMMON_CXXFLAGS_ONLY_WARNINGS
# The common C++ flags with (non-fatal) warnings enabled. Use this for 3rd
# party code that does not build correctly with COMMON_CXXFLAGS.
#
# COMMON_TESTING_FLAGS
# The common C++ flags for testing.
#
# COMMON_TESTING_FLAGS_ONLY_WARNINGS
# The common C++ flags for testing with (non-fatal) warnings enabled. Use this
# for 3rd party code that does not build correctly with COMMON_TESTING_FLAGS.
#
# COMMON_TESTING_PROTOBUF_FLAGS
# The common C++ flags for testing with some warnings protobuf code throws
# disabled.
COMMON_CXXFLAGS_ONLY_WARNINGS = \
    -I$(top_srcdir)/include \
    -I$(top_builddir)/include \
    -Wall -Wformat -W -fvisibility-inlines-hidden \
    $(libprotobuf_CFLAGS)

COMMON_CXXFLAGS = $(COMMON_CXXFLAGS_ONLY_WARNINGS)
COMMON_PROTOBUF_CXXFLAGS = $(COMMON_CXXFLAGS)

COMMON_TESTING_FLAGS_ONLY_WARNINGS = $(COMMON_CXXFLAGS_ONLY_WARNINGS) \
                                     $(CPPUNIT_CFLAGS) \
                                     -DTEST_SRC_DIR=\"$(srcdir)\" \
                                     -DTEST_BUILD_DIR=\"$(builddir)\"

COMMON_TESTING_FLAGS = $(COMMON_TESTING_FLAGS_ONLY_WARNINGS)
COMMON_TESTING_PROTOBUF_FLAGS = $(COMMON_TESTING_FLAGS)

# The generated protobuf files don't compile with -Werror on win32 so we
# disable fatal warnings on WIN32.
if ! USING_WIN32
if FATAL_WARNINGS
  COMMON_CXXFLAGS += -Werror
  COMMON_PROTOBUF_CXXFLAGS += -Werror -Wno-error=unused-parameter \
                              -Wno-error=deprecated-declarations \
                              -Wno-error=sign-compare
  COMMON_TESTING_FLAGS += -Werror
  COMMON_TESTING_PROTOBUF_FLAGS += -Werror -Wno-error=unused-parameter \
                                   -Wno-error=deprecated-declarations

if GNU_PLUS_PLUS_11_DEPRECATIONS
    # We have to use gnu++11 for some reason, so stop it complaining about
    # auto_ptr
    COMMON_CXXFLAGS += -Wno-error=deprecated-declarations
    COMMON_TESTING_FLAGS += -Wno-error=deprecated-declarations
endif
endif
endif

# AM_CXXFLAGS is used when target_CXXFLAGS isn't specified.
AM_CXXFLAGS = $(COMMON_CXXFLAGS)

# LIBS variables
# ------------------------------------------------------------------------------
#
# COMMON_TESTING_LIBS
# The set of libraries used in the unittests.
COMMON_TESTING_LIBS = $(CPPUNIT_LIBS) \
                      common/testing/libolatesting.la \
                      common/testing/libtestmain.la \
                      common/libolacommon.la

# Due to MinGW's handling of library archives, we need to append this.
if USING_WIN32
COMMON_TESTING_LIBS += $(CPPUNIT_LIBS)
endif

# Setup pkgconfigdir, the path where .pc files are installed.
pkgconfigdir = $(libdir)/pkgconfig
oladincludedir = $(includedir)/olad

# Define a custom runner for .py tests
TESTS_ENVIRONMENT = PYTHONPATH=${top_srcdir}/python
TEST_EXTENSIONS = .py
PY_LOG_COMPILER = $(PYTHON)

# Variables used by the included Makefile.mk(s)
# ------------------------------------------------------------------------------

# Common automake variables.
CLEANFILES =
BUILT_SOURCES =
EXTRA_DIST =

bin_PROGRAMS =
check_PROGRAMS =
noinst_PROGRAMS =

pkgconfig_DATA =
noinst_DATA =
dist_noinst_DATA =

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

check_SCRIPTS =
dist_check_SCRIPTS =
dist_noinst_SCRIPTS =

dist_man_MANS =

pkginclude_HEADERS =
noinst_HEADERS =
nodist_pkginclude_HEADERS =

# Append to this to define an install-exec-hook.
INSTALL_EXEC_HOOKS =

# Test programs, these are added to check_PROGRAMS and TESTS if BUILD_TESTS is
# true.
test_programs =

# Files in built_sources are included in BUILT_SOURCES and CLEANFILES
built_sources =

# Test scripts are run if BUILD_TESTS is true.
test_scripts =

# directories with python code that should be test compiled during the build
PYTHON_BUILD_DIRS =

# The includes
# -----------------------------------------------------------------------------

# Since `make install` relinks the libraries, this should be in the order in
# which libraries depend on each other. e.g. common, plugin_api, plugins, server.
include common/Makefile.mk
include data/Makefile.mk
include debian/Makefile.mk
include doxygen/Makefile.mk
include examples/Makefile.mk
include include/Makefile.mk
include javascript/Makefile.mk
include man/Makefile.mk
include libs/Makefile.mk
include ola/Makefile.mk
# olad/plugin_api/Makefile.mk is included directly due to the order of
# dependencies between them; we need to build olad/plugin_api, then the
# plugins, then olad
include olad/plugin_api/Makefile.mk
include plugins/Makefile.mk
include olad/Makefile.mk
include protoc/Makefile.mk
include python/Makefile.mk
include scripts/Makefile.mk
include tools/Makefile.mk

# -----------------------------------------------------------------------------

# Now some post-processing
BUILT_SOURCES += $(built_sources)
CLEANFILES += $(built_sources)

if BUILD_TESTS
TESTS = $(test_programs) $(test_scripts)
endif
check_PROGRAMS += $(test_programs)

install-exec-hook: $(INSTALL_EXEC_HOOKS)

# -----------------------------------------------------------------------------

# This gives us a rather hacky method to build the files normally built during
# the build, so they are present for Doxygen to run against
builtfiles : Makefile.am $(built_sources)
.PHONY : builtfiles

# I can't figure out how to safely execute a command (mvn) in a subdirectory,
# so this is recursive for now.
SUBDIRS = java

pkgconfig_DATA += libola.pc libolaserver.pc

# dvi is run from make distcheck. Do nothing.
dvi:

EXTRA_DIST += \
  CONTRIBUTING \
  Doxyfile \
  GPL \
  LGPL \
  LICENCE \
  README.debian \
  README.developer \
  README.mingw32 \
  README.rpm \
  config/ac_prog_java_cc.m4 \
  config/ac_python_devel.m4 \
  config/ac_saleae.m4 \
  config/ax_python_module.m4 \
  config/maven.m4 \
  config/ola.m4 \
  config/ola_version.m4 \
  config/pkg.m4 \
  config/resolv.m4 \
  config/stl_hash.m4 \
  mac_build.sh \
  ola.spec

# -----------------------------------------------------------------------------

# Create a linter target
.PHONY : lint
lint: flake8 cpplint

# flake8 linter
.PHONY : flake8
flake8: Makefile.am
if FOUND_FLAKE8
	flake8 --statistics --count
else
	$(error flake8 not found. Install flake8 and re-run configure.)
endif

# cpplint linter
CPP_LINT_FILTER = "-legal/copyright,-readability/streams,-runtime/arrays"
CPP_LINT_FILES = $(shell find . \( -name "*.h" -or -name "*.cpp" \) -and ! \( \
        -wholename "./common/protocol/Ola.pb.*" -or \
        -wholename "./common/rpc/Rpc.pb.*" -or \
        -wholename "./common/rpc/TestService.pb.*" -or \
        -wholename "./common/rdm/Pids.pb.*" -or \
        -wholename "./config.h" -or \
        -wholename "./plugins/*/messages/*ConfigMessages.pb.*" -or \
        -wholename "./tools/ola_trigger/config.tab.*" -or \
        -wholename "./tools/ola_trigger/lex.yy.cpp" \) | xargs)
.PHONY : cpplint
cpplint: Makefile.am
if FOUND_CPPLINT
	cpplint --filter=$(CPP_LINT_FILTER) $(CPP_LINT_FILES)
else
	$(error cpplint not found. Install the forked cpplint (e.g. via pip for the latest version) and re-run configure.)
endif

# coverage
COVERAGE_OUTPUTS = --txt coverage/coverage.txt \
	--csv coverage/coverage.csv \
	--json coverage/coverage.json \
	--cobertura coverage/coverage.cobertura.xml \
	--html-details coverage/details.html/coverage.details.html \
	--coveralls coverage/coverage.coveralls.json
COVERAGE_GCOV_EXE=--gcov-executable /usr/bin/gcov
COVERAGE_FILTERS=-e '.*Test\.cpp$$' \
	-e '.*\.pb\.cc$$' \
	-e '.*\.pb\.cpp$$' \
	-e '.*\.pb\.h$$' \
	-e '.*\.yy\.cpp$$' \
	-e '.*\.tab\.cpp$$' \
	-e '.*\.tab\.h$$' \
	-e '.*/doxygen/examples.*$$'
.PHONY : coverage
coverage: Makefile.am check
if !BUILD_GCOV
	$(error Generating coverage requires configuring with --enable-gcov)
else
if FOUND_GCOVR
	mkdir -p coverage/details.html/
	gcovr --print-summary $(COVERAGE_OUTPUTS) $(COVERAGE_GCOV_EXE) --root . $(COVERAGE_FILTERS)
else
	$(error gcovr not found. Install gcovr (e.g. via pip for the latest version) and re-run configure.)
endif
endif
