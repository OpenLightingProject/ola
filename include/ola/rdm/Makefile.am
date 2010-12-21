SOURCES = RDMAPI.h RDMAPIImplInterface.h RDMCommand.h \
          RDMControllerInterface.h RDMEnums.h RDMHelper.h \
          QueueingRDMController.h UID.h UIDSet.h

BUILT_SOURCES = RDMResponseCodes.h

EXTRA_DIST = $(SOURCES) make_rdm_codes.sh
pkginclude_HEADERS = $(SOURCES) RDMResponseCodes.h
pkgincludedir = $(includedir)/ola/rdm/


RDMResponseCodes.h:
	sh ./make_rdm_codes.sh

clean-local:
	rm -f RDMResponseCodes.h
