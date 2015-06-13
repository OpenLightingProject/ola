include python/examples/Makefile.mk
include python/ola/Makefile.mk

install-exec-local:
	pushd python && python setup.py install && popd
