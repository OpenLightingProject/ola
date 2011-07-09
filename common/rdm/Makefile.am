include $(top_srcdir)/common.mk

SUBDIRS = testdata

EXTRA_DIST = Pids.proto PidStoreLoader.h

BUILT_SOURCES = Pids.pb.cc Pids.pb.h

noinst_LTLIBRARIES = libolardm.la
libolardm_la_SOURCES = PidStore.cpp PidStoreLoader.cpp RDMAPI.cpp \
                       RDMCommand.cpp RDMHelper.cpp StringMessageBuilder.cpp \
                       QueueingRDMController.cpp
nodist_libolardm_la_SOURCES = Pids.pb.cc
libolardm_la_LDFLAGS = $(PROTOBUF_LIBS)

TESTS = RDMTester PidStoreTester RDMMessageTester
check_PROGRAMS = $(TESTS)
RDMTester_SOURCES = RDMAPITest.cpp RDMCommandTest.cpp RDMTester.cpp \
                    QueueingRDMControllerTest.cpp UIDTest.cpp
RDMTester_CPPFLAGS = $(CPPUNIT_CFLAGS) $(AM_CPPFLAGS)
RDMTester_LDFLAGS = $(CPPUNIT_LIBS)
RDMTester_LDADD = libolardm.la ../logging/liblogging.la \
                  ../network/libolanetwork.la ../utils/libolautils.la

PidStoreTester_SOURCES = PidStoreTester.cpp PidStoreTest.cpp
PidStoreTester_CPPFLAGS = $(CPPUNIT_CFLAGS) $(AM_CPPFLAGS)
PidStoreTester_LDFLAGS = $(CPPUNIT_LIBS) $(PROTOBUF_LIBS)
PidStoreTester_LDADD = libolardm.la ../logging/liblogging.la \
                       ../messaging/libolamessaging.la

RDMMessageTester_SOURCES = RDMMessageTester.cpp StringMessageBuilderTest.cpp
RDMMessageTester_CPPFLAGS = $(CPPUNIT_CFLAGS) $(AM_CPPFLAGS)
RDMMessageTester_LDFLAGS = $(CPPUNIT_LIBS) $(PROTOBUF_LIBS)
RDMMessageTester_LDADD = libolardm.la ../logging/liblogging.la \
                          ../messaging/libolamessaging.la \
                          ../utils/libolautils.la

Pids.pb.cc Pids.pb.h: Pids.proto
	$(PROTOC) --cpp_out ./ Pids.proto

TestService.pb.cc TestService.pb.h: TestService.proto
	$(PROTOC) --cpp_out ./ TestService.proto

clean-local:
	rm -f *.pb.{h,cc}
