# OLA Python client

include python/ola/rpc/Makefile.mk

# Python modules.
##################################################
artnet_path = ${top_srcdir}/plugins/artnet/messages
artnet_proto = $(artnet_path)/ArtNetConfigMessages.proto
ola_path = ${top_srcdir}/common/protocol
ola_proto = $(ola_path)/Ola.proto
pids_path = ${top_srcdir}/common/rdm
pids_proto = $(pids_path)/Pids.proto
usbpro_path = ${top_srcdir}/plugins/usbpro/messages
usbpro_proto = $(usbpro_path)/UsbProConfigMessages.proto

if BUILD_PYTHON_LIBS
output_files = \
    python/ola/ArtNetConfigMessages_pb2.py \
    python/ola/Ola_pb2.py \
    python/ola/UsbProConfigMessages_pb2.py \
    python/ola/Pids_pb2.py \
    python/ola/PidStoreLocation.py \
    python/ola/Version.py
built_sources += $(output_files)

nodist_pkgpython_PYTHON = $(output_files)
pkgpython_PYTHON = \
    python/ola/ClientWrapper.py \
    python/ola/DMXConstants.py \
    python/ola/DUBDecoder.py \
    python/ola/MACAddress.py \
    python/ola/OlaClient.py \
    python/ola/RDMAPI.py \
    python/ola/RDMConstants.py \
    python/ola/PidStore.py \
    python/ola/UID.py \
    python/ola/__init__.py
endif

python/ola/ArtNetConfigMessages_pb2.py: $(artnet_proto)
	$(PROTOC) --python_out $(top_builddir)/python/ola/ -I $(artnet_path) $(artnet_proto)

python/ola/Ola_pb2.py: $(ola_proto)
	$(PROTOC) --python_out $(top_builddir)/python/ola/ -I $(ola_path) $(ola_proto)

python/ola/Pids_pb2.py: $(pids_proto)
	$(PROTOC) --python_out $(top_builddir)/python/ola/ -I $(pids_path) $(pids_proto)

python/ola/UsbProConfigMessages_pb2.py: $(usbpro_proto)
	$(PROTOC) --python_out $(top_builddir)/python/ola/ -I $(usbpro_path) $(usbpro_proto)

python/ola/PidStoreLocation.py: python/ola/Makefile.mk configure.ac
	echo "location = '${piddatadir}'" > $(top_builddir)/python/ola/PidStoreLocation.py

python/ola/Version.py: python/ola/Makefile.mk configure.ac config/ola_version.m4
	echo "version = '${VERSION}'" > $(top_builddir)/python/ola/Version.py

# TESTS
##################################################

dist_check_SCRIPTS += \
    python/ola/DUBDecoderTest.py \
    python/ola/MACAddressTest.py \
    python/ola/UIDTest.py

if BUILD_PYTHON_LIBS
test_scripts += \
    python/ola/DUBDecoderTest.py \
    python/ola/MACAddressTest.py \
    python/ola/UIDTest.py
endif

CLEANFILES += python/ola/*.pyc
