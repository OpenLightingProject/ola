
# python wrappers - experiemental !
#
if BUILD_PYTHON_LIBS

SWIG_SOURCES = LlaClient.i

pkgpython_PYTHON = lla.py __init__.py
pkgpyexec_LTLIBRARIES = _lla.la
_lla_la_SOURCES = LlaClient_wrap.cxx $(SWIG_SOURCES)
#nodist__lla_la_SOURCES = LlaClient_wrap.cxx
_lla_la_CPPFLAGS = $(SWIG_PYTHON_CPPFLAGS)
_lla_la_LDFLAGS = -module
_lla_la_LIBADD = ../lla/liblla.la

LlaClient_wrap.cxx: $(SWIG_SOURCES)
	$(SWIG) -c++ $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src  $(SWIG_SOURCES)

all-local:
	if [ ! -e _lla.so ]; then ln -s .libs/_lla.so _lla.so; fi

endif
