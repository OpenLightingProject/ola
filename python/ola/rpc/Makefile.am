# ola python client

if BUILD_PYTHON_LIBS

rpcpythondir = $(pkgpythondir)/rpc
nodist_rpcpython_PYTHON = Rpc_pb2.py
rpcpython_PYTHON = SimpleRpcController.py StreamRpcChannel.py __init__.py

BUILT_SOURCES = Rpc_pb2.py

Rpc_pb2.py: ${top_builddir}/common/rpc/Rpc.proto
	protoc --python_out ./ -I ${top_builddir}/common/rpc/ ${top_builddir}/common/rpc/Rpc.proto

clean-local:
	rm -f Rpc_pb2.*

TESTS_ENVIRONMENT = PYTHON=$(PYTHON)
TESTS = SimpleRpcControllerTest.sh

endif

EXTRA_DIST = SimpleRpcControllerTest.py SimpleRpcControllerTest.sh
