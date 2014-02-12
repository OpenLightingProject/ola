# ola python client

if BUILD_PYTHON_LIBS

rpcpythondir = $(pkgpythondir)/rpc
nodist_rpcpython_PYTHON = Rpc_pb2.py
rpcpython_PYTHON = SimpleRpcController.py StreamRpcChannel.py __init__.py

BUILT_SOURCES = Rpc_pb2.py

Rpc_pb2.py: ${top_srcdir}/common/rpc/Rpc.proto
	$(PROTOC) --python_out ./ -I ${top_srcdir}/common/rpc/ ${top_srcdir}/common/rpc/Rpc.proto

SimpleRpcControllerTest.sh: Makefile
	echo "export PYTHONPATH=${top_builddir}/python; $(PYTHON) ${srcdir}/SimpleRpcControllerTest.py; exit \$$?" > SimpleRpcControllerTest.sh
	chmod +x SimpleRpcControllerTest.sh

TESTS = SimpleRpcControllerTest.sh

endif

EXTRA_DIST = SimpleRpcControllerTest.py
CLEANFILES = Rpc_pb2.* SimpleRpcControllerTest.sh
