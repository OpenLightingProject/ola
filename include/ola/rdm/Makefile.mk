olardmincludedir = $(pkgincludedir)/rdm/

dist_noinst_SCRIPTS += include/ola/rdm/make_rdm_codes.sh

olardminclude_HEADERS = \
    include/ola/rdm/AckTimerResponder.h \
    include/ola/rdm/AdvancedDimmerResponder.h \
    include/ola/rdm/CommandPrinter.h \
    include/ola/rdm/DimmerResponder.h \
    include/ola/rdm/DimmerRootDevice.h \
    include/ola/rdm/DimmerSubDevice.h \
    include/ola/rdm/DiscoveryAgent.h \
    include/ola/rdm/DummyResponder.h \
    include/ola/rdm/MessageDeserializer.h \
    include/ola/rdm/MessageSerializer.h \
    include/ola/rdm/MovingLightResponder.h \
    include/ola/rdm/NetworkManagerInterface.h \
    include/ola/rdm/NetworkResponder.h \
    include/ola/rdm/OpenLightingEnums.h \
    include/ola/rdm/PidStore.h \
    include/ola/rdm/PidStoreHelper.h \
    include/ola/rdm/QueueingRDMController.h \
    include/ola/rdm/RDMAPI.h \
    include/ola/rdm/RDMAPIImplInterface.h \
    include/ola/rdm/RDMCommand.h \
    include/ola/rdm/RDMCommandSerializer.h \
    include/ola/rdm/RDMControllerAdaptor.h \
    include/ola/rdm/RDMControllerInterface.h \
    include/ola/rdm/RDMEnums.h \
    include/ola/rdm/RDMFrame.h \
    include/ola/rdm/RDMHelper.h \
    include/ola/rdm/RDMMessagePrinters.h \
    include/ola/rdm/RDMPacket.h \
    include/ola/rdm/RDMReply.h \
    include/ola/rdm/ResponderHelper.h \
    include/ola/rdm/ResponderLoadSensor.h \
    include/ola/rdm/ResponderNSCStatus.h \
    include/ola/rdm/ResponderOps.h \
    include/ola/rdm/ResponderOpsPrivate.h \
    include/ola/rdm/ResponderPersonality.h \
    include/ola/rdm/ResponderSensor.h \
    include/ola/rdm/ResponderSettings.h \
    include/ola/rdm/ResponderSlotData.h \
    include/ola/rdm/ResponderTagSet.h \
    include/ola/rdm/SensorResponder.h \
    include/ola/rdm/StringMessageBuilder.h \
    include/ola/rdm/SubDeviceDispatcher.h \
    include/ola/rdm/UID.h \
    include/ola/rdm/UIDAllocator.h \
    include/ola/rdm/UIDSet.h
nodist_olardminclude_HEADERS = include/ola/rdm/RDMResponseCodes.h

# BUILT_SOURCES is our only option here since we can't override the generated
# automake rules for common/libolacommon.la
built_sources += include/ola/rdm/RDMResponseCodes.h

include/ola/rdm/RDMResponseCodes.h: include/ola/rdm/Makefile.mk include/ola/rdm/make_rdm_codes.sh common/protocol/Ola.proto
	mkdir -p $(top_builddir)/include/ola/rdm
	sh $(top_srcdir)/include/ola/rdm/make_rdm_codes.sh $(top_srcdir)/common/protocol/Ola.proto > $(top_builddir)/include/ola/rdm/RDMResponseCodes.h
