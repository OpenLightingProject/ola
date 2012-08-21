include $(top_srcdir)/common.mk


module_files = DMXSender.py \
               ExpectedResults.py \
               ResponderTest.py \
               TestCategory.py \
               TestDefinitions.py \
               TestMixins.py \
               TestRunner.py \
               TestState.py \
               __init__.py

test_server_files = \
    TestServer.py \
    static/images/blank.gif \
    static/images/discovery.png \
    static/images/expander.png \
    static/images/loader.gif \
    static/images/logo.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-bg_flat_0_aaaaaa_40x100.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-bg_flat_0_eeeeee_40x100.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-bg_flat_55_c0402a_40x100.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-bg_flat_55_eeeeee_40x100.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-bg_glass_100_f8f8f8_1x400.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-bg_glass_35_dddddd_1x400.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-bg_glass_60_eeeeee_1x400.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-bg_inset-hard_75_999999_1x100.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-bg_inset-soft_50_c9c9c9_1x100.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-icons_3383bb_256x240.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-icons_454545_256x240.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-icons_70b2e1_256x240.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-icons_999999_256x240.png \
    static/javascript/jQuery-UI/css/overcast/images/ui-icons_fbc856_256x240.png \
    static/javascript/jQuery-UI/css/overcast/jquery-ui-1.8.21.custom.css \
    static/javascript/jQuery-UI/js/jquery-1.7.2.min.js \
    static/javascript/jQuery-UI/js/jquery-ui-1.8.21.custom.min.js \
    static/javascript/jQuery-multiselect/MIT-LICENSE.txt \
    static/javascript/jQuery-multiselect/css/common.css \
    static/javascript/jQuery-multiselect/css/ui.multiselect.css \
    static/javascript/jQuery-multiselect/js/ui.multiselect.js \
    static/javascript/jquery.min.js \
    static/javascript/rdm_tests.js \
    static/rdmtests.html

launcher_files = \
  launch_tests.py \
  setup_patch.py \
  skel_config/ola-usbserial.conf

EXTRA_DIST = $(module_files) \
             $(test_server_files) \
             $(launcher_files) \
             rdm_model_collector.py \
             rdm_responder_test.py

if INSTALL_RDM_TESTS

rdmtestsdir = $(pkgpythondir)/testing/rdm
rdmtests_PYTHON = $(module_files)

# hack to put the __init__ file in place
rdminitdir = $(pkgpythondir)/testing
rdminit_PYTHON = __init__.py

rdmtestsexecdir = $(bindir)
rdmtestsexec_SCRIPTS = rdm_model_collector.py rdm_responder_test.py

endif
