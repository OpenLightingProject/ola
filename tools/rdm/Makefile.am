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

EXTRA_DIST = $(module_files) \
             model_collector.py \
             ola_rdm_test.py

if INSTALL_RDM_TESTS

rdmtestsdir = $(pkgpythondir)/testing/rdm
rdmtests_PYTHON = $(module_files)

# hack to put the __init__ file in place
rdminitdir = $(pkgpythondir)/testing
rdminit_PYTHON = __init__.py

rdmtestsexecdir = $(bindir)
rdmtestsexec_SCRIPTS = rdm_model_collector.py rdm_responder_test.py

endif
