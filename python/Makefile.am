# ola python client

if BUILD_PYTHON_LIBS

SUBDIRS = rpc

artnet_path = ../plugins/artnet/messages
artnet_proto = $(artnet_path)/ArtnetConfigMessages.proto
ola_path = ../common/protocol
ola_proto = $(ola_path)/Ola.proto
usbpro_path = ../plugins/usbpro/messages
usbpro_proto = $(usbpro_path)/UsbProConfigMessages.proto

output_files = ArtnetConfigMessages_pb2.py Ola_pb2.py \
               UsbProConfigMessages_pb2.py

BUILT_SOURCES = $(output_files)
nodist_pkgpython_PYTHON = $(output_files)
pkgpython_PYTHON = OlaClient.py __init__.py

ArtnetConfigMessages_pb2.py: $(artnet_proto)
	protoc --python_out ./ -I $(artnet_path) $(artnet_proto)

Ola_pb2.py: $(ola_proto)
	protoc --python_out ./ -I $(ola_path) $(ola_proto)

UsbProConfigMessages_pb2.py: $(usbpro_proto)
	protoc --python_out ./ -I $(usbpro_path) $(usbpro_proto)

clean-local:
	rm -f ArtnetConfigMessages_pb2.* Ola_pb2.* UsbProConfigMessages_pb2.*

endif
