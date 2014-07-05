# This is a non-recursive Makefile, see
# https://github.com/OpenLightingProject/ola/issues/397 for links
#
# Rather than put everything in a single Makefile we use includes to give it a
# bit more structure.
auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                          $(auxdir)/mkinstalldirs

# As suggested by libtoolize
ACLOCAL         = aclocal -I $(auxdir)
ACLOCAL_AMFLAGS = -I config

include aminclude.am

# TODO(simon): just copy the contents of this in here.
include common.mk

# We want to build the python code when running distcheck.
AM_DISTCHECK_CONFIGURE_FLAGS = --enable-python-libs

COMMON_TEST_LDADD = $(COMMON_TESTING_LIBS) \
                    common/libolacommon.la

# Setup pkgconfigdir
pkgconfigdir = $(libdir)/pkgconfig
oladincludedir = $(includedir)/olad

# Define the variables the included files will append to

CLEANFILES =
BUILT_SOURCES =
EXTRA_DIST =
INSTALL_EXEC_HOOKS =
tests =

bin_PROGRAMS =
check_PROGRAMS =
noinst_PROGRAMS =

pkgconfig_DATA =
noinst_DATA =
dist_noinst_DATA =

lib_LTLIBRARIES =
noinst_LTLIBRARIES =

dist_check_SCRIPTS =
dist_noinst_SCRIPTS =

dist_man_MANS =

pkginclude_HEADERS =
noinst_HEADERS =
nodist_pkginclude_HEADERS =

# The includes
# -----------------------------------------------------------------------------

include common/Makefile.mk
include data/Makefile.mk
include debian/Makefile.mk
include examples/Makefile.mk
include include/Makefile.mk
include man/Makefile.mk
include ola/Makefile.mk
include olad/Makefile.mk
include plugins/Makefile.mk
include protoc/Makefile.mk
include python/Makefile.mk
include slp/Makefile.mk
include tools/Makefile.mk

# -----------------------------------------------------------------------------

# TESTS
##################################################
if BUILD_TESTS
TESTS = $(tests)
endif
check_PROGRAMS += $(TESTS)

install-exec-hook: $(INSTALL_EXEC_HOOKS)

# I can't figure out how to safely execute a command (mvn) in a subdirectory,
# so this is recursive for now.
SUBDIRS = java

# pkg-config files.


pkgconfig_DATA += libola.pc libolaserver.pc

# dvi is run from make distcheck. Do nothing.
dvi:

EXTRA_DIST += \
  config/ac_prog_java_cc.m4 \
  config/ac_python_devel.m4 \
  config/ac_saleae.m4 \
  config/ax_python_module.m4 \
  config/cppunit.m4 \
  config/maven.m4 \
  config/ola.m4 \
  config/pkg.m4 \
  config/resolv.m4 \
  config/stl_hash.m4 \
  CONTRIBUTING \
  GPL \
  LGPL \
  LICENCE \
  README.debian \
  README.developer \
  README.rpm ola.spec \
  javascript/README \
  javascript/ola/base_frame.js \
  javascript/ola/common/dmx_constants.js \
  javascript/ola/common/dmx_monitor.js \
  javascript/ola/common/keypad_controller.js \
  javascript/ola/common/keypad_parser.js \
  javascript/ola/common/plugin_list.js \
  javascript/ola/common/rdm_section_list.js \
  javascript/ola/common/section_render.js \
  javascript/ola/common/server.js \
  javascript/ola/common/server_stats.js \
  javascript/ola/common/sorted_list.js \
  javascript/ola/common/uid_list.js \
  javascript/ola/dialog.js \
  javascript/ola/full/available_port_table.js \
  javascript/ola/full/base_universe_tab.js \
  javascript/ola/full/custom_dragger.js \
  javascript/ola/full/custom_dragscrollsupport.js \
  javascript/ola/full/dmx_console.js \
  javascript/ola/full/dmx_console_tab.js \
  javascript/ola/full/dmx_monitor_tab.js \
  javascript/ola/full/plugin_frame.js \
  javascript/ola/full/rdm_attributes_panel.js \
  javascript/ola/full/rdm_patcher.js \
  javascript/ola/full/rdm_patcher_tab.js \
  javascript/ola/full/rdm_tab.js \
  javascript/ola/full/universe_frame.js \
  javascript/ola/full/universe_settings_tab.js \
  javascript/ola/home_frame.js \
  javascript/ola/logger.js \
  javascript/ola/mobile.js \
  javascript/ola/mobile/controller_tab.js \
  javascript/ola/mobile/monitor_tab.js \
  javascript/ola/mobile/plugin_tab.js \
  javascript/ola/mobile/universe_tab.js \
  javascript/ola/new_universe_frame.js \
  javascript/ola/ola.js \
  javascript/ola/port_table.js \
  javascript/ola/universe_control.js \
  javascript/ola/universe_item.js \
  mac_build.sh \
  tests/recvdmx.py \
  tests/regression.py \
  version.sh
