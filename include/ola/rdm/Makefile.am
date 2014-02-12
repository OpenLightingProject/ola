pkgincludedir = $(includedir)/ola/rdm/

BUILT_SOURCES = RDMResponseCodes.h
EXTRA_DIST = make_rdm_codes.sh

pkginclude_HEADERS = AckTimerResponder.h AdvancedDimmerResponder.h \
                     CommandPrinter.h DimmerResponder.h DimmerRootDevice.h \
                     DimmerSubDevice.h DiscoveryAgent.h DummyResponder.h \
                     MessageDeserializer.h MessageSerializer.h \
                     MovingLightResponder.h OpenLightingEnums.h PidStore.h \
                     PidStoreHelper.h QueueingRDMController.h RDMAPI.h \
                     RDMAPIImplInterface.h RDMCommand.h RDMCommandSerializer.h \
                     RDMControllerAdaptor.h RDMControllerInterface.h \
                     RDMEnums.h RDMHelper.h RDMMessagePrinters.h RDMPacket.h \
                     ResponderHelper.h ResponderLoadSensor.h ResponderOps.h \
                     ResponderOpsPrivate.h ResponderPersonality.h \
                     ResponderSensor.h ResponderSettings.h \
                     ResponderSlotData.h SensorResponder.h \
                     StringMessageBuilder.h SubDeviceDispatcher.h UID.h \
                     UIDAllocator.h UIDSet.h
nodist_pkginclude_HEADERS = RDMResponseCodes.h

RDMResponseCodes.h: make_rdm_codes.sh $(top_srcdir)/common/protocol/Ola.proto
	sh $(srcdir)/make_rdm_codes.sh $(top_srcdir)/common/protocol/Ola.proto

CLEANFILES = RDMResponseCodes.h
