# ola python client

# Python modules.
##################################################
if BUILD_PYTHON_LIBS
rpcpythondir = $(pkgpythondir)/rpc
nodist_rpcpython_PYTHON = python/ola/rpc/Rpc_pb2.py
rpcpython_PYTHON = python/ola/rpc/SimpleRpcController.py \
                   python/ola/rpc/StreamRpcChannel.py \
                   python/ola/rpc/__init__.py
built_sources += python/ola/rpc/Rpc_pb2.py
endif

python/ola/rpc/Rpc_pb2.py: common/rpc/Rpc.proto
	mkdir -p $(top_builddir)/python/ola/rpc
	$(PROTOC) --python_out $(top_builddir)/python/ola/rpc -I ${top_srcdir}/common/rpc/ ${top_srcdir}/common/rpc/Rpc.proto

# TESTS
##################################################
if BUILD_PYTHON_LIBS
test_scripts += python/ola/rpc/SimpleRpcControllerTest.sh
endif

dist_check_SCRIPTS += python/ola/rpc/SimpleRpcControllerTest.py

python/ola/rpc/SimpleRpcControllerTest.sh: python/ola/rpc/Makefile.mk
	mkdir -p $(top_builddir)/python/ola/rpc
	echo "export PYTHONPATH=${top_builddir}/python:${top_srcdir}/python; $(PYTHON) ${srcdir}/python/ola/rpc/SimpleRpcControllerTest.py; exit \$$?" > $(top_builddir)/python/ola/rpc/SimpleRpcControllerTest.sh
	chmod +x $(top_builddir)/python/ola/rpc/SimpleRpcControllerTest.sh

CLEANFILES += python/ola/rpc/SimpleRpcControllerTest.sh \
              python/ola/rpc/*.pyc
