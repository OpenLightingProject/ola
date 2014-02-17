# OLA Python client

SUBDIRS = rpc

# Python modules.
##################################################
artnet_path = ${top_srcdir}/plugins/artnet/messages
artnet_proto = $(artnet_path)/ArtnetConfigMessages.proto
ola_path = ${top_srcdir}/common/protocol
ola_proto = $(ola_path)/Ola.proto
pids_path = ${top_srcdir}/common/rdm
pids_proto = $(pids_path)/Pids.proto
usbpro_path = ${top_srcdir}/plugins/usbpro/messages
usbpro_proto = $(usbpro_path)/UsbProConfigMessages.proto

if BUILD_PYTHON_LIBS
nodist_pkgpython_PYTHON = ArtnetConfigMessages_pb2.py Ola_pb2.py \
                          UsbProConfigMessages_pb2.py Pids_pb2.py \
                          PidStoreLocation.py Version.py
pkgpython_PYTHON = ClientWrapper.py DUBDecoder.py MACAddress.py OlaClient.py \
                   RDMAPI.py RDMConstants.py PidStore.py UID.py __init__.py
endif

ArtnetConfigMessages_pb2.py: $(artnet_proto)
	$(PROTOC) --python_out ./ -I $(artnet_path) $(artnet_proto)

Ola_pb2.py: $(ola_proto)
	$(PROTOC) --python_out ./ -I $(ola_path) $(ola_proto)

Pids_pb2.py: $(pids_proto)
	$(PROTOC) --python_out ./ -I $(pids_path) $(pids_proto)

UsbProConfigMessages_pb2.py: $(usbpro_proto)
	$(PROTOC) --python_out ./ -I $(usbpro_path) $(usbpro_proto)

PidStoreLocation.py: ../../configure.ac
	echo "location = '${pid_datadir}'" > PidStoreLocation.py

Version.py: ../../configure.ac
	echo "version = '${VERSION}'" > Version.py

# TESTS
##################################################
TESTS_ENVIRONMENT = PYTHONPATH=${top_srcdir}/python
if BUILD_PYTHON_LIBS
TESTS = DUBDecoderTest.py MACAddressTest.py UIDTest.py
endif
check_SCRIPTS = DUBDecoderTest.py MACAddressTest.py UIDTest.py


CLEANFILES = ArtnetConfigMessages_pb2.* Ola_pb2.* Pids_pb2.* \
             UsbProConfigMessages_pb2.* PidStoreLocation.py Version.py *.pyc
