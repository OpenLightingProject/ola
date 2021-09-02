module_files = \
    tools/rdm/DMXSender.py \
    tools/rdm/ExpectedResults.py \
    tools/rdm/ModelCollector.py \
    tools/rdm/ResponderTest.py \
    tools/rdm/TestCategory.py \
    tools/rdm/TestDefinitions.py \
    tools/rdm/TestHelpers.py \
    tools/rdm/TestLogger.py \
    tools/rdm/TestMixins.py \
    tools/rdm/TestRunner.py \
    tools/rdm/TimingStats.py \
    tools/rdm/TestState.py \
    tools/rdm/__init__.py

# These files are installed to the directory from DataLocation.py
testserver_static_files = \
    tools/rdm/static/MIT-LICENSE.txt \
    tools/rdm/static/common.css \
    tools/rdm/static/jquery-1.7.2.min.js \
    tools/rdm/static/jquery-ui-1.8.21.custom.css \
    tools/rdm/static/jquery-ui-1.8.21.custom.min.js \
    tools/rdm/static/rdm_tests.js \
    tools/rdm/static/rdmtests.html \
    tools/rdm/static/ui.multiselect.css \
    tools/rdm/static/ui.multiselect.js

# These files are installed to the images directory under the directory from
# DataLocation.py
testserver_image_files = \
    tools/rdm/static/images/discovery.png \
    tools/rdm/static/images/external.png \
    tools/rdm/static/images/favicon.ico \
    tools/rdm/static/images/loader.gif \
    tools/rdm/static/images/logo.png \
    tools/rdm/static/images/ui-bg_flat_0_aaaaaa_40x100.png \
    tools/rdm/static/images/ui-bg_flat_0_eeeeee_40x100.png \
    tools/rdm/static/images/ui-bg_flat_55_c0402a_40x100.png \
    tools/rdm/static/images/ui-bg_flat_55_eeeeee_40x100.png \
    tools/rdm/static/images/ui-bg_glass_100_f8f8f8_1x400.png \
    tools/rdm/static/images/ui-bg_glass_35_dddddd_1x400.png \
    tools/rdm/static/images/ui-bg_glass_60_eeeeee_1x400.png \
    tools/rdm/static/images/ui-bg_inset-hard_75_999999_1x100.png \
    tools/rdm/static/images/ui-bg_inset-soft_50_c9c9c9_1x100.png \
    tools/rdm/static/images/ui-icons_3383bb_256x240.png \
    tools/rdm/static/images/ui-icons_454545_256x240.png \
    tools/rdm/static/images/ui-icons_70b2e1_256x240.png \
    tools/rdm/static/images/ui-icons_999999_256x240.png \
    tools/rdm/static/images/ui-icons_fbc856_256x240.png

launcher_files = \
    tools/rdm/launch_tests.py \
    tools/rdm/setup_patch.py \
    tools/rdm/skel_config/ola-usbserial.conf

EXTRA_DIST += $(launcher_files)

tools/rdm/ResponderTestTest.sh: tools/rdm/Makefile.mk
	mkdir -p $(top_builddir)/python/ola
	echo "PYTHONPATH=${top_builddir}/python $(PYTHON) ${srcdir}/tools/rdm/ResponderTestTest.py; exit \$$?" > $(top_builddir)/tools/rdm/ResponderTestTest.sh
	chmod +x $(top_builddir)/tools/rdm/ResponderTestTest.sh

tools/rdm/TestStateTest.sh: tools/rdm/Makefile.mk
	mkdir -p $(top_builddir)/python/ola
	echo "PYTHONPATH=${top_builddir}/python $(PYTHON) ${srcdir}/tools/rdm/TestStateTest.py; exit \$$?" > $(top_builddir)/tools/rdm/TestStateTest.sh
	chmod +x $(top_builddir)/tools/rdm/TestStateTest.sh

dist_check_SCRIPTS += \
   tools/rdm/ResponderTestTest.py \
   tools/rdm/TestStateTest.py

if BUILD_PYTHON_LIBS
test_scripts += \
   tools/rdm/ResponderTestTest.sh \
   tools/rdm/TestStateTest.sh
endif

CLEANFILES += \
    tools/rdm/*.pyc \
    tools/rdm/ResponderTestTest.sh \
    tools/rdm/TestStateTest.sh \
    tools/rdm/__pycache__/*

if INSTALL_RDM_TESTS

built_sources += tools/rdm/DataLocation.py

# Create DataLocation.py with the directory of the static files.
tools/rdm/DataLocation.py: tools/rdm/Makefile.mk
	mkdir -p $(top_builddir)/tools/rdm
	echo "location = '${datadir}/ola/rdm-server'" > $(top_builddir)/tools/rdm/DataLocation.py

# RDM Test modules
rdmtestsdir = $(pkgpythondir)/testing/rdm
rdmtests_PYTHON = $(module_files)
nodist_rdmtests_PYTHON = tools/rdm/DataLocation.py

# Hack to put the top level __init__.py file in place
rdminitdir = $(pkgpythondir)/testing
rdminit_PYTHON = tools/rdm/__init__.py

# RDM Test Scripts
rdmtestsexecdir = $(bindir)
dist_rdmtestsexec_SCRIPTS = \
    tools/rdm/rdm_model_collector.py \
    tools/rdm/rdm_responder_test.py \
    tools/rdm/rdm_test_server.py

# Data files for the RDM Test Server
tools_rdm_testserver_staticdir = $(datadir)/ola/rdm-server
dist_tools_rdm_testserver_static_DATA = $(testserver_static_files)

tools_rdm_testserver_imagesdir = $(datadir)/ola/rdm-server/images
dist_tools_rdm_testserver_images_DATA = $(testserver_image_files)

endif
