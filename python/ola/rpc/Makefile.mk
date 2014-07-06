# ola python client

# Python modules.
##################################################
rpcpythondir = $(pkgpythondir)/rpc
if BUILD_PYTHON_LIBS
nodist_rpcpython_PYTHON = python/ola/rpc/Rpc_pb2.py
rpcpython_PYTHON = python/ola/rpc/SimpleRpcController.py \
                   python/ola/rpc/StreamRpcChannel.py \
                   python/ola/rpc/__init__.py
BUILT_SOURCES += python/ola/rpc/Rpc_pb2.py
endif

python/ola/rpc/Rpc_pb2.py: common/rpc/Rpc.proto
	mkdir -p  python/ola/rpc
	$(PROTOC) --python_out python/ola/rpc -I ${top_srcdir}/common/rpc/ ${top_srcdir}/common/rpc/Rpc.proto

# TESTS
##################################################
test_scripts += python/ola/rpc/SimpleRpcControllerTest.sh
dist_check_SCRIPTS += python/ola/rpc/SimpleRpcControllerTest.py

python/ola/rpc/SimpleRpcControllerTest.sh: python/ola/rpc/Makefile.mk
	echo "export PYTHONPATH=${top_builddir}/python; $(PYTHON) ${srcdir}/python/ola/rpc/SimpleRpcControllerTest.py; exit \$$?" > python/ola/rpc/SimpleRpcControllerTest.sh
	chmod +x python/ola/rpc/SimpleRpcControllerTest.sh

CLEANFILES += python/ola/rpc/Rpc_pb2.* \
              python/ola/rpc/SimpleRpcControllerTest.sh \
              python/ola/rpc/*.pyc
