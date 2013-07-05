SOURCES = AckTimerResponder.h CommandPrinter.h DimmerResponder.h \
          DimmerRootDevice.h DimmerSubDevice.h DiscoveryAgent.h \
          DummyResponder.h MessageDeserializer.h MessageSerializer.h \
          MovingLightResponder.h OpenLightingEnums.h PidStore.h \
          PidStoreHelper.h QueueingRDMController.h RDMAPI.h \
          RDMAPIImplInterface.h RDMCommand.h RDMCommandSerializer.h \
          RDMControllerAdaptor.h RDMControllerInterface.h RDMEnums.h \
          RDMHelper.h RDMMessagePrinters.h RDMPacket.h ResponderHelper.h \
          ResponderOps.h ResponderOpsPrivate.h ResponderPersonality.h \
          SensorResponder.h StringMessageBuilder.h SubDeviceDispatcher.h \
          UID.h UIDAllocator.h UIDSet.h

BUILT_SOURCES = RDMResponseCodes.h

EXTRA_DIST = $(SOURCES) make_rdm_codes.sh
pkginclude_HEADERS = $(SOURCES) RDMResponseCodes.h
pkgincludedir = $(includedir)/ola/rdm/


RDMResponseCodes.h: ../../../common/protocol/Ola.proto
	sh ./make_rdm_codes.sh

clean-local:
	rm -f RDMResponseCodes.h
