include tools/ja-rule/Makefile.mk
include tools/logic/Makefile.mk
include tools/ola_mon/Makefile.mk
include tools/ola_trigger/Makefile.mk
include tools/rdm/Makefile.mk

if !USING_WIN32
include tools/e133/Makefile.mk
include tools/usbpro/Makefile.mk
include tools/rdmpro/Makefile.mk
endif

dist_noinst_DATA += \
    tools/ola_mon/index.html \
    tools/ola_mon/ola_mon.conf \
    tools/ola_mon/ola_mon.py

PYTHON_BUILD_DIRS += tools
