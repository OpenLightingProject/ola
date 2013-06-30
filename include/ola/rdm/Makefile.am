SOURCES = CommandPrinter.h DiscoveryAgent.h MessageDeserializer.h \
          MessageSerializer.h PidStore.h PidStoreHelper.h RDMAPI.h \
          RDMAPIImplInterface.h RDMCommand.h RDMCommandSerializer.h \
          RDMControllerAdaptor.h RDMControllerInterface.h RDMEnums.h \
          RDMHelper.h OpenLightingEnums.h RDMMessagePrinters.h RDMPacket.h \
          ResponderOps.h ResponderOpsPrivate.h StringMessageBuilder.h \
          QueueingRDMController.h UID.h UIDAllocator.h UIDSet.h

BUILT_SOURCES = RDMResponseCodes.h

EXTRA_DIST = $(SOURCES) make_rdm_codes.sh
pkginclude_HEADERS = $(SOURCES) RDMResponseCodes.h
pkgincludedir = $(includedir)/ola/rdm/


RDMResponseCodes.h: ../../../common/protocol/Ola.proto
	sh ./make_rdm_codes.sh

clean-local:
	rm -f RDMResponseCodes.h
