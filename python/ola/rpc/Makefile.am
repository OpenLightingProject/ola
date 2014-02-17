# ola python client

# Python modules.
##################################################
rpcpythondir = $(pkgpythondir)/rpc
if BUILD_PYTHON_LIBS
nodist_rpcpython_PYTHON = Rpc_pb2.py
rpcpython_PYTHON = SimpleRpcController.py StreamRpcChannel.py __init__.py
endif

Rpc_pb2.py: ${top_srcdir}/common/rpc/Rpc.proto
	$(PROTOC) --python_out ./ -I ${top_srcdir}/common/rpc/ ${top_srcdir}/common/rpc/Rpc.proto

# TESTS
##################################################
if BUILD_PYTHON_LIBS
TESTS = SimpleRpcControllerTest.sh
endif
dist_check_SCRIPTS = SimpleRpcControllerTest.py

SimpleRpcControllerTest.sh: Makefile
	echo "export PYTHONPATH=${top_builddir}/python; $(PYTHON) ${srcdir}/SimpleRpcControllerTest.py; exit \$$?" > SimpleRpcControllerTest.sh
	chmod +x SimpleRpcControllerTest.sh

CLEANFILES = Rpc_pb2.* SimpleRpcControllerTest.sh
