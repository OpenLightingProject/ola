# ola python client

# Python modules.
##################################################
if BUILD_PYTHON_LIBS
built_sources += python/ola/rpc/Rpc_pb2.py
endif

python/ola/rpc/Rpc_pb2.py: common/rpc/Rpc.proto
	mkdir -p  python/ola/rpc
	$(PROTOC) --python_out python/ola/rpc -I ${top_srcdir}/common/rpc/ ${top_srcdir}/common/rpc/Rpc.proto

# TESTS
##################################################
if BUILD_PYTHON_LIBS
test_scripts += python/ola/rpc/SimpleRpcControllerTest.sh
endif

dist_check_SCRIPTS += python/ola/rpc/SimpleRpcControllerTest.py

python/ola/rpc/SimpleRpcControllerTest.sh: python/ola/rpc/Makefile.mk
	mkdir -p python/ola/rpc
	echo "export PYTHONPATH=${top_builddir}/python:${top_srcdir}/python; $(PYTHON) ${srcdir}/python/ola/rpc/SimpleRpcControllerTest.py; exit \$$?" > python/ola/rpc/SimpleRpcControllerTest.sh
	chmod +x python/ola/rpc/SimpleRpcControllerTest.sh

CLEANFILES += python/ola/rpc/SimpleRpcControllerTest.sh \
              python/ola/rpc/*.pyc
