# ola python client

EXTRA_DIST = UIDTest.py

if BUILD_PYTHON_LIBS

SUBDIRS = examples rpc

artnet_path = ../plugins/artnet/messages
artnet_proto = $(artnet_path)/ArtnetConfigMessages.proto
ola_path = ../common/protocol
ola_proto = $(ola_path)/Ola.proto
pids_path = ../common/rdm
pids_proto = $(pids_path)/Pids.proto
usbpro_path = ../plugins/usbpro/messages
usbpro_proto = $(usbpro_path)/UsbProConfigMessages.proto

output_files = ArtnetConfigMessages_pb2.py Ola_pb2.py \
               UsbProConfigMessages_pb2.py Pids_pb2.py \
               PidStoreLocation.py

BUILT_SOURCES = $(output_files)
nodist_pkgpython_PYTHON = $(output_files)
pkgpython_PYTHON = ClientWrapper.py OlaClient.py RDMAPI.py RDMConstants.py \
                   PidStore.py UID.py __init__.py

ArtnetConfigMessages_pb2.py: $(artnet_proto)
	protoc --python_out ./ -I $(artnet_path) $(artnet_proto)

Ola_pb2.py: $(ola_proto)
	protoc --python_out ./ -I $(ola_path) $(ola_proto)

Pids_pb2.py: $(pids_proto)
	protoc --python_out ./ -I $(pids_path) $(pids_proto)

UsbProConfigMessages_pb2.py: $(usbpro_proto)
	protoc --python_out ./ -I $(usbpro_path) $(usbpro_proto)

# This is a hack, but I can't find any other way to do it
piddir = $(pid_datadir)
PidStoreLocation.py:
	echo "location = '${piddir}/pids.proto'" > PidStoreLocation.py

clean-local:
	rm -f ArtnetConfigMessages_pb2.* Ola_pb2.* Pids_pb2.* UsbProConfigMessages_pb2.* PidStoreLocation.py

TESTS = UIDTest.py

endif
