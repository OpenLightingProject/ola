include $(top_srcdir)/common.mk

module_files = DMXSender.py \
               ExpectedResults.py \
               ModelCollector.py \
               ResponderTest.py \
               TestCategory.py \
               TestDefinitions.py \
               TestMixins.py \
               TestRunner.py \
               TestState.py \
               __init__.py

testserver_static_files = \
    static/MIT-LICENSE.txt \
    static/common.css \
    static/jquery-1.7.2.min.js \
    static/jquery-ui-1.8.21.custom.css \
    static/jquery-ui-1.8.21.custom.min.js \
    static/rdm_tests.js \
    static/rdmtests.html \
    static/ui.multiselect.css \
    static/ui.multiselect.js

testserver_image_files = \
    static/images/blank.gif \
    static/images/discovery.png \
    static/images/expander.png \
    static/images/favicon.ico \
    static/images/loader.gif \
    static/images/logo.png \
    static/images/ui-bg_flat_0_aaaaaa_40x100.png \
    static/images/ui-bg_flat_0_eeeeee_40x100.png \
    static/images/ui-bg_flat_55_c0402a_40x100.png \
    static/images/ui-bg_flat_55_eeeeee_40x100.png \
    static/images/ui-bg_glass_100_f8f8f8_1x400.png \
    static/images/ui-bg_glass_35_dddddd_1x400.png \
    static/images/ui-bg_glass_60_eeeeee_1x400.png \
    static/images/ui-bg_inset-hard_75_999999_1x100.png \
    static/images/ui-bg_inset-soft_50_c9c9c9_1x100.png \
    static/images/ui-icons_3383bb_256x240.png \
    static/images/ui-icons_454545_256x240.png \
    static/images/ui-icons_70b2e1_256x240.png \
    static/images/ui-icons_999999_256x240.png \
    static/images/ui-icons_fbc856_256x240.png

launcher_files = \
  launch_tests.py \
  setup_patch.py \
  skel_config/ola-usbserial.conf

EXTRA_DIST = $(launcher_files)

if INSTALL_RDM_TESTS

BUILT_SOURCES = DataLocation.py
datalocationdir = $(pkgpythondir)/testing/rdm
nodist_datalocation_PYTHON = DataLocation.py

# This is a hack, but I can't find any other way to do it
DataLocation.py:
	echo "location = '${datadir}/ola/rdm-server'" > DataLocation.py

# RDM Test modules
rdmtestsdir = $(pkgpythondir)/testing/rdm
rdmtests_PYTHON = $(module_files)

# hack to put the __init__ file in place
rdminitdir = $(pkgpythondir)/testing
rdminit_PYTHON = __init__.py

# RDM Test Scripts
rdmtestsexecdir = $(bindir)
dist_rdmtestsexec_SCRIPTS = rdm_model_collector.py \
                            rdm_responder_test.py \
                            rdm_test_server.py

# Data files for the RDM Test Server
testserver_staticdir = $(datadir)/ola/rdm-server
dist_testserver_static_DATA = $(testserver_static_files)

testserver_imagesdir = $(datadir)/ola/rdm-server/images
dist_testserver_images_DATA = $(testserver_image_files)

clean-local:
	rm -f DataLocation.py


endif
