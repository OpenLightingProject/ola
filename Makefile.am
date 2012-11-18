auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                          $(auxdir)/mkinstalldirs

SUBDIRS = include data common ola plugins olad python tools examples debian \
          java

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libola.pc libolaserver.pc

ACLOCAL         = aclocal -I $(auxdir)
ACLOCAL_AMFLAGS = -I config

EXTRA_DIST = \
  config/ac_pkg_swig.m4 \
  config/ac_python_devel.m4 \
  config/swig_enable_cxx.m4 \
  config/swig_python.m4 \
  GPL \
  LGPL \
  LICENCE \
  README.debian \
  README.developer \
  README.rpm ola.spec \
  javascript/README \
  javascript/ola/base_frame.js \
  javascript/ola/common/dmx_monitor.js \
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
  javascript/ola/mobile/keypad_controller.js \
  javascript/ola/mobile/keypad_parser.js \
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
  tests/regression.py
